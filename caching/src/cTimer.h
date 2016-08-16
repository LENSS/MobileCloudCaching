
#ifndef CTIMER_H
#define CTIMER_H

#include <asm/div64.h>

#include "cache.h"
#include "cUtils.h"
#include "cTask.h"

int init_timer_queue();
void update_timer_queue();
int insert_timer(u_int8_t task_type, u_int32_t delay, u_int32_t ip, u_int32_t file_id, u_int32_t frag_id);
void delete_timer(u_int32_t id, u_int8_t type);

#endif
