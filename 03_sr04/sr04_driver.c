#include "asm-generic/errno.h"
#include "asm-generic/gpio.h"
#include "asm/delay.h"
#include "linux/jiffies.h"
#include <linux/module.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>

#define CMD_TRIG  100

struct gpio_desc{
    int gpio;
    int irq;
    char *name;
    int key;
    struct timer_list key_timer;
};

static struct gpio_desc gpios[2] = {
    {115, 0, "trig", },
    {116, 0, "echo", },
};

static int major = 0;
static struct class *gpio_class;

#define BUF_LEN 128
static int g_keys[BUF_LEN];
static int r, w;

struct fasync_struct *button_fasync;

#define NEXT_POS(x) ((x+1) % BUF_LEN)

static int is_key_buf_empty(void)
{
	return (r == w);
}

static int is_key_buf_full(void)
{
	return (r == NEXT_POS(w));
}

static void put_key(int key)
{
	if (!is_key_buf_full())
	{
		g_keys[w] = key;
		w = NEXT_POS(w);
	}
}

static int get_key(void)
{
	int key = 0;
	if (!is_key_buf_empty())
	{
		key = g_keys[r];
		r = NEXT_POS(r);
	}
	return key;
}


static DECLARE_WAIT_QUEUE_HEAD(gpio_wait);

static ssize_t sr04_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	int key;

	if (is_key_buf_empty() && (file->f_flags & O_NONBLOCK))
		return -EAGAIN;


	wait_event_interruptible(gpio_wait, !is_key_buf_empty());
	key = get_key();

	if (key == -1)
		return -ENODATA;

	err = copy_to_user(buf, &key, 4);
	
	return 4;
}


static unsigned int sr04_poll(struct file *fp, poll_table * wait)
{
	poll_wait(fp, &gpio_wait, wait);
	return is_key_buf_empty() ? 0 : POLLIN | POLLRDNORM;
}

static int sr04_fasync(int fd, struct file *file, int on)
{
	if (fasync_helper(fd, file, on, &button_fasync) >= 0)
		return 0;
	else
		return -EIO;
}


static long sr04_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
	switch (command)
	{
		case CMD_TRIG:
		{
			gpio_set_value(gpios[0].gpio, 1);
			udelay(20);
			gpio_set_value(gpios[0].gpio, 0);

			mod_timer(&gpios[1].key_timer, jiffies + msecs_to_jiffies(50));  
		}
	}

	return 0;
}

static struct file_operations sr04_drv = {
	.owner	 = THIS_MODULE,
	.read    = sr04_read,
	.poll    = sr04_poll,
	.fasync  = sr04_fasync,
	.unlocked_ioctl = sr04_ioctl,
};


static irqreturn_t sr04_isr(int irq, void *dev_id)
{
	struct gpio_desc *gpio_desc = dev_id;
	int val;
	static u64 rising_time = 0;
	u64 time;

	val = gpio_get_value(gpio_desc->gpio);

	if (val)
	{
		rising_time = ktime_get_ns();
	}
	else
	{
		if (rising_time == 0)
		{
			return IRQ_HANDLED;
		}

		del_timer(&gpios[1].key_timer);

		time = ktime_get_ns() - rising_time;
		rising_time = 0;

		put_key(time);

		wake_up_interruptible(&gpio_wait);
		kill_fasync(&button_fasync, SIGIO, POLL_IN);
		
	}

	return IRQ_HANDLED;
}

static void sr04_timer_func(unsigned long data)
{
	put_key(-1);
	wake_up_interruptible(&gpio_wait);
	kill_fasync(&button_fasync, SIGIO, POLL_IN);
}

static int __init sr04_init(void)
{
    int err;
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	
	// trig pin
	err = gpio_request(gpios[0].gpio, gpios[0].name);
	gpio_direction_output(gpios[0].gpio, 0);

	// echo pin
	{		
		gpios[1].irq  = gpio_to_irq(gpios[1].gpio);

		err = request_irq(gpios[1].irq, sr04_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, gpios[1].name, &gpios[1]);

		setup_timer(&gpios[1].key_timer, sr04_timer_func, (unsigned long)&gpios[1]);
	}

	major = register_chrdev(0, "sr04_dev", &sr04_drv);  /* /dev/gpio_desc */

	gpio_class = class_create(THIS_MODULE, "sr04_class");
	if (IS_ERR(gpio_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "sr04_dev");
		return PTR_ERR(gpio_class);
	}

	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "sr04"); /* /dev/sr04 */
	
	return err;
}

static void __exit sr04_exit(void)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, "sr04_dev");

	// trig pin
	gpio_free(gpios[0].gpio);

	// echo pin
	{
		free_irq(gpios[1].irq, &gpios[1]);
		del_timer(&gpios[1].key_timer);
	}
}


module_init(sr04_init);
module_exit(sr04_exit);

MODULE_LICENSE("GPL");


