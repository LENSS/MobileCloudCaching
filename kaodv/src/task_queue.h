/***************************************************************************
                          task_queue.h  -  description
                             -------------------
    begin                : Tue Jul 8 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "aodv.h"
#include "utils.h"
#include "aodv_thread.h"

task *get_task();
task *create_task(int type);
int insert_task(int type, struct sk_buff *packet);
int insert_task_from_timer(task * timer_task);
void init_task_queue();
void cleanup_task_queue();

#endif
