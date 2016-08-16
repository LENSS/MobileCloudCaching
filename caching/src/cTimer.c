
#include "cTimer.h"

struct timer_list cache_timer;
rwlock_t timer_lock = RW_LOCK_UNLOCKED;
cTask *timer_queue;
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
    init_timer(&cache_timer);
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

cTask *first_timer_due(u_int64_t currtime)
{
    cTask *tmp_task;

    // lock Read
    timer_write_lock();
    if (timer_queue != NULL)
    {       
        /* If pqe's time is in the interval */
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
    cTask *tmp_task;
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
        del_timer(&cache_timer);
        timer_read_unlock();
        return;
    }
    else
    {
        // Get the first time value
        currtime = getcurrtime();
        
        if (time_before( timer_queue->time, currtime))
        {
            // If the event has allready happend, set the timeout to 1 microsecond :-)
            delay_time.tv_sec = 0;
            delay_time.tv_usec = 1;
        }
        else
        {
            // Set the timer to the actual seconds / microseconds from now 					
            numerator = (timer_queue->time - currtime);
            remainder = do_div(numerator, 1000);

            delay_time.tv_sec = numerator;
            delay_time.tv_usec = remainder * 1000;
        }
    }

    if (!timer_pending(&cache_timer))
    {
        cache_timer.function = &timer_queue_signal;
        cache_timer.expires = jiffies + tvtojiffies(&delay_time); 
        add_timer(&cache_timer);
    }
    else
    {
        mod_timer(&cache_timer, jiffies + tvtojiffies(&delay_time));
    }

    /* lock Read */
    timer_read_unlock();

    // Set the timer (in real time)
    return;
}

void queue_timer(cTask * new_timer)
{
    cTask *prev_timer = NULL;
    cTask *tmp_timer = NULL;
    u_int64_t currtime = getcurrtime();

    /* lock Write */
    timer_write_lock();

    tmp_timer = timer_queue;

    while (tmp_timer != NULL && (time_after(new_timer->time,tmp_timer->time)))
    {
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
        if (tmp_timer == NULL) // If at the end of the List
        {
            new_timer->next = NULL;
            prev_timer->next = new_timer;
        }
        else // Inserting in to the middle of the list somewhere
        {
            new_timer->next = prev_timer->next;
            prev_timer->next = new_timer;
        }
    }

    /* unlock Write */
    timer_write_unlock();
}

int insert_timer(u_int8_t task_type, u_int32_t delay, u_int32_t ip, u_int32_t file_id, u_int32_t frag_id)
{
    cTask *new_entry;

    new_entry = create_task(task_type);
    
    // get memory
    if (new_entry == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Error allocating timer!\n");
        return -ENOMEM;
    }
        
    new_entry->src_ip = ip;
    new_entry->dst_ip = ip;
    new_entry->file_id = file_id;
    new_entry->frag_id = frag_id;
   
    new_entry->time = getcurrtime() + delay;
    queue_timer(new_entry);
    return 0;
}


