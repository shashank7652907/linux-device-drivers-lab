#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/string.h>

static dev_t dev_rev;

static char* Device_name = "Reverse Char";
static struct cdev cdev_rev;
static struct class* rev_class;
static struct device* rev_dev_file;

#define CLASS_NAME "rev_char0"
#define DEV_FILE_NAME "rev_char_file"

void reverse_portion(char* s,int left, int right){
    while(left < right){
        char temp = s[left];
        s[left] = s[right];
        s[right] = temp;

        left++;
        right--;
    }
    
}


void reverse_words_in_a_string(char* s, int len){
    reverse_portion(s, 0, len - 1);
    int start = 0;
    int end;
    for(int i = 0; i <= len; i++){
        if(s[i] == ' ' || s[i] == '\0'){
            end = i - 1;
            reverse_portion(s, start, end);
            start = i + 1;
        }
    }


}


static char s[100];

static int rev_open(struct inode* i, struct file* f){
    return 0;
}

static int rev_close(struct inode* i, struct file* f){
    return 0;
}

static ssize_t rev_write(struct file* f,const  char __user *buff, size_t len, loff_t* offset){
    unsigned long ret;
    ret = copy_from_user(s, buff, len);
    if(ret != 0){
        pr_info("%s : Copy from user failed\n",Device_name);
    }
    s[len] = '\0';
    if(len > 0 && s[len-1] == '\n') s[--len] = '\0';  
    reverse_words_in_a_string(s, len);
    pr_info("%s : buff copied from user is %s\n", Device_name, s);


    return len;
}

static ssize_t rev_read(struct file* f, char __user *buff, size_t len, loff_t* offset){
    unsigned long ret;
    int slen = strlen(s);
    
    if(*offset >= slen) return 0;
    
    ret = copy_to_user(buff, s, slen + 1);
    if(ret != 0){
        pr_info("%s : copy to user failed\n", Device_name);
        return -EFAULT;
    }
    *offset += slen + 1;
    return slen + 1;
}




const struct file_operations fops = {
    .open = rev_open,
    .release = rev_close,
    .write = rev_write,
    .read = rev_read,
};

static int __init module_init_func(void){
    int ret = alloc_chrdev_region(&dev_rev, 0, 1, Device_name);
    if(ret < 0){
        pr_info("%s : Device reg failed \n", Device_name);
        return ret;
    }
    pr_info("%s : Device resitered with Major no. : %d and Minor number : %d\n",Device_name, MAJOR(dev_rev), MINOR(dev_rev));
    cdev_init(&cdev_rev, &fops);
    ret =  cdev_add(&cdev_rev, dev_rev, 1);
    if(ret < 0){
        pr_info("%s : File ops reg failed\n",Device_name);
        return ret;
    }

    pr_info("%s : File ops reg success\n",Device_name);

    rev_class = class_create(CLASS_NAME);

    if (IS_ERR(rev_class))
	{
        pr_info("%s : Class create failed\n",Device_name);
        return 1;
    }

    pr_info("%s : Class create Success\n",Device_name);
    
    rev_dev_file = device_create(rev_class, NULL, dev_rev, NULL,  DEV_FILE_NAME);

    if(IS_ERR(rev_dev_file)){
        pr_info("%s : Dev File creation failure\n", Device_name);
        return 2;
    }

    pr_info("%s : Dev File creation success\n", Device_name);

    return 0;
}


static void __exit module_exit_func(void){
    device_destroy(rev_class, dev_rev);
    pr_info("%s : Dev File deletion success\n", Device_name);

    class_destroy(rev_class);
    pr_info("%s : Class deletion Success",Device_name);

    cdev_del(&cdev_rev);
    pr_info("%s : File ops deletion success\n",Device_name);

    unregister_chrdev_region(dev_rev, 1);
    pr_info("%s : Device unregistered \n", Device_name);
}

module_init(module_init_func);
module_exit(module_exit_func);

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Adepu Shashank");


