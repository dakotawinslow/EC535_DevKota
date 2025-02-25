// Dakota Winslow 2025

#include <linux/module.h>  // Required for all kernel modules
#include <linux/kernel.h>  // Provides kernel macros like KERN_INFO
#include <linux/timer.h>   // Provides timer-related functions and structures
#include <linux/fs.h>      // File operations structure
#include <linux/uaccess.h> // For copy_from_user
#include <linux/slab.h>    // For kmalloc & kfree
#include <linux/string.h>  // For string operations
#include <linux/cdev.h>    // For character device registration

#define DEVICE_NAME "mytimer" // Device name in /dev
#define MAJOR_NUM 61           // Major # for the device
#define MINOR_NUM 0            // Minor # for the device

// Configurable soft max number of timers
int g_num_timers;
// Array of timer structures
struct msg_timer
{
    struct timer_list timer;
    char *message;
};
struct msg_timer *g_timers;

/**
 * timer_init - Initialize the timers.
 *
 * This function sets up the timers and their callback functions.
 *
 * Return: 0 on success, error code on failure.
 */
static int timer_init(void)
{
    int i;
    for (i = 0; i < g_num_timers; i++)
    {
        timer_setup(&g_timers[i].timer, NULL, 0);
    }
    return 0;
}

/**
 * timers_destroy - Destroy all timers and free allocated memory.
 *
 * Return: 0 on success.
 */
static int timers_destroy(void)
{
    int i;
    for (i = 0; i < g_num_timers; i++)
    {
        kfree((char *)g_timers[i].timer.function);
        del_timer(&g_timers[i].timer);
    }

    kfree(g_timers);
    return 0;
}

/**
 * timer_cb - Callback function for the timer.
 * @timer: Pointer to the timer_list structure.
 *
 * This function is called when the timer expires. It prints the message
 * associated with the timer and frees the allocated memory.
 */
void timer_cb(struct timer_list *timer)
{
    struct msg_timer *msg_timer = container_of(timer, struct msg_timer, timer);
    printk(KERN_INFO "%s\n", msg_timer->message);
    timer->function = NULL;
    kfree(msg_timer->message);
}

/**
 * create_timers - Allocate space for a dynamic number of timers.
 * @num_timers: Number of timers to create.
 *
 * Return: 0 on success, -1 on failure.
 */
static int create_timers(uint num_timers)
{
    g_num_timers = num_timers;

    g_timers = kmalloc(num_timers * sizeof(struct msg_timer), GFP_KERNEL);

    if (!g_timers)
        return -1;

    // Initialize the timers
    return timer_init();
}

/**
 * set_timer - Set a timer to expire after a specified number of seconds.
 * @index: Index of the timer to set.
 * @seconds: Number of seconds to wait before the timer expires.
 * @message: Message to print when the timer expires.
 *
 * Return: 0 on success, error code on failure.
 */
int set_timer(int index, int seconds, char *message)
{
    if (index < 0 || index >= g_num_timers)
        return -EINVAL;
    g_timers[index].timer.function = timer_cb;

    g_timers[index].message = kmalloc(strlen(message) + 1, GFP_KERNEL);
    if (!g_timers[index].message)
        return -ENOMEM;
    strcpy(g_timers[index].message, message);
    // printk(KERN_INFO "DEBUG: Setting timer %d to expire in %d seconds with message '%s'\n", index, seconds, g_timers[index].message);

    return mod_timer(&g_timers[index].timer, jiffies + seconds * HZ);
}

/**
 * find_available_timer - Find the index of the first available timer or a timer with a matching message.
 * @message: Message to match against existing timers.
 *
 * Return: Index of the available timer or -ENOMEM if no available timer is found.
 */
int find_available_timer(char *message)
{
    int i;
    // Check for existing timers with matching messages
    for (i = 0; i < g_num_timers; i++)
    {
        if (g_timers[i].timer.function && strcmp(g_timers[i].message, message) == 0)
            // printk(KERN_INFO "The The timer %s was updated!", g_timers[i].message);
            return i;
    }
    // Find the first available timer
    for (i = 0; i < g_num_timers; i++)
    {
        if (!g_timers[i].timer.function)
            return i;
    }
    return -ENOMEM;
}

/**
 * parse_control_input - Parse the user input as a control command.
 * @input: User input string.
 *
 * Return: 0 on success, error code on failure.
 *
 * Currently only reacts to $COUNT=<number> to set the number of timers.
 */
int parse_control_input(char *input)
{
    char *token = strsep(&input, "=");
    if (!token)
        return -EINVAL;

    if (strcmp(token, "COUNT") == 0)
    {
        long num_timers;
        int ret = kstrtol(input, 10, &num_timers);
        if (ret < 0)
            return ret;
        return create_timers(num_timers);
    }
    else
    {
        return -EINVAL;
    }
}

/**
 * parse_timer_input - Parse the user input as a number of seconds for a timer.
 * @input: User input string.
 *
 * Return: 0 on success, error code on failure.
 *
 * User input format: "<seconds> <message>"
 */
int parse_timer_input(char *input)
{
    long seconds;
    char *message;
    char *token;
    int index;
    int ret;

    if (input[0] == '^')
    {
        return parse_control_input(input + 1);
    }

    token = strsep(&input, " ");
    if (!token)
        return -EINVAL;

    ret = kstrtol(token, 10, &seconds);
    if (ret < 0)
        return ret;

    message = strsep(&input, "\n");
    if (!message)
        return -EINVAL;

    index = find_available_timer(message);
    if (index >= 0)
        return set_timer(index, seconds, message);
    else
        return 0;
}

