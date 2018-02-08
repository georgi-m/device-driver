#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/slab.h>         /* Needed for kmalloc()*/
#include <linux/cdev.h>         /* char device stuff */
#include <linux/fs.h>           /* file stuff */
#include <linux/device.h>

#define BUFFER_SIZE 4096
#define DEVICE_NAME "cma_module"

static dev_t dev_num = 0; // Global variable for the device number
static struct class *cl;  // Global variable for the device class
static struct cdev c_dev; // Global variable for the character device structure

static ssize_t cma_write(struct file *f, const char __user *buf, size_t len,
      loff_t *off)
{
    printk(KERN_INFO "Driver: write()\n");
      return len;
}


static struct file_operations cma_module_fops = {
        .owner = THIS_MODULE,
        .write = cma_write,
};

static int create_cma_interface(void)
{
        int ret;
        struct device *dev_ret;
        printk(KERN_INFO "CMA-module: register_device() is called.");
        ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
        if (ret < 0) {
                printk(KERN_WARNING "CMA-module: Unable to allocate character device region\n");
                return ret;
        }
        if (IS_ERR(cl = class_create(THIS_MODULE, DEVICE_NAME))) {
                unregister_chrdev_region(dev_num, 1);
                return PTR_ERR(cl);
        }
        if (IS_ERR(dev_ret = device_create(cl, NULL, dev_num, NULL, DEVICE_NAME))) {
                class_destroy(cl);
                unregister_chrdev_region(dev_num, 1);
                return PTR_ERR(dev_ret);
        }
        cdev_init(&c_dev, &cma_module_fops);
        ret = cdev_add(&c_dev, dev_num, 1);
        if (ret < 0) {
                device_destroy(cl, dev_num);
                class_destroy(cl);
                unregister_chrdev_region(dev_num, 1);
                return ret;
        }
        return 0;
}

static int __init cma_init(void)
{
        int result = 0;
        printk(KERN_INFO "CMA-module: Initialization started");
       // void * mem_ptr;
       // if ((mem_ptr = kmalloc(BUFFER_SIZE, GFP_KERNEL)) == NULL)
       //         return -ENOMEM;
       // printk(KERN_INFO "Virual address is %p", mem_ptr);
        result = create_cma_interface();
        return result;
}

static void __exit cma_exit(void)
{
        cdev_del(&c_dev);
        device_destroy(cl, dev_num);
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_INFO "CMA-module: Exiting");
}

module_init(cma_init);
module_exit(cma_exit);

MODULE_DESCRIPTION("Contiguous memory allocation");
MODULE_LICENSE("GPL");
