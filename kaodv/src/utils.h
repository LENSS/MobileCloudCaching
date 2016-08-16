/***************************************************************************
                          utils.h  -  description
                             -------------------
    begin                : Wed Jul 30 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef UTILS_H
#define UTILS_H

#include <asm/byteorder.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/ctype.h>
#include <asm/div64.h>
#include "aodv.h"
#include "aodv_dev.h"


u_int64_t getcurrtime();
char *inet_ntoa(u_int32_t ina);
int inet_aton(const char *cp, u_int32_t * addr);
//u_int32_t calculate_netmask(int t);
//int calculate_prefix(u_int32_t t);
int seq_greater(u_int32_t seq_one,u_int32_t seq_two);
int seq_less_or_equal(u_int32_t seq_one,u_int32_t seq_two);
int aodv_subnet_test(u_int32_t tmp_ip);


#endif
