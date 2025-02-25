#include <linux/module.h>  // Required for all kernel modules
#include <linux/kernel.h>  // Provides kernel macros like KERN_INFO
#include <linux/timer.h>   // Provides timer-related functions and structures
#include <linux/fs.h>      // File operations structure
#include <linux/uaccess.h> // For copy_from_user
#include <linux/miscdevice.h>
#include <linux/slab.h>    // For kmalloc & kfree

#define MAX_TIMERS 6 // Hard maximum number of timers
// I really wanted to support arbitrary numbers of timers, but
// since they removed the data field from the timer_list,
// I just cannot find a way to pass information to the callback function.
// There is just no way for a generic timer callback to know what timer
// it is responding to, since the timer_list structure gets moved
// around in memory by the kernel and the timer structure has no unique
// identifier. The limitations of C prevent me from spinning up a custom
// callback function for each timer programmatically, so I am stuck with
// a fixed number of timers. I am very disappointed by this limitation.

// Configurable soft max number of timers
int g_num_timers;
// Array of timer structures
struct timer_list *g_timers;
// Array of timer callback function pointers
typedef void (*timer_cb)(struct timer_list *timer);
timer_cb g_timer_cbs[MAX_TIMERS];
// Array of timer end messages
char *g_timer_messages[MAX_TIMERS];



// callback for timer 0
void timer_cb0(struct timer_list *timer)
{
    // printk(KERN_INFO "%s\n", g_timer_messages[0]);
    printk(KERN_INFO "%s\n", g_timer_messages[0]);
    timer->function = NULL;
    kfree(g_timer_messages[0]);
}

// callback for timer 1
void timer_cb1(struct timer_list *timer)
{
    printk(KERN_INFO "%s\n", g_timer_messages[1]);
}

// callback for timer 2
void timer_cb2(struct timer_list *timer)
{
    printk(KERN_INFO "%s\n", g_timer_messages[2]);
}

// callback for timer 3
void timer_cb3(struct timer_list *timer)
{
    printk(KERN_INFO "%s\n", g_timer_messages[3]);
}

// callback for timer 4
void timer_cb4(struct timer_list *timer)
{
    printk(KERN_INFO "%s\n", g_timer_messages[4]);
}

// callback for timer 5
void timer_cb5(struct timer_list *timer)
{
    printk(KERN_INFO "%s\n", g_timer_messages[5]);
}

void set_timer_cbs(void)
{
    g_timer_cbs[0] = timer_cb0;
    g_timer_cbs[1] = timer_cb1;
    g_timer_cbs[2] = timer_cb2;
    g_timer_cbs[3] = timer_cb3;
    g_timer_cbs[4] = timer_cb4;
    g_timer_cbs[5] = timer_cb5;
}

/**
 * create_timers - Allocate space for a dynamic number of timers.
 * @num_timers: Number of timers to create.
 */
struct timer_list *create_timers(uint num_timers)
{
    struct timer_list *timers;

    g_num_timers = num_timers;

    timers = kmalloc(num_timers * sizeof(struct timer_list), GFP_KERNEL);

    if (!timers)
        return NULL;

    return timers;
}

/**
 * set_timer - Set a timer to expire after a specified number of seconds.
 * @index: Index of the timer to set.
 * @seconds: Number of seconds to wait before the timer expires.
 * @message: Message to print when the timer expires.
 */
int set_timer(int index, int seconds, char *message)
{
    if (index < 0 || index >= g_num_timers)
        return -EINVAL;
    g_timers[index].function = g_timer_cbs[index];

    g_timer_messages[index] = kmalloc(strlen(message) + 1, GFP_KERNEL);
    if (!g_timer_messages[index])
        return -ENOMEM;
    strcpy(g_timer_messages[index], message);
    printk(KERN_INFO "DEBUG: Setting timer %d to expire in %d seconds with message '%s'\n", index, seconds, g_timer_messages[index]);

    return mod_timer(&g_timers[index], jiffies + seconds * HZ);
}



/**
 * find_available_timer - Find the index of the first available timer.
 */
int find_available_timer(void)
{
    int i;
    for (i = 0; i < g_num_timers; i++)
    {
        if (!g_timers[i].function)
            return i;
    }
    return -ENOMEM;
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
    char *token = strsep(&input, " ");
    int index;
    int ret;

    if (!token)
        return -EINVAL;

    ret = kstrtol(token, 10, &seconds);
    if (ret < 0)
        return ret;

    message = strsep(&input, "\n");
    printk(KERN_INFO "DEBUG: Message: %s\n", message);
    if (!message)
        return -EINVAL;

    index = find_available_timer();
    if (index >= 0)
        return set_timer(index, seconds, message);
    else
        return -ENOMEM;
}

/**
 * timer_write - Write handler for the timer control file.
 * @file: Pointer to the file structure.
 * @buf: User-space buffer containing data.
 * @count: Number of bytes to write.
 * @ppos: Current position in the file.
 *
 *Sends user inputs to the parse_timer_input function.
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

static ssize_t timer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char message[] = "How am I supposed to know what kind of timers you have set up?\n";
    size_t len = strlen(message);

    if (*ppos >= len)
        return 0;

    if (count > len - *ppos)
        count = len - *ppos;

    if (copy_to_user(buf, message + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

// Define file operations for the timer interface
static const struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .write = timer_write,
    .read = timer_read,
};

// Define a misc device to expose the timer control
static struct miscdevice timer_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "timer",
    .fops = &timer_fops,
};

/**
 * timers_init - Initialize the timers.
 *
 * This function sets up the timers and their callback functions.
 *
 * Return: 0 on success, error code on failure.
 */
static int timers_init(void)
{
    int i;
    for (i = 0; i < g_num_timers; i++)
    {
        timer_setup(&g_timers[i], NULL, 0);
    }
    return 0;
}

static int timers_destroy(void)
{
    int i;
    for (i = 0; i < g_num_timers; i++)
    {
        del_timer(&g_timers[i]);
        kfree((char *)g_timers[i].function);
    }

    kfree(g_timers);
    return 0;
}

/**
 * my_module_init - Module initialization function.
 *
 * This function registers a misc device and sets up the module defaults.
 *
 * Return: 0 on success, error code on failure.
 */
static int __init my_module_init(void)
{
    int ret;

    set_timer_cbs();
    printk(KERN_INFO "DEBUG Loading module and setting up timer control file...\n");
    g_timers = create_timers(1);

    // Initialize the timers (only one for now)
    timers_init();

    // Register the misc device
    ret = misc_register(&timer_device);
    if (ret)
    {
        printk(KERN_ERR "Failed to register timer device\n");
        return ret;
    }

    return 0; // Return 0 to indicate successful initialization
}

/**
 * my_module_exit - Module cleanup function.
 *
 * This function unregisters the misc device and deletes the timer.
 */
static void __exit my_module_exit(void)
{
    printk(KERN_INFO "Unloading module and deleting timer control file...\n");
    timers_destroy();          // Delete all the timers
    misc_deregister(&timer_device); // Remove the device file
}

// Register module entry and exit points
module_init(my_module_init);
module_exit(my_module_exit);

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dakota Winslow");
MODULE_DESCRIPTION("A Linux kernel timer module supporting multiple timers. C 2025");
