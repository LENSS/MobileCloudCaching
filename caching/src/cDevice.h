
#ifndef CDEVICE_H
#define CDEVICE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>

u_int32_t find_dev_ip(struct net_device * dev);

#endif
