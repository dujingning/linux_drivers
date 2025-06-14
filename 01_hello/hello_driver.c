#include <linux/backing-dev.h>
#include <linux/capability.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/pfn.h>
#include <linux/ptrace.h>
#include <linux/random.h>
#include <linux/raw.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/splice.h>
#include <linux/tty.h>
#include <linux/uio.h>
#include <linux/vmalloc.h>

#include <linux/uaccess.h>

static int major;
static char hello_data[50];

static int hello_open(struct inode *node, struct file *filp) {
  printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  return 0;
}
static ssize_t hello_read(struct file *filp, char __user *buf, size_t size,
                          loff_t *offset) {
  unsigned long ret = 0;
  printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  ret = copy_to_user(buf, hello_data, size);
  if (ret != 0) {
    return size - ret;
  }
  return size;
}

static ssize_t hello_write(struct file *filp, const char __user *buf,
                           size_t size, loff_t *offset) {
  unsigned long ret = 0;
  printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  ret = copy_from_user(hello_data, buf, size);
  if (ret != 0) {
    return size - ret;
  }
  return size;
}

static int hello_release(struct inode *node, struct file *filp) {
  printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  return 0;
}

/* 1. create file_operations */
static const struct file_operations hello_drv = {
    .owner = THIS_MODULE,
    .read = hello_read,
    .write = hello_write,
    .open = hello_open,
    .release = hello_release,
};

/* 2. register_chrdev */
/* 3. entry function */
static int hello_init(void) {
  major = register_chrdev(0, "hello", &hello_drv);
  return 0;
}

/* 4. exit function */
static void hello_exit(void) { unregister_chrdev(major, "hello"); }

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
