/***************************************************************************
                          rreq.h  -  description
                             -------------------
    begin                : Mon Aug 4 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef RREQ_H
#define RREQ_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>

#include "aodv.h"
#include "aodv_route.h"
#include "utils.h"
#include "timer_queue.h"
#include "packet_queue.h"
#include "flood_id.h"

int recv_rreq(task * tmp_packet);
int gen_rreq(u_int32_t src_ip, u_int32_t dst_ip);
int resend_rreq(task * tmp_packet);



#endif
