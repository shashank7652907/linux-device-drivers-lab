#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/list.h>

#define MAX_NODES  10
#define NO_PROD    100
#define NO_CONS     5

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shashank");

/* ── linked list node ───────────────────────────────────────── */
struct node {
    int data;
    struct node *next;
};

static struct node *head = NULL;

/* ── sync primitives ────────────────────────────────────────── */
static struct mutex       lock;
static struct semaphore   sem_full;   /* how many items in queue   */
static struct semaphore   sem_empty;  /* how many free slots left  */

/* ── thread handles ─────────────────────────────────────────── */
static struct task_struct *prod_threads[NO_PROD];
static struct task_struct *cons_threads[NO_CONS];

/* ── counter (atomic to avoid race on i itself) ─────────────── */
static atomic_t job_counter = ATOMIC_INIT(0);

/* ── helpers ─────────────────────────────────────────────────── */
static struct node *create_node(int data)
{
    struct node *n = kmalloc(sizeof(*n), GFP_KERNEL);
    if (!n) {
        pr_err("kmalloc failed\n");
        return NULL;
    }
    n->data = data;
    n->next = NULL;
    return n;
}

static void add_node_at_start(void)
{
    int val = atomic_inc_return(&job_counter);
    struct node *n = create_node(val);
    if (!n) return;
    n->next = head;
    head = n;
    pr_info("Produced %d\n", val);
}

static void delete_node_at_start(void)
{
    struct node *temp;
    if (!head) {
        /* should never happen if semaphores are correct */
        pr_warn("Nothing to consume\n");
        return;
    }
    temp = head;
    pr_info("Consumed %d\n", temp->data);
    head = head->next;
    kfree(temp);
}

/* ── producer thread ─────────────────────────────────────────── */
static int producer_fn(void *arg)
{
    while (!kthread_should_stop()) {
        down(&sem_empty);               /* wait for a free slot   */

        if (kthread_should_stop()) {
            up(&sem_empty);             /* give back what we took */
            break;
        }

        mutex_lock(&lock);
        add_node_at_start();
        mutex_unlock(&lock);

        up(&sem_full);                  /* signal item available  */
        ssleep(1);
    }
    return 0;
}

/* ── consumer thread ─────────────────────────────────────────── */
static int consumer_fn(void *arg)
{
    while (!kthread_should_stop()) {
        down(&sem_full);                /* wait for an item       */

        if (kthread_should_stop()) {
            up(&sem_full);
            break;
        }

        mutex_lock(&lock);
        delete_node_at_start();
        mutex_unlock(&lock);

        up(&sem_empty);                 /* signal slot freed      */
        ssleep(3);
    }
    return 0;
}

/* ── module init ─────────────────────────────────────────────── */
static int __init pc_init(void)
{
    int i;
    char name[16];

    mutex_init(&lock);
    sema_init(&sem_full,  0);
    sema_init(&sem_empty, MAX_NODES);

    for (i = 0; i < NO_CONS; i++) {
        snprintf(name, sizeof(name), "consumer_%d", i);
        cons_threads[i] = kthread_run(consumer_fn, NULL, name);
        if (IS_ERR(cons_threads[i])) {
            pr_err("Failed to create consumer %d\n", i);
            cons_threads[i] = NULL;
        }
    }

    for (i = 0; i < NO_PROD; i++) {
        snprintf(name, sizeof(name), "producer_%d", i);
        prod_threads[i] = kthread_run(producer_fn, NULL, name);
        if (IS_ERR(prod_threads[i])) {
            pr_err("Failed to create producer %d\n", i);
            prod_threads[i] = NULL;
        }
    }

    pr_info("pc_module loaded\n");
    return 0;
}

/* ── module exit ─────────────────────────────────────────────── */
static void __exit pc_exit(void)
{
    int i;
    struct node *cur, *tmp;

    /* stop all threads */
    for (i = 0; i < NO_PROD; i++)
        if (prod_threads[i]) kthread_stop(prod_threads[i]);

    for (i = 0; i < NO_CONS; i++)
        if (cons_threads[i]) kthread_stop(cons_threads[i]);

    /* drain remaining nodes */
    cur = head;
    while (cur) {
        tmp = cur->next;
        kfree(cur);
        cur = tmp;
    }

    pr_info("pc_module unloaded\n");
}

module_init(pc_init);
module_exit(pc_exit);
