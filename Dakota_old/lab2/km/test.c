#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init my_module_init(void)
{
    printk(KERN_INFO "Hello, world!\n");
    return 0;
}

static void __exit my_module_exit(void)
{
    printk(KERN_INFO "Goodbye, world!\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dakota Winslow");
MODULE_DESCRIPTION("A simple Hello World kernel module");