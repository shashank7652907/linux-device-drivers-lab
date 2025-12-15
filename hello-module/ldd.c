#include <linux/init.h>
#include <linux/module.h>


MODULE_LICENSE("GPL");

MODULE_AUTHOR("Adepu Shashank");
MODULE_DESCRIPTION("Our first dynamically loadable kernel module");

static int hello_module_init(void){
    pr_info("Hello World from Module The Insertion is successful\n");
    return 0;

}

static void hello_module_exit(void){
    pr_info("Good bye from Module  The removal is successful");
}


module_init(hello_module_init);
module_exit(hello_module_exit);