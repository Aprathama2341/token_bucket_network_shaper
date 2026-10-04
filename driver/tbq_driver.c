// Virtual network queue driver for the Token-Bucket Bandwidth Shaper.
// Kernel-space component: tracks packet byte-counts and queue occupancy.
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include "tbq_ioctl.h"

#define DEVICE_NAME "tbqueue"

static DEFINE_MUTEX(tbq_lock);
static u64 queued_bytes;
static u64 enqueued_packets;
static u64 transmitted_packets;
static u64 queue_drops;
static u64 max_queue_bytes = 1024 * 1024;

static ssize_t tbq_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    u64 packet_bytes;

    if (count != sizeof(packet_bytes))
        return -EINVAL;
    if (copy_from_user(&packet_bytes, buf, sizeof(packet_bytes)))
        return -EFAULT;
    if (packet_bytes == 0)
        return -EINVAL;

    mutex_lock(&tbq_lock);
    if (packet_bytes > max_queue_bytes - queued_bytes) {
        queue_drops++;
        mutex_unlock(&tbq_lock);
        return -ENOSPC;
    }

    queued_bytes += packet_bytes;
    enqueued_packets++;
    mutex_unlock(&tbq_lock);
    return sizeof(packet_bytes);
}

static long tbq_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct tbq_stats stats;
    u64 bytes;

    switch (cmd) {
    case TBQ_IOC_DEQUEUE:
        if (copy_from_user(&bytes, (void __user *)arg, sizeof(bytes)))
            return -EFAULT;
        if (bytes == 0)
            return -EINVAL;

        mutex_lock(&tbq_lock);
        if (bytes > queued_bytes) {
            mutex_unlock(&tbq_lock);
            return -ENODATA;
        }
        queued_bytes -= bytes;
        transmitted_packets++;
        mutex_unlock(&tbq_lock);
        return 0;

    case TBQ_IOC_SET_MAX_QUEUE:
        if (copy_from_user(&bytes, (void __user *)arg, sizeof(bytes)))
            return -EFAULT;
        if (bytes == 0)
            return -EINVAL;
        mutex_lock(&tbq_lock);
        if (bytes < queued_bytes) {
            mutex_unlock(&tbq_lock);
            return -EBUSY;
        }
        max_queue_bytes = bytes;
        mutex_unlock(&tbq_lock);
        return 0;

    case TBQ_IOC_GET_STATS:
        mutex_lock(&tbq_lock);
        stats.queued_bytes = queued_bytes;
        stats.enqueued_packets = enqueued_packets;
        stats.transmitted_packets = transmitted_packets;
        stats.queue_drops = queue_drops;
        stats.max_queue_bytes = max_queue_bytes;
        mutex_unlock(&tbq_lock);
        if (copy_to_user((void __user *)arg, &stats, sizeof(stats)))
            return -EFAULT;
        return 0;

    case TBQ_IOC_RESET:
        mutex_lock(&tbq_lock);
        queued_bytes = 0;
        enqueued_packets = 0;
        transmitted_packets = 0;
        queue_drops = 0;
        mutex_unlock(&tbq_lock);
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations tbq_fops = {
    .owner = THIS_MODULE,
    .write = tbq_write,
    .unlocked_ioctl = tbq_ioctl,
    .llseek = no_llseek,
};

static struct miscdevice tbq_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &tbq_fops,
    .mode = 0666,
};

static int __init tbq_init(void)
{
    int ret = misc_register(&tbq_device);
    if (ret)
        pr_err("tbqueue: misc_register failed: %d\n", ret);
    else
        pr_info("tbqueue: virtual network queue driver loaded\n");
    return ret;
}

static void __exit tbq_exit(void)
{
    misc_deregister(&tbq_device);
    pr_info("tbqueue: virtual network queue driver unloaded\n");
}

module_init(tbq_init);
module_exit(tbq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Capstone Team");
MODULE_DESCRIPTION("Virtual network queue driver for token-bucket traffic shaping");
MODULE_VERSION("1.0");
