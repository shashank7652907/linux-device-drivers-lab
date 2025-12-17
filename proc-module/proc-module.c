#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>


MODULE_LICENSE("GPL");

MODULE_AUTHOR("Adepu Shashank");
MODULE_DESCRIPTION("Basic Read Loadable Kernel");

static struct proc_dir_entry *custom_proc_node;


static ssize_t proc_module_read(struct file* file_pointer,
                        char  *user_space_buffer,
                        size_t count,
                        loff_t* offset){
    char msg[] = "Ack!\n";
    size_t len = strlen(msg);
    int result;
    printk("proc_module_read\n");
    
    if(*offset >= len){
        return 0;
    }

    result = copy_to_user(user_space_buffer, msg, len);
    *offset += len;

    
    return len; /* for cat to print sufficient bytes*/
}


struct proc_ops driver_proc_ops = {
    .proc_read = proc_module_read

};

static int proc_module_init(void){
    printk("proc_module_init : entry \n ");
    custom_proc_node = proc_create("proc-module-driver",
                                    0,
                                    NULL,
                                    &driver_proc_ops);

    
    printk("proc_module_init : exit \n ");
    return 0;

}

static void proc_module_exit(void){
    printk("proc_module_exit : entry \n ");
    proc_remove(custom_proc_node);
    printk("proc_module_exit : exit \n ");
}


module_init(proc_module_init);
module_exit(proc_module_exit);
