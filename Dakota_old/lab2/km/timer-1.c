#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>

static struct timer_list my_timer;

void my_timer_callback(struct timer_list *t)
{
    printk(KERN_INFO "Timer expired!\n");
}

static int __init my_module_init(void)
{
    printk(KERN_INFO "Loading module and setting up timer...\n");

    // Initialize the timer
    timer_setup(&my_timer, my_timer_callback, 0);

    // Set the timer to expire after 5 seconds (jiffies are system time units)
    mod_timer(&my_timer, jiffies + 5 * HZ);

    return 0;
}

static void __exit my_module_exit(void)
{
    printk(KERN_INFO "Unloading module and deleting timer...\n");
    del_timer(&my_timer);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dakota Winslow");
MODULE_DESCRIPTION("Basic kernel timer module. c2025");