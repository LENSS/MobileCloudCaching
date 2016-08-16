/***************************************************************************
                          flood_id.h  -  description
                             -------------------
    begin                : Mon Aug 4 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef FLOOD_ID_QUEUE_H
#define FLOOD_ID_QUEUE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <asm/div64.h>

#include "aodv.h"
#include "utils.h"
#include "timer_queue.h"
#include "aodv_dev.h"

int read_flood_id_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);
int flush_flood_id_queue();
int init_flood_id_queue(void);
void cleanup_flood_id_queue();
int insert_flood_id(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t id, u_int64_t lt);
flood_id *find_flood_id(u_int32_t src_ip, u_int32_t id);


#endif
