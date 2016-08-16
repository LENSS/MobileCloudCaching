/***************************************************************************
                          kernel_route.h  -  description
                             -------------------
    begin                : Fri Aug 8 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef KERNEL_ROUTE_H
#define KERNEL_ROUTE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <net/route.h>
#include <linux/socket.h>
#include <linux/in.h>

#include "aodv.h"
#include "utils.h"

int insert_kernel_route_entry(u_int32_t dst_ip, u_int32_t gw_ip, u_int32_t genmask_ip, char *interf);
int delete_kernel_route_entry(u_int32_t dst_ip, u_int32_t gw_ip, u_int32_t genmask_ip);


#endif
