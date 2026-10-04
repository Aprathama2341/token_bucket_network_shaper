#ifndef TBQ_IOCTL_H
#define TBQ_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define TBQ_IOC_MAGIC 'T'

struct tbq_stats {
    __u64 queued_bytes;
    __u64 enqueued_packets;
    __u64 transmitted_packets;
    __u64 queue_drops;
    __u64 max_queue_bytes;
};

#define TBQ_IOC_DEQUEUE       _IOW(TBQ_IOC_MAGIC, 1, __u64)
#define TBQ_IOC_SET_MAX_QUEUE _IOW(TBQ_IOC_MAGIC, 2, __u64)
#define TBQ_IOC_GET_STATS     _IOR(TBQ_IOC_MAGIC, 3, struct tbq_stats)
#define TBQ_IOC_RESET         _IO(TBQ_IOC_MAGIC, 4)

#endif
