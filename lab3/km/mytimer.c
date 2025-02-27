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


#define MSG_LEN               128
#define MAX_INFO_LENGTH       PAGE_SIZE
#define COMM_SIZE             16
#define MAX_TIMERS            1


MODULE_LICENSE("Dual BSD/GPL");

static int mytimer_init(void);
static void mytimer_exit(void);
static int mytimer_open(struct inode *inode, struct file *filp);
static int proc_open(struct inode *inode, struct file *filp);
static int mytimer_release(struct inode *inode, struct file *filp);
static ssize_t mytimer_write(struct file *filp,
                             const char __user *buf, size_t count, loff_t *f_pos);
static void mytimer_handler(struct timer_list*);
static int mytimer_async(int fd, struct file *filp, int mode);
static int mytimer_proc_show(struct seq_file *m, void *v);
uint my_atoi(const char* seconds);
char * my_strcpy(char *dest, const char *src);
int my_strcmp(const char *str1, const char *str2);

static int timer_major = 61;
static int proc_index;
static int next_message;
static char *mytimer_info;
static struct proc_dir_entry *proc_entry;
char kernel_out[1000] = {0};
char calling_command[COMM_SIZE] = {0};
int calling_process = 0;
int cat_process = 0;
static unsigned long module_load_time;  // Store the module's load time
int allowed_timers = 1;
int number_active_timers = 0;
int cat = 0;
int timer_msgs = 0;

struct timer_info
{
    struct timer_list timer;
    char timer_msg[MSG_LEN];
    int active;
};

struct timer_info timers[MAX_TIMERS];
struct fasync_struct *async_queue; /* structure for keeping track of asynchronous readers */


struct file_operations chrdev_fops = {
    .read = seq_read,
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
            printk(KERN_INFO "mytimer: Module loaded.\n");
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

    if(user_buf[1] == 'l') {

        int i;
        int offset = 0;
        memset(kernel_out, 0, sizeof(kernel_out));
        for(i = 0; i < MAX_TIMERS; i++) {
            if(timers[i].active) {
                printk(KERN_INFO "%s %lu\n", timers[i].timer_msg, ((timers[i].timer.expires - jiffies)/HZ));
                offset += snprintf(kernel_out + offset, sizeof(kernel_out) - offset,
                                   "%s %lu\n", timers[i].timer_msg, ((timers[i].timer.expires - jiffies)/HZ));
            }
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

        calling_process = current->pid;
        my_strcpy(calling_command, current->comm);

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
                snprintf(kernel_out, sizeof(kernel_out), "The timer %s was updated!\n", timers[k].timer_msg);
                return count;
            }
        }

        if(number_active_timers < allowed_timers) {
            int i;
            for(i = 0; i < MAX_TIMERS; i++) {
                if(!timers[i].active) {
                    timers[i].active = 1;
                    my_strcpy(timers[i].timer_msg, user_msg);
                    // printk(KERN_INFO "DEBUG: timer_msg after timer set: [%s]\n", timers[i].timer_msg);
                    timer_setup(&timers[i].timer, mytimer_handler, 0);
                    number_active_timers++;
                    // printk(KERN_INFO "Number of active timers is %u\n", number_active_timers);
                    mod_timer(&timers[i].timer, jiffies + sec * HZ);
                    memset(kernel_out, 0, sizeof(kernel_out));
                    return count;
                }
            }
        } else {
            printk(KERN_INFO "%u timer(s) already exist(s)!\n", allowed_timers);
            memset(kernel_out, 0, sizeof(kernel_out));
            snprintf(kernel_out, sizeof(kernel_out), "%u timer(s) already exist(s)!\n", allowed_timers);
        }
    }
    printk(KERN_INFO "Done with write\n");

    return count;
}

static int mytimer_async(int fd, struct file *filp, int mode) {
    // printk(KERN_INFO "setting async\n");
    return fasync_helper(fd, filp, mode, &async_queue);
}

static void mytimer_handler(struct timer_list *data) {
    number_active_timers --;
    timers[0].active = 0;
    // printk(KERN_INFO "timer expired\n");
    if (async_queue)
        kill_fasync(&async_queue, SIGIO, POLL_IN);

}

static int mytimer_proc_show(struct seq_file *m, void *v) {

    unsigned long elapsed = jiffies - module_load_time;
    unsigned long elapsed_ms = jiffies_to_msecs(elapsed);
    unsigned long expiration = 0;

    if(timers[0].active) {
        expiration = ((timers[0].timer.expires - jiffies) / HZ);
    } else {
        expiration = 0;
    }

    seq_printf(m, "module: %s\n"
               "load timer: %lu ms\n"
               "calling pid: %d\n"
               "command: %s\n"
               "expiration: %lu s\n",
               THIS_MODULE->name, elapsed_ms,
               calling_process, calling_command,
               expiration);

    return 0;
}

static int mytimer_chrdev_show(struct seq_file *m, void *v) {
    int i;
    int offset = 0;
    for(i = 0; i < MAX_TIMERS; i++) {
        if(timers[i].active) {
            offset += snprintf(kernel_out + offset, sizeof(kernel_out) - offset,
                               "%s %lu\n", timers[i].timer_msg, ((timers[i].timer.expires - jiffies)/HZ));
        }
    }
    seq_printf(m, "%s", kernel_out);
    // seq_printf(m, "It's a me, the CharDev!\n");
    return 0;
}

static int proc_open(struct inode *inode, struct file *filp) {
    cat = 1;
    // printk(KERN_INFO "proc module opened\n");
    return single_open(filp, mytimer_proc_show, NULL);
}

static int mytimer_open(struct inode *inode, struct file *filp) {
    cat = 1;
    // printk(KERN_INFO "chrdev module opened\n");
    return single_open(filp, mytimer_chrdev_show, NULL);
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

