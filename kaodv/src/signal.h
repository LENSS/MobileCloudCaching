/***************************************************************************
                          signal.h  -  description
                             -------------------
    begin                : Thu Aug 7 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef SIGNAL_H
#define SIGNAL_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <net/iw_handler.h>
#include <linux/wireless.h>

#include "aodv.h"
#include "aodv_dev.h"
#include "aodv_neigh.h"


//void init_iw_sock(void);
//void close_iw_sock(void);
int set_spy();
void get_wireless_stats();

#endif
