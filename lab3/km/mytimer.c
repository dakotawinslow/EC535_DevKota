#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>


#define MSG_LEN               128
#define MAX_INFO_LENGTH       PAGE_SIZE
#define COMM_SIZE             16
#define MAX_TIMERS            2


MODULE_LICENSE("Dual BSD/GPL");

static int mytimer_init(void);
static void mytimer_exit(void);
static int mytimer_open(struct inode *inode, struct file *filp);
static int proc_open(struct inode *inode, struct file *filp);
static int mytimer_release(struct inode *inode, struct file *filp);
static ssize_t mytimer_write(struct file *filp,
                             const char __user *buf, size_t count, loff_t *f_pos);
static void mytimer_handler_0(struct timer_list*);
static void mytimer_handler_1(struct timer_list*);
static int mytimer_async(int fd, struct file *filp, int mode);
static int mytimer_proc_show(struct seq_file *m, void *v);
static ssize_t mytimer_read(struct file *filp,
                            char *buf, size_t count, loff_t *f_pos);
static int prepare_read_msg(void);
uint my_atoi(const char* seconds);
char * my_strcpy(char *dest, const char *src);
int my_strcmp(const char *str1, const char *str2);

static int timer_major = 61;
static int proc_index;
static int next_message;
static char *mytimer_info;
static struct proc_dir_entry *proc_entry;
char kernel_out[1000] = {0};
// char calling_command[COMM_SIZE] = {0};
// int calling_process = 0;
int cat_process = 0;
static unsigned long module_load_time;  // Store the module's load time
int allowed_timers = 1;
int number_active_timers = 0;
int cat = 0;
int timer_msgs = 0;

struct timer_info
{
    struct timer_list timer;
    int owner_pid;
    char calling_command[COMM_SIZE];
    char timer_msg[MSG_LEN];
    int active;
};

static void send_wakeup_call(struct timer_info *timer);
struct timer_info timers[MAX_TIMERS];
struct fasync_struct *async_queue; /* structure for keeping track of asynchronous readers */


struct file_operations chrdev_fops = {
    .owner = THIS_MODULE,
    .read = mytimer_read,
    .write = mytimer_write,
    .open = mytimer_open,
    .fasync = mytimer_async,
    .release = mytimer_release
};

static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .open = proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = mytimer_release
};

module_init(mytimer_init);
module_exit(mytimer_exit);

static int mytimer_init(void)
{
    int ret = 0;

    int result;

    printk(KERN_INFO "Loading mytimer module\n");

    module_load_time = jiffies;

    result = register_chrdev(timer_major, "mytimer", &chrdev_fops);
    if(result < 0) {
        printk(KERN_ALERT
               "mytimer: cannot obtain major number %d\n", timer_major);
        return result;

    }

    mytimer_info = (char *)vmalloc(MAX_INFO_LENGTH);

    if (!mytimer_info) {
        ret = -ENOMEM;
    } else {
        memset(mytimer_info, 0, MAX_INFO_LENGTH);
        proc_entry = proc_create("mytimer", 0644, NULL, &proc_fops);
        if (proc_entry == NULL) {
            ret = -ENOMEM;
            vfree(mytimer_info);
            printk(KERN_INFO "mytimer: Couldn't create proc entry\n");
        } else {
            proc_index = 0;
            next_message = 0;
            // printk(KERN_INFO "mytimer: Module loaded.\n");
        }
    }


    return ret;
}

static void mytimer_exit(void) {
    /* Freeing the major number */
    int i;
    for(i = 0; i < allowed_timers; i++) {
        if (timer_pending(&timers[i].timer) && timers[i].active) {
            del_timer_sync(&timers[i].timer); // Only delete if active
            timers[i].active = 0;
            number_active_timers --;
        }
    }
    unregister_chrdev(timer_major, "mytimer");

    remove_proc_entry("mytimer", NULL);
    vfree(mytimer_info);

    printk(KERN_ALERT "Removing mytimer module\n");

}

