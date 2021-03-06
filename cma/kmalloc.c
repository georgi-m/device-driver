#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/slab.h>         /* Needed for kmalloc()*/
#include <linux/cdev.h>         /* char device stuff */
#include <linux/fs.h>           /* file stuff */
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/list.h>

typedef unsigned long long u64;

struct cma_alloc
{
        u64 virt_start_addr;
        u64 phys_start_addr;
        u64 buffer_size;
        bool log_control; // enable/disable kprit() messages during memory
                          // allocation from user space.
};

struct address_list
{
        void* ptr;
        struct list_head list;
};

LIST_HEAD(cma_address);

#define IOCTL_CMA_ALLOC _IOWR('A', 1, struct cma_alloc)
#define IOCTL_CMA_RELEASE _IO('A', 2)


#define DEVICE_NAME "cma_module"

static dev_t dev_num = 0; // Global variable for the device number
static struct class *cl;  // Global variable for the device class
static struct cdev c_dev; // Global variable for the character device structure
struct list_head *pos, *q;


static void release_memory(void)
{
        list_for_each_safe(pos, q, &cma_address) {
                struct address_list *obj = NULL;
                obj = list_entry(pos, struct address_list, list);
                printk(KERN_DEBUG "CMA-module: Release address %p\n", obj->ptr);
                kfree(obj->ptr);
                list_del(pos);
                kfree(obj);
        }
}

static long cma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        ssize_t ret = 0;
        static struct cma_alloc cma;
        struct address_list *address;
        address = kmalloc(sizeof(struct address_list), GFP_KERNEL);

        switch (cmd) {
                case IOCTL_CMA_ALLOC:
                        cma.log_control = false; // by default disabling kprint() function call.
                        ret = copy_from_user(&cma, (struct cma_alloc *)arg, sizeof(struct cma_alloc));
                        if (cma.log_control) {
                                printk(KERN_INFO "CMA-module: start copy from user\n");
                        }
                        if (0 != ret) {
                                printk("CMA-module: Error in copy from user");        
                                return -EFAULT;
                        }
                        if (cma.log_control) {
                                printk(KERN_INFO "CMA-module: end copy from user\n");
                                printk(KERN_INFO "CMA-module: buffer_size %llu\n", cma.buffer_size);
                        }
                        //TODO: need to print a warnning mesage in case of kmalloc fails and notify user
                        address->ptr = kmalloc(cma.buffer_size, GFP_KERNEL);
                        if (NULL == address->ptr) {
                                return -ENOMEM;
                        }
                        cma.virt_start_addr = (u64)address->ptr;
                        cma.phys_start_addr = virt_to_phys(address->ptr);
                        list_add(&address->list, &cma_address);
                        if (cma.log_control) {
                                printk(KERN_INFO "CMA-module: Physical address is %llx\n", cma.phys_start_addr);
                                printk(KERN_INFO "CMA-module: Virual address is %llx\n", cma.virt_start_addr);
                                printk(KERN_INFO "CMA-module: start copy to user\n");
                        }
                        ret = copy_to_user((struct cma_alloc *)arg, &cma, sizeof(struct cma_alloc));
                        if (0 != ret) {
                                printk("CMA-module: Error in copy to user\n");
                                return -EFAULT;
                        }
                        if (cma.log_control) {
                                printk(KERN_INFO "CMA-module: end copy to user\n");
                        }
                        break;
                 case IOCTL_CMA_RELEASE:
                        release_memory();
                        break;
                 default:
                        ret = -EINVAL;
                        break;
        }
        return ret;
}

static int cma_open(struct inode * in, struct file * file)
{
        printk(KERN_INFO "CMA-module: Opening %s device file\n", DEVICE_NAME);
        return 0;
}

static struct file_operations cma_module_fops = {
        .owner = THIS_MODULE,
        .open = cma_open,
        .unlocked_ioctl  = cma_ioctl,
};

static int create_cma_interface(void)
{
        int ret;
        struct device *dev_ret;
        printk(KERN_INFO "CMA-module: alloc_chrdev_region() is called.\n");
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
        printk(KERN_INFO "CMA-module: Initialization started\n");
        result = create_cma_interface();
        printk(KERN_INFO "CMA-module: KMALLOC_MAX_SIZE %lu\n", KMALLOC_MAX_SIZE);
        return result;
}

static void __exit cma_exit(void)
{
        cdev_del(&c_dev);
        device_destroy(cl, dev_num);
        class_destroy(cl);
        unregister_chrdev_region(dev_num, 1);
        release_memory();
        printk(KERN_INFO "CMA-module: Exiting\n");
}

module_init(cma_init);
module_exit(cma_exit);

MODULE_DESCRIPTION("Contiguous memory allocation");
MODULE_LICENSE("GPL");
