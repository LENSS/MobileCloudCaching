
#ifndef FUTILS_H
#define FUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <asm/byteorder.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include "sfs.h"

int atoi(const char *nptr);
int get_local_ip(char *ip, char *dev_name);
int my_inet_aton(const char *cp, u_int32_t * addr);
int createFrag(char *file_id, char *frag_id);
int init_sock();
int broadcast(char *data, int datalen);
int gen_flreq(u_int32_t src_ip, u_int32_t file_id, u_int32_t seq);

#endif
