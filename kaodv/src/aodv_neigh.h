/***************************************************************************
                          aodv_neigh.h  -  description
                             -------------------
    begin                : Thu Jul 31 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef AODV_NEIGH_H
#define AODV_NEIGH_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/netdevice.h>

#include "aodv.h"
#include "aodv_route.h"
#include "kernel_route.h"
#include "rerr.h"

//inline void neigh_read_unlock();
//void update_aodv_neigh_packets(u_int32_t target_ip);

//void update_aodv_neigh_packets_dropped(char *hw_addr);
void update_aodv_neigh_link(char *hw_addr, u_int8_t link);
void init_aodv_neigh_list();
aodv_neigh *find_aodv_neigh(u_int32_t target_ip);
int read_neigh_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);
int update_aodv_neigh( aodv_neigh *tmp_neigh, rrep *tmp_rrep);
//int flush_aodv_neigh();
aodv_neigh *first_aodv_neigh();
aodv_neigh *create_aodv_neigh(u_int32_t ip);
//rrep * create_valid_neigh_hello();
//int aodv_neigh_lost_confirmation(aodv_neigh *tmp_neigh);
//int aodv_neigh_got_confirmation(aodv_neigh *tmp_neigh);
int valid_aodv_neigh(u_int32_t target_ip);
int delete_aodv_neigh(u_int32_t ip);
#endif
