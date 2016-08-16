
#ifndef CSOCKET_H
#define CSOCKET_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <net/inet_sock.h>

#include "cache.h"
#include "cUtils.h"

int init_sock(u_int32_t ip, char *dev_name);
int local_broadcast(u_int8_t ttl, void *data, const size_t datalen);
int send_message(u_int32_t dst_ip, u_int8_t ttl, void *data, const size_t datalen);

#endif
