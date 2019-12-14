#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init lkm1_init(void) {
    printk(KERN_INFO "lkm_simple: init\n");
    return 0;
}

static void __exit lkm1_exit(void) {
    printk(KERN_INFO "lkm_simple: exit\n");
}

module_init(lkm1_init);
module_exit(lkm1_exit);

MODULE_AUTHOR("me");
MODULE_DESCRIPTION("lkm_simple");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
