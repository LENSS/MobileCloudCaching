
#ifndef CFILEREQ_H
#define CFILEREQ_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>

#include "cache.h"
#include "cUtils.h"
#include "cFlood.h"
#include "cFileRep.h"
#include "cSocket.h"

int recv_flreq(cTask *tmp_task);
int forward_flreq(fileReq *tmp_flreq, int ttl);

#endif
