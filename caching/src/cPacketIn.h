
#ifndef CPACKETIN_H
#define CPACKETIN_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/netfilter_ipv4.h>

#include "cache.h"
#include "cUtils.h"
#include "cReqList.h"
#include "cTask.h"

unsigned int input_handler(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn) (struct sk_buff *));
//int read_overhead_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);
int read_metric_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);

#endif
