
#ifndef CTHREAD_H
#define CTHREAD_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>
#include <linux/socket.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <asm/uaccess.h>

#include "cache.h"
#include "cTask.h"
#include "cFileReq.h"
#include "cFileRep.h"
#include "cFragReq.h"
#include "cExchange.h"
#include "cFlood.h"
#include "cReqList.h"

void kill_cache();
void kick_cache();
void startup_cache();

#endif
