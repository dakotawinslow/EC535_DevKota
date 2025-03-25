#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devin Kot-Thompson/Dakota Winslow");
MODULE_DESCRIPTION("Traffic Light Driver");

#define MAJOR_NUM       61
#define MINOR_NUM        0
#define DEVICE_NAME     "mytraffic"
#define RED             67
#define GREEN           68
#define BLUE            44

static struct gpio_desc *red_led;
static struct gpio_desc *green_led;
static struct gpio_desc *blue_led;

static int mytraffic_init(void);
static void mytraffic_exit(void);
static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *user_buffer,
    size_t size, loff_t *offset);
static ssize_t my_write(struct file *file, const char __user *user_buffer,
    size_t size, loff_t *offset);

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release
};

module_init(mytraffic_init);
module_exit(mytraffic_exit);

static int mytraffic_init(void){
    int err;
    int red_status;
    int green_status;
    int blue_status;

    err = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

    printk(KERN_INFO "character device registration returned %d\n", err);

    red_led = gpio_to_desc(RED);
    if(!red_led){
        printk(KERN_ALERT "Unable to get descriptor for red\n");
        return -ENODEV;
    }

    red_status = gpiod_direction_output(red_led, 0);
    if(red_status) {
        printk(KERN_ALERT "Unable to set red as output");
        return -ENODEV;
    }

    green_led = gpio_to_desc(GREEN);
    if(!green_led){
        printk(KERN_ALERT "Unable to get descriptor for green\n");
        return -ENODEV;
    }

    green_status = gpiod_direction_output(green_led, 0);
    if(green_status) {
        printk(KERN_ALERT "Unable to set red as output");
        return -ENODEV;
    }

    blue_led = gpio_to_desc(BLUE);
    if(!blue_led){
        printk(KERN_ALERT "Unable to get descriptor for blue\n");
        return -ENODEV;
    }

    blue_status = gpiod_direction_output(blue_led, 0);
    if(blue_status) {
        printk(KERN_ALERT "Unable to set red as output");
        return -ENODEV;
    }

    return 0;
}

static void mytraffic_exit(void)
{
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_ALERT "Error in unregistering character device");
}

static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Character device opened, inode number: %lu, file pos: %lld\n", inode->i_ino, file->f_pos);
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Character device releasing, inode number: %lu, file pos: %lld\n", inode->i_ino, file->f_pos);
    return 0;
}

static ssize_t my_read(struct file *file, char __user *user_buffer,
    size_t size, loff_t *offset)
{
    /*Your module should set up a character device13 at /dev/mytraffic that returns the following information when read (e.g., using cat /dev/mytraffic):
    Current operational mode (i.e., “normal”, “flashing-red”, or “flashing-yellow”)
    Current cycle rate (e.g., “1 Hz”, “2 Hz”, etc.)
    Current status of each light (e.g., “red off, yellow off, green on”)
    Whether or not a pedestrian is “present” (i.e., currently crossing or waiting to cross after pressing the call button)
    This field is only required if your traffic light supports a pedestrian call button (see “Additional Features” below)
    */
   char test[] = "Hello World from read\n";

   ssize_t len = min((ssize_t)((sizeof(test)/sizeof(test[0])) - *offset), (ssize_t)size);
   
   if(len <= 0){
        return 0;
   }
    if(copy_to_user(user_buffer, test + *offset, len)){
        return -EFAULT;
    }

    *offset += len;

    return len;
}

static ssize_t my_write(struct file *file, const char __user *user_buffer,
    size_t size, loff_t *offset)
{
    char *input_data;
    ssize_t ret = size;

    if((input_data = kmalloc(size + 1, GFP_KERNEL)) == NULL){
        printk(KERN_ALERT "failed to allocate write buffer\n");
        return -ENOMEM;
    }

    if(copy_from_user(input_data, user_buffer, size)){
        printk(KERN_ALERT "failed to copy to write buffer\n");
        kfree(input_data);
        return -EFAULT;
    }

    input_data[size] = '\0';

    printk(KERN_INFO "Write buffer contatins %s\n", input_data);

    switch (input_data[0])
    {
    case  '0': 
        gpiod_set_value(red_led, 1);
        break;
    case  '1': 
        gpiod_set_value(green_led, 1);
        break;
    case  '2': 
        gpiod_set_value(blue_led, 1);
        break;
    
    default:
        gpiod_set_value(red_led, 0);
        gpiod_set_value(green_led, 0);
        gpiod_set_value(blue_led, 0);
        break;
    }

    kfree(input_data);

    return ret;
    
}