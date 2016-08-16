/***************************************************************************
                          timer_queue.c  -  description
                             -------------------
    begin                : Mon Jul 14 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "timer_queue.h"


struct timer_list aodv_timer;
rwlock_t timer_lock = RW_LOCK_UNLOCKED;
task *timer_queue;
unsigned long flags;

inline void timer_read_lock()
{
    read_lock_irqsave(&timer_lock, flags);
}

inline void timer_read_unlock()
{
    read_unlock_irqrestore(&timer_lock, flags);
}

inline void timer_write_lock()
{
    write_lock_irqsave(&timer_lock, flags);
}

inline void timer_write_unlock()
{
    write_unlock_irqrestore(&timer_lock, flags);
}


int init_timer_queue()
{
    init_timer(&aodv_timer);
    timer_queue = NULL;
    return 0;
}


static unsigned long tvtojiffies(struct timeval *value)
{
    unsigned long sec = (unsigned) value->tv_sec;
    unsigned long usec = (unsigned) value->tv_usec;

    if (sec > (ULONG_MAX / HZ))
        return ULONG_MAX;
    usec += 1000000 / HZ - 1;
    usec /= 1000000 / HZ;
    return HZ * sec + usec;
}


task *first_timer_due(u_int64_t currtime)
{
    task *tmp_task;

    // lock Read
    timer_write_lock();
    if (timer_queue != NULL)
    {       
        /* If pqe's time is in teh interval */
        if (time_before_eq(timer_queue->time, currtime))
        {
	    tmp_task = timer_queue;
            timer_queue = timer_queue->next;
            timer_write_unlock();
            return tmp_task;
        }
    }

    /* unlock read */
    timer_write_unlock();
    return NULL;
}


void timer_queue_signal()
{
    task *tmp_task;
    u_int64_t currtime;

    // Get the first due entry in the queue /
    currtime = getcurrtime();
    tmp_task = first_timer_due(currtime);
		
    // While there is still events that has timed out
    while (tmp_task != NULL)
    {
        insert_task_from_timer(tmp_task);
        tmp_task = first_timer_due(currtime);
    }
    update_timer_queue();
}

void update_timer_queue()
{
    struct timeval delay_time;
    u_int64_t currtime;
    u_int64_t tv;
    u_int64_t remainder, numerator;

    delay_time.tv_sec = 0;
    delay_time.tv_usec = 0;

    /* lock Read */
    timer_read_lock();

    if (timer_queue == NULL)
    {
        // No event to set timer for
        delay_time.tv_sec = 0;
        delay_time.tv_usec = 0;
        del_timer(&aodv_timer);
        timer_read_unlock();
        return;
    }
    else
    {
        //* Get the first time value
        currtime = getcurrtime();
        
        if (time_before( timer_queue->time, currtime))
        {
            // If the event has allready happend, set the timeout to              1 microsecond :-)
            delay_time.tv_sec = 0;
            delay_time.tv_usec = 1;
        }
        else
        {
            // Set the timer to the actual seconds / microseconds from now

            //This is a fix for an error that occurs on ARM Linux Kernels because they do 64bits differently
            //Thanks to S. Peter Li for coming up with this fix!
 					
            numerator = (timer_queue->time - currtime);
            remainder = do_div(numerator, 1000);

            delay_time.tv_sec = numerator;
            delay_time.tv_usec = remainder * 1000;
        }
    }

    if (!timer_pending(&aodv_timer))
    {
        aodv_timer.function = &timer_queue_signal;
        aodv_timer.expires = jiffies + tvtojiffies(&delay_time);
        //printk("timer sched in %u sec and %u milisec delay %u\n",delay_time.tv_sec, delay_time.tv_usec,aodv_timer.expires);
 
        add_timer(&aodv_timer);
    }
    else
    {
        mod_timer(&aodv_timer, jiffies + tvtojiffies(&delay_time));
    }

    /* lock Read */
    timer_read_unlock();

    // Set the timer (in real time)
    return;
}


void queue_timer(task * new_timer)
{

    task *prev_timer = NULL;
    task *tmp_timer = NULL;
    u_int64_t currtime = getcurrtime();

    /* lock Write */
    timer_write_lock();

    tmp_timer = timer_queue;

    while (tmp_timer != NULL && (time_after(new_timer->time,tmp_timer->time)))
    {
    	//printk("%d is larger than %s type: %d time dif of %d \n", new_timer->type,inet_ntoa(tmp_timer->id),tmp_timer->type, new_timer->time-tmp_timer->time);
        prev_timer = tmp_timer;
        tmp_timer = tmp_timer->next;
    }

    if ((timer_queue == NULL) || (timer_queue == tmp_timer))
    {
        new_timer->next = timer_queue;
        timer_queue = new_timer;
    }
    else
    {
        if (tmp_timer == NULL)  // If at the end of the List
        {
            new_timer->next = NULL;
            prev_timer->next = new_timer;
        }
        else                  // Inserting in to the middle of the list somewhere
        {
            new_timer->next = prev_timer->next;
            prev_timer->next = new_timer;
        }
    }

    /* unlock Write */
    timer_write_unlock();
}

/****************************************************

   insert_timer_queue_entry
----------------------------------------------------
Insert an event into the queue. Also allocates
enough room for the data and copies that too
****************************************************/
int insert_timer(u_int8_t task_type, u_int32_t delay, u_int32_t ip)
{
    task *new_entry;

    new_entry = create_task(task_type);
    
    // get memory
    if (new_entry == NULL)
    {
        printk(KERN_WARNING "AODV: Error allocating timer!\n");
        return -ENOMEM;
    }
        
    new_entry->src_ip = ip;
    new_entry->dst_ip = ip;
    new_entry->id = ip;
   
    new_entry->time = getcurrtime() + delay;
    queue_timer(new_entry);
    return 0;
}

