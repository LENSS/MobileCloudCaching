/***************************************************************************
                          timer_queue.h  -  description
                             -------------------
    begin                : Mon Jul 14 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include <asm/div64.h>

#include "aodv.h"
#include "task_queue.h"

//void timer_queue_signal();
int init_timer_queue();
void update_timer_queue();
task *find_timer(u_int32_t id, u_int8_t flags);
int read_timer_queue_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data);
//int resend_rreq(task * tmp_packet);
int insert_timer(u_int8_t task_type, u_int32_t delay, u_int32_t ip);
void delete_timer(u_int32_t id, u_int8_t type);
int insert_rreq_timer(rreq * tmp_rreq, u_int8_t retries);
#endif
