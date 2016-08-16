/***************************************************************************
                          rrep.h  -  description
                             -------------------
    begin                : Wed Aug 6 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef RREP_H
#define RREP_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>

#include "aodv.h"
#include "aodv_route.h"
#include "utils.h"
#include "timer_queue.h"

int recv_rrep(task * tmp_packet);
int gen_rrep(u_int32_t src_ip, u_int32_t dst_ip, rreq *tmp_rreq);
#endif
