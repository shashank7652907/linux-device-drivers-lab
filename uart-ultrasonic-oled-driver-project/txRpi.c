#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/tty.h>

#define DRIVER          "ultrasonic_tx"
#define UART_DEV        "/dev/serial0"
#define TRIG_GPIO       535
#define ECHO_GPIO       536
#define DATA_READY_GPIO 537
#define SOUND_SPEED     34300

/* 
 * UART baud=115200 → ~86µs/byte. "DIST:177 TIME:1773383234\n" = 26 bytes
 * → ~2.3ms to fully clock out. Give 5ms margin.
 */
#define UART_DRAIN_MS   5
#define DATA_READY_US   100   /* wider pulse so RX doesn't miss it */

static int echo_irq_num;
static struct timer_list trig_timer;
static struct work_struct uart_work;
static struct file *uart_fp;
static char tx_buf[64];
static int tx_len;
static DEFINE_SPINLOCK(lock);

/*
 * FIX 1: protect echo_start with the same spinlock.
 * Both rising and falling edges run in the threaded IRQ context,
 * but on SMP they can run on different cores — spinlock is mandatory.
 */
static ktime_t echo_start;

static void uart_send_work(struct work_struct *w)
{
    char buf[64];
    int len;
    loff_t pos = 0;
    unsigned long flags;

    spin_lock_irqsave(&lock, flags);
    len = tx_len;
    memcpy(buf, tx_buf, len);
    spin_unlock_irqrestore(&lock, flags);

    kernel_write(uart_fp, buf, len, &pos);

    /*
     * FIX 2: kernel_write returns when data hits the FIFO, NOT when
     * it's clocked out on the wire. At 115200 baud, 26 bytes takes
     * ~2.3ms. Wait for drain before asserting DATA_READY.
     */
    msleep(UART_DRAIN_MS);

    gpio_set_value(DATA_READY_GPIO, 1);
    usleep_range(90, 110);          /* FIX 3: use usleep_range in non-atomic context */
    gpio_set_value(DATA_READY_GPIO, 0);

    pr_info("TX → %s", buf);
}

static irqreturn_t echo_thread(int irq, void *d)
{
    ktime_t now = ktime_get();
    int val = gpio_get_value(ECHO_GPIO);
    unsigned long flags;

    if (val) {
        /* FIX 4: protect echo_start write */
        spin_lock_irqsave(&lock, flags);
        echo_start = now;
        spin_unlock_irqrestore(&lock, flags);
    } else {
        s64 us;
        int dist;
        struct timespec64 ts;
        ktime_t start_snapshot;

        /* FIX 5: snapshot echo_start atomically before using it */
        spin_lock_irqsave(&lock, flags);
        start_snapshot = echo_start;
        spin_unlock_irqrestore(&lock, flags);

        us = ktime_to_us(ktime_sub(now, start_snapshot));

        /* Sanity check: HC-SR04 max range ~400cm → ~23ms echo */
        if (us < 100 || us > 25000) {
            pr_debug("TX: ignoring spurious echo %lld µs\n", us);
            return IRQ_HANDLED;
        }

        dist = (int)((us * SOUND_SPEED) / 2000000);
        ktime_get_real_ts64(&ts);

        spin_lock_irqsave(&lock, flags);
        tx_len = snprintf(tx_buf, sizeof(tx_buf),
                          "DIST:%d TIME:%lld\n",
                          dist, (long long)ts.tv_sec);
        spin_unlock_irqrestore(&lock, flags);

        schedule_work(&uart_work);
    }
    return IRQ_HANDLED;
}

static irqreturn_t echo_irq_handler(int irq, void *d)
{
    return IRQ_WAKE_THREAD;
}

static void trigger_pulse(void)
{
    gpio_set_value(TRIG_GPIO, 1);
    udelay(10);
    gpio_set_value(TRIG_GPIO, 0);
}

static void trig_timer_fn(struct timer_list *t)
{
    trigger_pulse();
    mod_timer(&trig_timer, jiffies + msecs_to_jiffies(1000));
}

static int __init tx_init(void)
{
    int ret;
    pr_info("TX driver loading\n");

    gpio_request(TRIG_GPIO, "trig");
    gpio_request(ECHO_GPIO, "echo");
    gpio_request(DATA_READY_GPIO, "ready");
    gpio_direction_output(TRIG_GPIO, 0);
    gpio_direction_input(ECHO_GPIO);
    gpio_direction_output(DATA_READY_GPIO, 0);

    echo_irq_num = gpio_to_irq(ECHO_GPIO);
    ret = request_threaded_irq(
            echo_irq_num,
            echo_irq_handler,
            echo_thread,
            IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
            DRIVER, NULL);
    if (ret)
        goto err_irq;

    uart_fp = filp_open(UART_DEV, O_WRONLY, 0);
    if (IS_ERR(uart_fp)) {
        ret = PTR_ERR(uart_fp);
        goto err_file;
    }

    INIT_WORK(&uart_work, uart_send_work);
    timer_setup(&trig_timer, trig_timer_fn, 0);
    mod_timer(&trig_timer, jiffies + msecs_to_jiffies(1000));

    pr_info("TX ready\n");
    return 0;

err_file:
    free_irq(echo_irq_num, NULL);
err_irq:
    gpio_free(TRIG_GPIO);
    gpio_free(ECHO_GPIO);
    gpio_free(DATA_READY_GPIO);
    return ret;
}

static void __exit tx_exit(void)
{
    del_timer_sync(&trig_timer);   /* FIX 6: del_timer_sync not del_timer */
    flush_work(&uart_work);
    free_irq(echo_irq_num, NULL);
    gpio_free(TRIG_GPIO);
    gpio_free(ECHO_GPIO);
    gpio_free(DATA_READY_GPIO);
    filp_close(uart_fp, NULL);
    pr_info("TX unloaded\n");
}

module_init(tx_init);
module_exit(tx_exit);
MODULE_LICENSE("GPL");
