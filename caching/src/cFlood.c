
#include "cFlood.h"

cFloodId *flood_id_queue;
rwlock_t flood_id_lock = RW_LOCK_UNLOCKED;
extern u_int32_t g_my_ip;

void flood_id_read_lock()
{
    read_lock_bh(&flood_id_lock);
}

void flood_id_read_unlock()
{
    read_unlock_bh(&flood_id_lock);
}

void flood_id_write_lock()
{
    write_lock_bh(&flood_id_lock);
}

void flood_id_write_unlock()
{
    write_unlock_bh(&flood_id_lock);
}

int init_flood_id_queue()
{
    flood_id_queue = NULL;
    return 0;
}

cFloodId *find_flood_id(u_int32_t src_ip, u_int32_t id)
{
    cFloodId *tmp_flood_id;     /* Working entry in the File Request list */

    u_int64_t curr = getcurrtime();

    /*lock table */
    flood_id_read_lock();

    tmp_flood_id = flood_id_queue;      /* Start at the header */

    //go through the whole queue
    while (tmp_flood_id != NULL)
    {
        if (time_before(tmp_flood_id->lifetime, curr))
        {
            /*unlock table */
            flood_id_read_unlock();
            return NULL;
        }
        //if there is a match and it is still valid
        if ((src_ip == tmp_flood_id->src_ip) && (id == tmp_flood_id->id))
        {
            /*unlock table */
            flood_id_read_unlock();
            return tmp_flood_id;
        }
        //continue on to the next entry
        tmp_flood_id = tmp_flood_id->next;
    }

    /*unlock table */
    flood_id_read_unlock();

    return NULL;
}

void cleanup_flood_id_queue()
{
    cFloodId *tmp_flood_id, *dead_flood_id;
    int count = 0;

    flood_id_write_lock();
    tmp_flood_id = flood_id_queue;

    while (tmp_flood_id != NULL)
    {
        dead_flood_id = tmp_flood_id;
        tmp_flood_id = tmp_flood_id->next;
        kfree(dead_flood_id);
        count++;
    }

    flood_id_queue = NULL;
    flood_id_write_unlock();

    printk(KERN_INFO "Removed %d Flood ID entries! \n", count);
    printk(KERN_INFO "---------------------------------------------\n");
}

int insert_flood_id(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t id, u_int64_t lt)
{
    cFloodId *new_flood_id;     /* Pointer to the working entry */

    /* Allocate memory for the new entry */
    if ((new_flood_id = (cFloodId *) kmalloc(sizeof(cFloodId), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Not enough memory to create Flood ID queue\n");

        /* Failed to allocate memory for new Flood ID entry */
        return -ENOMEM;
    }

    /* Fill in the information in the new entry */
    new_flood_id->src_ip = src_ip;
    new_flood_id->dst_ip = dst_ip;
    new_flood_id->id = id;
    new_flood_id->lifetime = lt;

    /*lock table */
    flood_id_write_lock();

    new_flood_id->next = flood_id_queue;
    /* Put the new entry in the list */
    flood_id_queue = new_flood_id;

    /*unlock table */
    flood_id_write_unlock();

    return 0;
}

int flush_flood_id_queue()
{
    cFloodId *tmp_flood_id, *prev_flood_id, *dead_flood_id;
    u_int64_t curr_time = getcurrtime();
    int id_count = 0;

    /*lock table */
    flood_id_write_lock();

    tmp_flood_id = flood_id_queue;
    prev_flood_id = NULL;

    //go through the entire queue
    while (tmp_flood_id)
    {
        //if the entry has expired
        if (time_before(tmp_flood_id->lifetime, curr_time))
        {
            //if it is the first entry
            if (prev_flood_id == NULL)
                flood_id_queue = tmp_flood_id->next;
            else
                prev_flood_id->next = tmp_flood_id->next;

            //kill it!
            dead_flood_id = tmp_flood_id;
            tmp_flood_id = tmp_flood_id->next;
            kfree(dead_flood_id);
            id_count++;
        }
        else
        {
            //next entry
            prev_flood_id = tmp_flood_id;
            tmp_flood_id = tmp_flood_id->next;
        }
    }

    /*unlock table */
    flood_id_write_unlock();
    
    insert_timer(TASK_CLEAN_UP, FLUSH_INTERVAL, g_my_ip, 0, 0);
    update_timer_queue();
    
    return id_count;
}


