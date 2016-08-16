
#ifndef CEXCHANGE_H
#define CEXCHANGE_H

#include "cache.h"
#include "cTask.h"
#include "cSocket.h"
#include "cacheTable.h"
#include "cTimer.h"

int recv_exreq(cTask *tmp_task);
int recv_exrep(cTask *tmp_task);
int recv_addcache(cTask *tmp_task);
int select_agent(cTask *tmp_task);
int gen_exchReq(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id);
int gen_exchRep(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t neigh_ip, u_int32_t file_id, u_int32_t frag_id, u_int32_t score);
u_int32_t getScore(u_int32_t file_id, u_int32_t frag_id);
int gen_addCache(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id);
int retrieve_frag(u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id);
//int read_prefetch_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);
void init_extable();
exchTable *find_extable(exchTable *head, u_int32_t fileId, u_int32_t fragId);
void flush_extable();
int cleanup_extable();
exchTable *create_extable(cTask *tmp_task);

#endif