int insert_rreq_timer(rreq * tmp_rreq, u_int8_t retries)
{
    task *new_timer;
    // get memory
    if (!(new_timer = create_task(TASK_RESEND_RREQ)))
    {
        printk(KERN_WARNING "AODV: Error allocating timer!\n");
        return -ENOMEM;
    }

    new_timer->src_ip = tmp_rreq->src_ip;
    new_timer->dst_ip = tmp_rreq->dst_ip;
    new_timer->id = tmp_rreq->dst_ip;
    new_timer->retries = retries;
    new_timer->ttl = 30;
    new_timer->time = getcurrtime() + ((2 ^ (RREQ_RETRIES - retries)) * NET_TRAVERSAL_TIME);
    queue_timer(new_timer);
    return 0;
}


/****************************************************

   find_first_timer_qu2 ^ (RREQ_RETRIES - retries)eue_entry
----------------------------------------------------
Returns the first entry in the timer queue
****************************************************/
/*task *find_first_timer_queue_entry()
{
    return timer_queue;
}*/


/****************************************************

   find_first_timer_queue_entry_of_id
----------------------------------------------------
Returns the first timer queue entry with a matching
ID
****************************************************/
/*
struct timer_queue_entry * find_first_timer_queue_entry_of_id(u_int32_t id)
{
    struct timer_queue_entry *tmp_entry;

    // lock Read 
    timer_read_lock();


    tmp_entry=timer_queue;

    while (tmp_entry != NULL && tmp_entry->id != id)
        tmp_entry=tmp_entry->next;

    // unlock Read 
    timer_read_unlock();

    return tmp_entry;
}*/


task *find_timer(u_int32_t id, u_int8_t type)
{
    task *tmp_task;
    // lock Read
    
    timer_read_lock();
    tmp_task = timer_queue;
    while (tmp_task != NULL)
    {
        if ((tmp_task->id == id) && (tmp_task->type == type))
        {
            timer_read_unlock();
            return tmp_task;
        }
        tmp_task = tmp_task->next;
    }

    // unlock Read 
    timer_read_unlock();
    return NULL;
}

/****************************************************

   delete_timer_queue_entry_of_id
----------------------------------------------------
Deletes the first entry with a matching id
****************************************************/
void delete_timer(u_int32_t id, u_int8_t type)
{
    task *tmp_task;
    task *prev_task;
    task *dead_task;
    /* lock Write */

    //printk("deleting timer: %s  type: %u", inet_ntoa(id), type);
    timer_write_lock();
    tmp_task = timer_queue;
    prev_task = NULL;
    while (tmp_task != NULL)
    {
        if ((tmp_task->id == id) && (tmp_task->type == type))
        {
            if (prev_task == NULL)
            {
                timer_queue = tmp_task->next;
            }
            else
            {
                prev_task->next = tmp_task->next;
            }

            dead_task = tmp_task;
            tmp_task = tmp_task->next;
            kfree(dead_task->data);
            kfree(dead_task);
        }
        else
        {
            prev_task = tmp_task;
            tmp_task = tmp_task->next;
        }
    }

    /* unlock Write */
    timer_write_unlock();
    //update_timer_queue();
}

int read_timer_queue_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data)
{
    task *tmp_task;
    u_int64_t remainder, numerator;
    u_int64_t tmp_time;
    u_int64_t currtime = getcurrtime();
    int len;
    char tmp_buffer[200];

    /* lock Read */
    timer_read_lock();

    sprintf(buffer,"\nTimer Queue\n-------------------------------------------------------\n");
    strcat(buffer,"       ID      |     Type     |   sec/msec   |   Retries\n");
    strcat(buffer,"-------------------------------------------------------\n");
    tmp_task = timer_queue;

    while (tmp_task != NULL)
    {
	//This is a fix for an error that occurs on ARM Linux Kernels because they do 64bits differently
	//Thanks printk(" timer has %d left on it\n", timer_queue->time - currtime);
        // to S. Peter Li for coming up with this fix!
	sprintf( tmp_buffer,"    %-16s  ", inet_ntoa(tmp_task->dst_ip));
	strcat(buffer, tmp_buffer);

	switch (tmp_task->type)
        {
             case TASK_RESEND_RREQ:
                strcat(buffer, "RREQ    ");
                break;

             case TASK_CLEANUP:
                strcat(buffer, "Cleanup ");
                break;

             case TASK_HELLO:
                strcat(buffer, "Hello   ");
                break;

             case TASK_NEIGHBOR:
                strcat(buffer, "Neighbor");
                break;
	}

        tmp_time=tmp_task->time - currtime;

	numerator = (tmp_time);
	remainder = do_div( numerator, 1000 );  
	sprintf(tmp_buffer,"    %lu/%lu       %u\n", (unsigned long)numerator, (unsigned long)remainder, tmp_task->retries);
	strcat(buffer, tmp_buffer);
        tmp_task = tmp_task->next;
    }

    strcat(buffer,"-------------------------------------------------------\n");
  
    /* unlock Read */
    timer_read_unlock();

    len = strlen(buffer);
    if (len <= offset+buffer_length) *eof = 1;
    *buffer_location = buffer + offset;
    len -= offset;
    if (len>buffer_length) len = buffer_length;
    if (len<0) len = 0;
    return len;
}