static ssize_t mytimer_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {

    char user_buf[150] = {0};

    if(count > sizeof(user_buf) - 1) {
        return -EINVAL;
    }

    if(copy_from_user(user_buf, buf, count)) { //safely moves bytes between user program and kernel memory
        return -EFAULT;
    }

    user_buf[min(count, sizeof(user_buf) - 1)] = '\0';

    // if(user_buf[1] == 'l') {

    //     int i;
    //     int offset = 0;
    //     memset(kernel_out, 0, sizeof(kernel_out));
    //     for(i = 0; i < MAX_TIMERS; i++) {
    //         if(timers[i].active) {
    //             // printk(KERN_INFO "%s %lu\n", timers[i].timer_msg, ((timers[i].timer.expires - jiffies)/HZ));
    //             offset += snprintf(kernel_out + offset, sizeof(kernel_out) - offset,
    //                                "%s %lu\n", timers[i].timer_msg, ((timers[i].timer.expires - jiffies)/HZ));
    //         }
    //     }
    //     return count;
    // }
    if (user_buf[1] == 'r') {
        int i;
        for (i = 0; i < MAX_TIMERS; i++) {
            if (timer_pending(&timers[i].timer) && timers[i].active) {
                del_timer_sync(&timers[i].timer); // Only delete if active
                // struct task_struct *task = get_pid_task(timers[i].owner_pid, PIDTYPE_PID);
                // if (task != NULL) {
                //     // kill the process that set the timer
                //     send_sig(SIGKILL, task, 1);
                // }
                timers[i].active = 0;
            }
        }
        number_active_timers = 0;
        return count;
    }
    else if(user_buf[1] == 'm') {
        // char c_count[6] = {0};
        // int count;
        // int i = 3;
        // int j = 0;

        // while(user_buf[i] != '\0' && j < 5) //extract expiration seconds from string
        // {
        //     c_count[j] = user_buf[i];
        //     i++;
        //     j++;
        // }
        // c_count[j] = '\0';

        // count = my_atoi(c_count);
        // printk(KERN_INFO "DEBUG: user_buf[3] is '%c'\n", user_buf[3]);
        if (user_buf[3] == '1') {
            allowed_timers = 1;
            return count;
        } else if (user_buf[3] == '2') {
            allowed_timers = 2;
            return count;
        }
        return count;

    }
    else if(user_buf[1] == 's') {

        char user_msg[129] = {0};
        char c_sec[6] = {0};
        uint sec;
        int i = 3;
        int j = 0;
        int k = 0;

        // calling_process = current->pid;

        while(user_buf[i] != ' ' && user_buf[i] != '\0' && j < 5) //extract expiration seconds from string
        {
            c_sec[j] = user_buf[i];
            i++;
            j++;
        }
        c_sec[j] = '\0';
        if(user_buf[i] == ' ') i++;
        j=0;

        while(user_buf[i] != '\0' && j < 128) //extract timer message
        {
            user_msg[j] = user_buf[i];
            i++;
            j++;
        }
        user_msg[j] = '\0';


        sec = my_atoi(c_sec);


        for(k = 0; k < MAX_TIMERS; k++) {
            if((my_strcmp(timers[k].timer_msg, user_msg) == 0) && timers[k].active) {
                mod_timer(&timers[k].timer, jiffies + sec * HZ);
                memset(kernel_out, 0, sizeof(kernel_out));
                // snprintf(kernel_out, sizeof(kernel_out), "The timer %s was updated!\n", timers[k].timer_msg);
                return count;
            }
        }

        if(number_active_timers < allowed_timers) {
            int i;
            for(i = 0; i < MAX_TIMERS; i++) {
                if(!timers[i].active) {
                    timers[i].active = 1;
                    my_strcpy(timers[i].timer_msg, user_msg);
                    timers[i].owner_pid = current->pid;
                    my_strcpy(timers[i].calling_command, current->comm);
                    // printk(KERN_INFO "DEBUG: timer_msg after timer set: [%s]\n", timers[i].timer_msg);
                    // timer_setup(&timers[i].timer, mytimer_handler, 0);
                    if (i == 0) {
                        timer_setup(&timers[0].timer, mytimer_handler_0, 0);
                    } else {
                        timer_setup(&timers[1].timer, mytimer_handler_1, 0);
                    }
                    number_active_timers++;
                    // printk(KERN_INFO "Number of active timers is %u\n", number_active_timers);
                    mod_timer(&timers[i].timer, jiffies + sec * HZ);
                    memset(kernel_out, 0, sizeof(kernel_out));
                    return count;
                }
            }
        }
    }
    return count;
}

static int mytimer_async(int fd, struct file *filp, int mode) {
    // printk(KERN_INFO "setting async\n");
    return fasync_helper(fd, filp, mode, &async_queue);
}

static void mytimer_handler_0(struct timer_list *data) {
    number_active_timers --;
    timers[0].active = 0;
    send_wakeup_call(&timers[0]);
    // printk(KERN_INFO "timer expired\n");
    // if (async_queue)
    //     kill_fasync(&async_queue, SIGIO, POLL_IN);
}

static void mytimer_handler_1(struct timer_list *data) {
    number_active_timers --;
    timers[1].active = 0;
    send_wakeup_call(&timers[1]);
    // printk(KERN_INFO "timer expired\n");
    // if (async_queue)
    //     kill_fasync(&async_queue, SIGIO, POLL_IN);
}

