/***************************************************************************
                          aodv_dev.h  -  description
                             -------------------
    begin                : Thu Aug 7 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef AODV_DEV_H
#define AODV_DEV_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>

#include "aodv.h"
#include "aodv_route.h"
#include "socket.h"

//int insert_aodv_dev(struct net_device *dev);
int init_aodv_dev();
aodv_dev *first_aodv_dev();
aodv_dev *find_aodv_dev_by_dev(struct net_device *dev);


#endif
