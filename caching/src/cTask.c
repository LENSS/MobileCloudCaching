
#include "cTask.h"

spinlock_t task_queue_lock = SPIN_LOCK_UNLOCKED;
cTask *task_q;
cTask *task_end;

void queue_lock()
{
    spin_lock_bh(&task_queue_lock);
}

void queue_unlock()
{
    spin_unlock_bh(&task_queue_lock);
}

void init_task_queue()
{
    task_q=NULL;
    task_end=NULL;
}

void cleanup_task_queue()
{
    cTask *dead_task, *tmp_task;
    queue_lock();
    tmp_task = task_q;
    task_q=NULL;

    while(tmp_task)
    {
        dead_task = tmp_task;
        tmp_task = tmp_task->next;
        kfree(dead_task->data);
        kfree(dead_task);
    }
    queue_unlock();
}

cTask *create_task(int type)
{
    cTask *new_task;
    new_task = (cTask *) kmalloc(sizeof(cTask), GFP_ATOMIC);

    if (new_task == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Not enough memory to create cTask Entry\n");
        return NULL;
    }

    new_task->type = type;
    new_task->time = getcurrtime();
    new_task->src_ip = 0;
    new_task->dst_ip = 0;
    new_task->file_id = 0;
    new_task->frag_id = 0;
    new_task->ttl = 0;
    new_task->data_len = 0;
    new_task->data = NULL;
    
    new_task->next = NULL;
    new_task->prev = NULL;

    return new_task;
}

int queue_task(cTask * new_entry)
{
    /*lock table */
    queue_lock();

    //Set all the variables
    new_entry->next = task_q;
    new_entry->prev = NULL;
		
    if (task_q != NULL)
    {
	task_q->prev = new_entry; 
    }
 
    if (task_end == NULL)
    {
        task_end = new_entry; 
    }

    task_q = new_entry;
        
    //unlock table 
    queue_unlock();

    //wake up the caching thread
    kick_cache();

    return 0;
}

cTask *get_task()
{
    cTask *tmp_task = NULL;

    queue_lock();
    if (task_end)
    {
        tmp_task = task_end;
        if (task_end == task_q)
        {
            task_q = NULL;
            task_end = NULL;
        }
        else
        {
            task_end = task_end->prev;
        }
        queue_unlock();
        return tmp_task;
    }

    if (task_q != NULL)
    {
        printk(KERN_ERR "Cache Daemon: Error with cTask\n");
    }
    queue_unlock();
    return NULL;
}

int insert_task(int type, struct sk_buff *packet)
{
    cTask *new_task;
    struct iphdr *ip;

    int start_point = sizeof(struct udphdr) + sizeof(struct iphdr);

    new_task = create_task(type);

    if (!new_task)
    {
        printk(KERN_WARNING "Cache Daemon: Not enough memory to create cTask\n");
        return -ENOMEM;
    }

    if (type < 8)
    {
        ip = (struct iphdr *)skb_network_header(packet);
        new_task->src_ip = ip->saddr;
        new_task->dst_ip = ip->daddr;
	new_task->file_id = 0;
	new_task->frag_id = 0;
	new_task->ttl = ip->ttl;
        new_task->data_len = packet->len - start_point;

        //create space for the data and copy it there
        new_task->data = kmalloc(new_task->data_len, GFP_ATOMIC);
        if (!new_task->data)
        {
            kfree(new_task);
            printk(KERN_WARNING "Cache Daemon: Not enough memory to create cTask Data Entry\n");
            return -ENOMEM;
        }
        memcpy(new_task->data, packet->data + start_point, new_task->data_len);
    }

    queue_task(new_task);
}

int insert_task_from_timer(cTask * timer_task)
{
    if (!timer_task)
    {
        printk(KERN_WARNING "Cache Daemon: Passed a Null timer Task\n");
        return -ENOMEM;
    }
    queue_task(timer_task);
}