static void send_wakeup_call(struct timer_info *timer) {
    struct pid *pid_struct;
    struct task_struct *task;
    pid_struct = find_get_pid(timer->owner_pid);
    task = pid_task(pid_struct, PIDTYPE_PID);
    if (task != NULL) {
        send_sig(SIGIO, task, 1);
    }
}

static int mytimer_proc_show(struct seq_file *m, void *v) {

    unsigned long elapsed = jiffies - module_load_time;
    unsigned long elapsed_ms = jiffies_to_msecs(elapsed);
    unsigned long expiration = 0;
    char buf[1000];

    if(timers[0].active) {
        expiration = ((timers[0].timer.expires - jiffies) / HZ);
    } else {
        expiration = 0;
    }

    snprintf(buf, 1000, "module: %s\n"
             "uptime: %lu ms\n",
             THIS_MODULE->name, elapsed_ms);

    if (timers[0].active) {
        snprintf(buf + strlen(buf), 1000 - strlen(buf), "~~~ Timer 0 Active ~~~\ncalling pid: %d\n"
                 "command: %s\n"
                 "expiration: %lu s\n",
                 timers[0].owner_pid, timers[0].calling_command, expiration);
    }
    if (timers[1].active) {
        expiration = ((timers[1].timer.expires - jiffies) / HZ);
        snprintf(buf + strlen(buf), 1000 - strlen(buf), "~~~ Timer 1 Active ~~~\ncalling pid: %d\n"
                 "command: %s\n"
                 "expiration: %lu s\n",
                 timers[1].owner_pid, timers[1].calling_command, expiration);
    }

    seq_printf(m, "%s", buf);

    return 0;
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
static ssize_t mytimer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char *message;
    char has_room;
    int len;

    // prepeare the message to be read (will be stored in kernel_out)
    prepare_read_msg();

    len = strlen(kernel_out);
    // printk(KERN_INFO "The length of the message is %d\n", len);

    // If *ppos is greater than or equal to len, return 0 to indicate end of file
    if (*ppos >= len)
    {
        // kfree(message);
        // *ppos = 0; // Reset the file position pointer
        return 0;
    }

    // Adjust count if it exceeds the remaining length of the message
    if (count > len - *ppos)
        count = len - *ppos;

    // Copy data to user space
    if (copy_to_user(buf, kernel_out + *ppos, count))
    {
        // kfree(message);
        return -EFAULT;
    }

    // Update the file position pointer
    *ppos += count;

    // Free the allocated memory for the message
    // kfree(message);

    return count;
}

static int prepare_read_msg(void)
{
    int i;
    int offset = 0;

    // printk(KERN_INFO "DEBUG: MAX_TIMERS is %d\n", MAX_TIMERS);
    for(i = 0; i < MAX_TIMERS; i++) {

        // printk(KERN_INFO "DEBUG: Timer %d active: %d\n", i, timers[i].active);
        if(timers[i].active) {
            // printk(KERN_INFO "DEBUG: Timer %d active\n", i);
            // printk(KERN_INFO "DEBUG: msg - '%s', time -  '%lu'\n", timers[i].timer_msg, ((timers[i].timer.expires - jiffies)/HZ));
            offset += snprintf(kernel_out + offset, sizeof(kernel_out) - offset,
                               "%s %lu\n",
                               timers[i].timer_msg,
                               ((timers[i].timer.expires - jiffies)/HZ));
        }
        // printk(KERN_INFO "%s", kernel_out);
    }
    // tack on information about currently set timers
    offset += snprintf(kernel_out + offset, sizeof(kernel_out) - offset,
                       " %d %d", number_active_timers, allowed_timers);
    // printk(KERN_INFO "DEBUG: kernel_out is [%s]\n", kernel_out);
    return 0;
}

static int proc_open(struct inode *inode, struct file *filp) {
    // cat = 1;
    // printk(KERN_INFO "proc module opened\n");
    return single_open(filp, mytimer_proc_show, NULL);

}

static int mytimer_open(struct inode *inode, struct file *filp) {
    // cat = 1;
    // printk(KERN_INFO "chrdev module opened\n");
    // return single_open(filp, mytimer_chrdev_show, NULL);
    // printk(KERN_INFO "DEBUG: mytimer_open() opened\n");
    return 0;
}

static int mytimer_release(struct inode *inode, struct file *filp) {
    // printk(KERN_INFO "module released\n");
    return 0;
}

uint my_atoi(const char* seconds) {
    int result = 0;

    while(*seconds == ' ' || *seconds == '\t' || *seconds == '\n') {
        seconds ++;
    }

    while(*seconds >= '0' && *seconds <= '9') {
        result = result * 10 + (*seconds - '0');
        seconds ++;
    }

    return result > 0 ? result : 0;
}

char * my_strcpy(char *dest, const char *src) {
    char *orig_dest = dest;

    while(*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return orig_dest;
}

int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) { // Continue while characters are the same and not null
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}