/**
 * timer_write - Write handler for the timer control file.
 * @file: Pointer to the file structure.
 * @buf: User-space buffer containing data.
 * @count: Number of bytes to write.
 * @ppos: Current position in the file.
 *
 * Sends user inputs to the parse_timer_input function.
 *
 * Return: Number of bytes written or an error code.
 */
static ssize_t timer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[128];
    ssize_t ret;

    if (count > 128)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;

    kbuf[count] = '\0';
    ret = parse_timer_input(kbuf);

    if (ret >= 0)
        *ppos += count;

    return ret < 0 ? ret : count;
}

/**
 * get_active_timers - Get a list of active timers.
 *
 * Return: A string containing the active timers. Each line has one timer,
 *         with the format "<message> <seconds remaining>\n".
 */
static char *get_active_timers(void)
{
    char *active_timers;
    int i;
    int len = 0;
    int remaining;
    int seconds;
    int pos = 0;

    for (i = 0; i < g_num_timers; i++)
    {
        if (g_timers[i].timer.function)
        {
            seconds = (g_timers[i].timer.expires - jiffies) / HZ;
            remaining = seconds > 0 ? seconds : 0;
            len += strlen(g_timers[i].message) + 20;
        }
    }

    if (len == 0)
        return NULL;

    active_timers = kmalloc(len + 1, GFP_KERNEL);
    if (!active_timers)
        return NULL;

    for (i = 0; i < g_num_timers; i++)
    {
        if (g_timers[i].timer.function)
        {
            seconds = (g_timers[i].timer.expires - jiffies) / HZ;
            remaining = seconds > 0 ? seconds : 0;
            pos += sprintf(active_timers + pos, "%s %d\n", g_timers[i].message, remaining);
        }
    }

    return active_timers;
}

/**
 * check_for_room - Check if there is room for a new timer.
 *
 * Return: 1 if there is room, 0 if there is not.
 */
static char check_for_room(void)
{
    int i;
    char ret = '0';
    for (i = 0; i < g_num_timers; i++)
    {
        if (!g_timers[i].timer.function)
            ret = '1';
    }
    return ret;
}

/**
 * timer_read - Read handler for the timer control file.
 * @file: Pointer to the file structure.
 * @buf: User-space buffer to copy data to.
 * @count: Number of bytes to read.
 * @ppos: Current position in the file.
 *
 * Return: Number of bytes read or an error code.
 */
static ssize_t timer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char *message;
    char has_room;
    int len;

    // Check if g_timers is NULL
    if (!g_timers)
    {
        // Send a null string if no timers are set
        if (copy_to_user(buf, "", 1))
            return -EFAULT;
        return 1;
    }

    message = get_active_timers();
    has_room = check_for_room();
    // printk(KERN_INFO "The has_room value is %c\n", has_room);

    // append has_room char to end of message
    if (message)
    {
        len = strlen(message);
        message[len-1] = has_room;
        message[len] = '\n';
        message[len+1] = '\0';
        // printk(KERN_INFO "The message is %s\n", message);
    }
    else
    {
        message = kmalloc(2, GFP_KERNEL);
        message[0] = has_room;
        message[1] = '\0';
        // printk(KERN_INFO "The message is %s\n", message);
    }

    len = strlen(message);
    // printk(KERN_INFO "The length of the message is %d\n", len);

    // If *ppos is greater than or equal to len, return 0 to indicate end of file
    if (*ppos >= len)
    {
        kfree(message);
        *ppos = 0; // Reset the file position pointer
        return 0;
    }

    // Adjust count if it exceeds the remaining length of the message
    if (count > len - *ppos)
        count = len - *ppos;

    // Copy data to user space
    if (copy_to_user(buf, message + *ppos, count))
    {
        kfree(message);
        return -EFAULT;
    }

    // Update the file position pointer
    *ppos += count;

    // Free the allocated memory for the message
    kfree(message);

    return count;
}

// Define file operations for the timer interface
static const struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .write = timer_write,
    .read = timer_read,
};

static dev_t dev_num;
static struct cdev my_cdev;

/**
 * my_module_init - Module initialization function.
 *
 * This function registers a character device and sets up the module defaults.
 *
 * Return: 0 on success, error code on failure.
 */
static int __init my_module_init(void)
{
    int ret;

    create_timers(1);

    // Allocate device number
    dev_num = MKDEV(MAJOR_NUM, MINOR_NUM);
    ret = register_chrdev_region(dev_num, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to register char device region\n");
        return ret;
    }

    // Initialize and add the character device
    cdev_init(&my_cdev, &timer_fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add char device\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    return 0; // Return 0 to indicate successful initialization
}

/**
 * my_module_exit - Module cleanup function.
 *
 * This function unregisters the character device and deletes the timer.
 */
static void __exit my_module_exit(void)
{
    printk(KERN_INFO "Unloading module and deleting timer control file...\n");
    timers_destroy();          // Delete all the timers
    cdev_del(&my_cdev);        // Remove the character device
    unregister_chrdev_region(dev_num, 1); // Unregister the device number
}

// Register module entry and exit points
module_init(my_module_init);
module_exit(my_module_exit);

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dakota Winslow");
MODULE_DESCRIPTION("A Linux kernel timer module supporting multiple timers. C 2025");
