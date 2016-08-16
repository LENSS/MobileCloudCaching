
#ifndef CFRAGREQ
#define CFRAGREQ

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>

#include "cache.h"
#include "cacheTable.h"
#include "cReqList.h"
#include "cExchange.h"
#include "cTimer.h"
#include "cSocket.h"

int recv_frreq_ori(cTask *tmp_task);
int recv_frreq_fwd(cTask *tmp_task);
int gen_frreq(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id);
int gen_frreq_by_task(cTask *tmp_task);

#endif
