#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define CLASS_NAME "char"
#define DEVICE_FILE_NAME "char0" 
#define DEVICE_NAME "char_driver"

static dev_t dev;
static struct cdev c_dev;
char t;
static struct class* cl;
struct device* dev_ret;

static int char_module_open(struct inode* i, struct file* f){
    return 0;
}

static int char_module_release(struct inode* i, struct file* f){
    return 0;
}

static ssize_t char_module_read(struct file* f, char __user *buff, size_t len, loff_t* offset){
    int ret;
    if(*offset > 0){
        return 0;
    }
    ret = copy_to_user(buff, &t, 1);
    if(ret != 0){
        pr_info(DEVICE_NAME " copy_to_user failed\n");
        return -EFAULT;
    }
    pr_info(DEVICE_NAME " Last char copied is %c\n",t);
    *offset = 1;

    return 1;


}

static ssize_t char_module_write(struct file* f, const char __user *buff, size_t len, loff_t* offset){
    int ret;
    for(int i = 0; i < len; i++){
        ret = copy_from_user(&t, buff + i, 1);
        if(ret != 0){
            pr_info(DEVICE_NAME " copy_from_user failed at %d with status %d\n",i,ret);
            return  -EFAULT;
        }
        pr_info(DEVICE_NAME " char copied from user is %c\n",t);
        *offset += 1;
    }
    return len;
}

struct file_operations char_file_ops = {
    .open = char_module_open,
    .release = char_module_release,
    .write = char_module_write,
    .read = char_module_read,
    .llseek  = default_llseek,

};

static int __init char_module_init(void){
    int ret;
    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if(ret != 0){
        printk(KERN_INFO DEVICE_NAME "Alloc failed\n");
        return ret;
    }
    pr_info(DEVICE_NAME " is laoded\n");
    pr_info(DEVICE_NAME " Major No. : %d , Minor N0. : %d\n",MAJOR(dev), MINOR(dev));
    cdev_init(&c_dev, &char_file_ops);
    cdev_add(&c_dev, dev, 1);
    pr_info(DEVICE_NAME " Fops is registered\n");

    cl = class_create(CLASS_NAME);
    if (IS_ERR(cl))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(cl);
	}
    pr_info(DEVICE_NAME " Class is created \n");
    /*check in cat /sys/class/CLASS_NAME/DEVICE_FILE_NAME/dev*/
    /*we will see major minor number*/


    dev_ret = device_create(cl, NULL, dev, NULL, DEVICE_FILE_NAME);
    if (IS_ERR(dev_ret))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(dev_ret);
	}
    pr_info(DEVICE_NAME " Device is created\n");
    /*do ls /dev/DEVICE_FILE_NAME*/

    return ret;

}


static void __exit char_module_exit(void){

    device_destroy(cl, dev);
    pr_info(DEVICE_NAME " Device is destroyed\n");  

    class_destroy(cl);
    pr_info(DEVICE_NAME " Class is destroyed\n");

    cdev_del(&c_dev);
    pr_info(DEVICE_NAME " Fops is unregistered");

    unregister_chrdev_region(dev, 1);
    pr_info(DEVICE_NAME " is unlaoded\n");
}

module_init(char_module_init);
module_exit(char_module_exit);

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Adepu Shashank");
MODULE_DESCRIPTION("Character driver on my own");









