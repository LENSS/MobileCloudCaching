/***************************************************************************
                          aodv_thread.h  -  description
                             -------------------
    begin                : Tue Aug 12 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef AODV_THREAD_H
#define AODV_THREAD_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>
#include <linux/socket.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <asm/uaccess.h>

#include "aodv.h"
#include "task_queue.h"
#include "aodv_neigh.h"
#include "rreq.h"
//#include "rrep_ack.h"
#include "rrep.h"
#include "rerr.h"
#include "aodv_route.h"
#include "flood_id.h"
#include "hello.h"
#include "signal.h"


void kill_aodv();
void kick_aodv();
void startup_aodv();

#endif
