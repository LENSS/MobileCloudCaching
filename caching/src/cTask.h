
#ifndef CTASK_H
#define CTASK_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "cache.h"
#include "cUtils.h"
#include "cThread.h"

cTask *get_task();
cTask *create_task(int type);
int insert_task(int type, struct sk_buff *packet);
int insert_task_from_timer(cTask *timer_task);
void init_task_queue();
void cleanup_task_queue();

#endif
