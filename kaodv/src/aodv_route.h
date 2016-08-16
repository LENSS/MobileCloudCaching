/***************************************************************************
                          aodv_route.h  -  description
                             -------------------
    begin                : Mon Jul 29 2002
    copyright            : (C) 2002 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef AODV_ROUTE_H
#define AODV_ROUTE_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>

#include "aodv.h"
#include "utils.h"
#include "packet_queue.h"
#include "kernel_route.h"
#include "aodv_neigh.h"

inline void route_read_lock();
inline void route_read_unlock();

int read_route_table_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);

int init_aodv_route_table(void);
int cleanup_aodv_route_table();
void expire_aodv_route(aodv_route * tmp_route);
//int delete_aodv_route(u_int32_t target_ip);
int update_aodv_route(u_int32_t ip, u_int32_t next_hop_ip, u_int8_t metric, u_int32_t seq, struct net_device *dev);
int flush_aodv_route_table();
aodv_route *find_aodv_route(u_int32_t target_ip);
aodv_route *create_aodv_route(uint32_t ip);
aodv_route *first_aodv_route();
//int find_metric(u_int32_t tmp_ip);
void remove_aodv_route(aodv_route * dead_route);

#endif
