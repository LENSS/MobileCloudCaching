/***************************************************************************
                          module.h  -  description
                             -------------------
    begin                : Wed Aug 20 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef MODULE_H
#define MODULE_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/wireless.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#include "aodv.h"
#include "packet_in.h"
#include "packet_out.h"
#include "task_queue.h"
#include "packet_queue.h"
#include "timer_queue.h"
#include "aodv_route.h"
#include "aodv_neigh.h"
#include "flood_id.h"
#include "aodv_thread.h"
#include "socket.h"
#include "signal.h"

#endif

//#define NF_IP_PRE_ROUTING       0
//#define NF_IP_POST_ROUTING      4
