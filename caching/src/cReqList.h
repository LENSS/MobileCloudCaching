
#ifndef CREQLIST_H
#define CREQLIST_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>

#include "cache.h"

void init_req_list();
reqList *find_creq(u_int32_t fileId, u_int32_t fragId);
int reachThreshold(reqList *tmp_creq);
int update_count(reqList *tmp_creq);
int flush_count(reqList *tmp_creq);
int delete_creq(u_int32_t fileId, u_int32_t fragId);
int cleanup_req_list();
reqList *create_creq(u_int32_t fileId, u_int32_t fragId);
int read_creq_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);

#endif
