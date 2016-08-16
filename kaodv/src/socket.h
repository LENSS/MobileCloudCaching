/***************************************************************************
                          socket.h  -  description
                             -------------------
    begin                : Wed Aug 13 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef SOCKET_H
#define SOCKET_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <net/inet_sock.h>

#include "aodv.h"
#include "aodv_route.h"
#include "aodv_dev.h"

int init_sock(struct socket *sock, u_int32_t ip, char *dev_name);
void close_sock(void);
int local_broadcast(u_int8_t ttl, void *data, const size_t datalen);
int send_message(u_int32_t dst_ip, u_int8_t ttl, void *data, const size_t datalen, struct net_device *dev);



#endif
