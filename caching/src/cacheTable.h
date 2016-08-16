
#ifndef CACHETABLE_H
#define CACHETABLE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/dcache.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#include "cache.h"
#include "cUtils.h"

inline void ctable_read_lock();
inline void ctable_read_unlock();
void init_cache_table();
cacheTable *find_cache_by_fl(cacheTable *head, u_int32_t fileId);
cacheTable *find_cache_by_fr(u_int32_t fileId, u_int32_t fragId);
int update_ref(u_int32_t fileId, u_int32_t fragId);
void remove_cache(cacheTable *dead_cache);
int delete_cache(u_int32_t fileId, u_int32_t fragId);
int cleanup_cache_table();
cacheTable *create_cache(u_int32_t fileId, u_int32_t fragId, u_int32_t flag);
int read_ctable_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);

#endif
