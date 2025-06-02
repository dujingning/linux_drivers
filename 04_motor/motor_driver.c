#include "asm-generic/gpio.h"
#include "asm/gpio.h"
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/delay.h>
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

struct gpio_desc{
	int gpio;
	int irq;
    char *name;
    int key;
	struct timer_list key_timer;
} ;


/* setup by self hardware */
static struct gpio_desc gpios[] = {
    {115, 0, "motor_gpio0", },
    {116, 0, "motor_gpio1", },
    {117, 0, "motor_gpio2", },
    {118, 0, "motor_gpio3", },
};

static int major = 0;
static struct class *gpio_class;

/* set motor pin */
static int g_motor_pin_ctrl[8]= {0x2,0x3,0x1,0x9,0x8,0xc,0x4,0x6};
static int g_motor_index = 0;

void set_pins_for_motor(int index)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		gpio_set_value(gpios[i].gpio, g_motor_pin_ctrl[index] & (1<<i) ? 1 : 0);
	}
}

void disable_motor(void)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		gpio_set_value(gpios[i].gpio, 0);
	}
}

/* int buf[2];
 * buf[0] = steps, > 0 : +direction; < 0 : -direction
 * buf[1] = mdelay time
 */
static ssize_t motor_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int ker_buf[2];
    int err;
    int step;

    if (size != 8)
        return -EINVAL;

    err = copy_from_user(ker_buf, buf, size);
    
    
	if (ker_buf[0] > 0) 
	{
		// +direction
		for (step = 0; step < ker_buf[0]; step++)
		{
			set_pins_for_motor(g_motor_index);
			mdelay(ker_buf[1]);
			g_motor_index--;
			if (g_motor_index == -1)
				g_motor_index = 7;
		}
	}
	else
	{
		// -direction
		ker_buf[0] = 0 - ker_buf[0];
		for (step = 0; step < ker_buf[0]; step++)
		{
			set_pins_for_motor(g_motor_index);
			mdelay(ker_buf[1]);
			g_motor_index++;
			if (g_motor_index == 8)
				g_motor_index = 0;
		}
	}

	/* save power */
	disable_motor();

    return 8;    
}


static struct file_operations gpio_key_drv = {
	.owner	 = THIS_MODULE,
	.write   = motor_write,
};

static int __init motor_init(void)
{
    int err;
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	for (i = 0; i < count; i++)
	{
		err = gpio_request(gpios[i].gpio, gpios[i].name);
		gpio_direction_output(gpios[i].gpio, 0);
	}

	major = register_chrdev(0, "gpio_key", &gpio_key_drv);  /* /dev/gpio_desc */

	gpio_class = class_create(THIS_MODULE, "gpio_key_class");
	if (IS_ERR(gpio_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "gpio_key");
		return PTR_ERR(gpio_class);
	}

	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "motor"); /* /dev/motor */
	
	return err;
}

static void __exit motor_exit(void)
{
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, "gpio_key");

	for (i = 0; i < count; i++)
	{
		gpio_free(gpios[i].gpio);
	}
}


module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");


