
#ifndef CFILEREP_H
#define CFILEREP_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kd.h>
#include <asm/io.h>
#include <linux/sched.h>

#include "cache.h"
#include "cTimer.h"
#include "cacheTable.h"
#include "cSocket.h"
#include "cFragReq.h"

int recv_flrep(cTask *tmp_task);
int gen_flrep(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t dist);
int process_flrep(cTask *tmp_task);
//int read_energy_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);
//int read_success_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);
void init_rinfo();
fileRepInfo *find_rinfo(fileRepInfo *head, u_int32_t fileId);
void flush_rinfo();
int cleanup_rinfo();
fileRepInfo *create_rinfo(cTask *tmp_task);
//int getDist(u_int32_t dst_ip);

#endif
