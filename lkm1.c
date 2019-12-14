#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

// open/close, read/write to a device
#include <linux/fs.h>

// copy_to_user, copy_from_user
#include <linux/uaccess.h>

// access cdev
//#include <linux/cdev.h>

#include <linux/moduleparam.h>

// saved in /proc/devices
#define DEVICE_NAME "lkm1"

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);

static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

// driver id
static int major;

// can be set by a write
static char kernel_buffer[256] = {0};
static short kernel_buffer_len;
static short open_cnt;

static int int_arg = 0;
static char *string_arg = "default_value";

// can be read and written
module_param(int_arg, int, S_IRUSR | S_IWUSR);
module_param(string_arg, charp, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(string_arg, "Default value");

static int __init lkm1_init(void) {
    printk(KERN_INFO "lkm1: %s - int_arg : %d, string_arg: %s", __FUNCTION__, int_arg, string_arg);

    // register character device
    // 0: allocate dynamic major number
    major = register_chrdev(0, DEVICE_NAME, &fops);

    if (major < 0) {
        printk(KERN_ALERT "lkm1: init - major : %d (< 0)\n", major);
        return major;
    }

    strcpy(kernel_buffer, string_arg);
    kernel_buffer_len = strlen(kernel_buffer);

    printk(KERN_INFO "lkm1: init - major : %d\n", major);
    return 0;
}

static void __exit lkm1_exit(void) {
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "lkm1: exit\n");
}

static int dev_open(struct inode *inodep, struct file *filep) {
    open_cnt++;
    printk(KERN_INFO "lkm1: open (cnt: %d)\n", open_cnt);
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "lkm1: release\n");
    return 0;
}

static ssize_t dev_write(struct file *filep, const char __user *user_buffer, size_t len, loff_t *offset) {
    int errors;
    // note: can't access the user_buffer here because it's in userspace
    // -> page fault, error_code(0x0001) - permissions violation
    printk(KERN_INFO "lkm1: write - saving %zu chars\n", len);

    // reset it, otherwise it will screw up during printk (old strings that are longer will still be printed)
    // but the \n is ugly as well, so let's just limit it inside printk
    //memset(kernel_buffer, 0, sizeof kernel_buffer);
    
    errors = copy_from_user(kernel_buffer, user_buffer, len);
    if (errors == 0) {
        kernel_buffer_len = len;
        printk(KERN_INFO "lkm1: write - copied %*.*s(%zu chars) from userspace\n", kernel_buffer_len-1, kernel_buffer_len-1, kernel_buffer, len);

        return len;
    }

    printk(KERN_ALERT "lkm1: write - failed to copy %zu chars from userspace (errors: %d)\n", len, errors);
    // return -EFAULT;
    return errors;
}

static ssize_t dev_read(struct file *filep, char __user *user_buffer, size_t len, loff_t *offset) {
    int errors = 0;
    int tmp_len = 0;

    printk(KERN_INFO "lkm1: read\n");

    // copy kernelspace buffer into the userspace buffer
    errors = copy_to_user(user_buffer, kernel_buffer, kernel_buffer_len);
    if (errors == 0) {
        printk(KERN_INFO "lkm1: read - copied %d characters to userspace\n", kernel_buffer_len);
        tmp_len = kernel_buffer_len;

        // clear kernel_buffer (returning 0 will stop the output)
        kernel_buffer_len = 0;
        
        return tmp_len;
    }

    printk(KERN_ALERT "lkm1: read - failed to copy %d characters to userspace\n", kernel_buffer_len);
    return -EFAULT;
}

module_init(lkm1_init);
module_exit(lkm1_exit);

MODULE_AUTHOR("me");
MODULE_DESCRIPTION("lkm1");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
