/***************************************************************************
                          rerr.h  -  description
                             -------------------
    begin                : Mon Aug 11 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef RERR_H
#define RERR_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>

#include "aodv.h"
#include "aodv_route.h"

int gen_rerr(u_int32_t brk_dst_ip);
int recv_rerr(task * tmp_packet);
#endif
