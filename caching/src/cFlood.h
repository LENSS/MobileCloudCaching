
#ifndef CFLOOD_H
#define CFLOOD_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <asm/div64.h>

#include "cache.h"
#include "cUtils.h"
#include "cTimer.h"

int init_flood_id_queue();
cFloodId *find_flood_id(u_int32_t src_ip, u_int32_t id);
void cleanup_flood_id_queue();
int insert_flood_id(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t id, u_int64_t lt);
int flush_flood_id_queue();

#endif
