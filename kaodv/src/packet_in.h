/***************************************************************************
                          packet_in.h  -  description
                             -------------------
    begin                : Mon Jul 29 2002
    copyright            : (C) 2002 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef PACKET_IN_H
#define PACKET_IN_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/netfilter_ipv4.h>


#include "aodv.h"
#include "aodv_route.h"
unsigned int input_handler(unsigned int hooknum, struct sk_buff *skb,
                           const struct net_device *in, const struct net_device *out, int (*okfn) (struct sk_buff *));
int read_overhead_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);


#endif
