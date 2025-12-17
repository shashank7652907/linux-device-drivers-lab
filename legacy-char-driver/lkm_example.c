#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert W. Oliver II");
MODULE_DESCRIPTION("A simple legacy character device example");
MODULE_VERSION("0.01");

#define DEVICE_NAME "lkm_example"
#define EXAMPLE_MSG "Hello, World!\n"
#define MSG_BUFFER_LEN 15

/* Function prototypes */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int major_num;
static int device_open_count = 0;
static char msg_buffer[MSG_BUFFER_LEN];
static char *msg_ptr;

/* File operations structure */
static struct file_operations file_ops = {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

/* Read from device */
static ssize_t device_read(struct file *flip,
			   char *buffer,
			   size_t len,
			   loff_t *offset)
{
	if(*offset > 0){
        return 0;
    }
	int bytes_read = 0;



	/* Reset pointer when message ends */
	if (*msg_ptr == '\0')
		msg_ptr = msg_buffer;

	

	while (len && *msg_ptr) {
		if (put_user(*(msg_ptr++), buffer++))
			return -EFAULT;
		len--;
		bytes_read++;
	}


	*offset += bytes_read;
	return bytes_read;
}

/* Write to device (not supported) */
static ssize_t device_write(struct file *flip,
			    const char *buffer,
			    size_t len,
			    loff_t *offset)
{
	printk(KERN_ALERT "lkm_example: write operation not supported\n");
	return -EINVAL;
}

/* Open device */
static int device_open(struct inode *inode, struct file *file)
{
	if (device_open_count)
		return -EBUSY;

	device_open_count++;
	try_module_get(THIS_MODULE);
	return 0;
}

/* Close device */
static int device_release(struct inode *inode, struct file *file)
{
	device_open_count--;
	module_put(THIS_MODULE);
	return 0;
}

/* Module init */
static int __init lkm_example_init(void)
{
	strncpy(msg_buffer, EXAMPLE_MSG, MSG_BUFFER_LEN);
	msg_ptr = msg_buffer;

	major_num = register_chrdev(0, DEVICE_NAME, &file_ops);
	if (major_num < 0) {
		printk(KERN_ALERT "lkm_example: failed to register device\n");
		return major_num;
	}

	printk(KERN_INFO "lkm_example loaded with major number %d\n", major_num);
	return 0;
}

/* Module exit */
static void __exit lkm_example_exit(void)
{
	unregister_chrdev(major_num, DEVICE_NAME);
	printk(KERN_INFO "lkm_example unloaded\n");
}

module_init(lkm_example_init);
module_exit(lkm_example_exit);
