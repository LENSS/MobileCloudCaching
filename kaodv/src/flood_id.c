/***************************************************************************
                          flood_id.c  -  description
                             -------------------
    begin                : Mon Aug 4 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "flood_id.h"
/****************************************************

   flood_id_queue
----------------------------------------------------
This is used to keep track of messages which
are flooded to prevent rebroadcast of messages
****************************************************/

flood_id *flood_id_queue;
rwlock_t flood_id_lock = RW_LOCK_UNLOCKED;
extern u_int32_t g_my_ip;

/****************************************************

   init_flood_id_queue
----------------------------------------------------
Gets the ball rolling!
****************************************************/

int init_flood_id_queue(void)
{
    flood_id_queue = NULL;
    return 0;
}

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


/****************************************************

   find_flood_id_queue_entry
----------------------------------------------------
will search the queue for an entry with the
matching ID and src_ip
****************************************************/
flood_id *find_flood_id(u_int32_t src_ip, u_int32_t id)
{
    flood_id *tmp_flood_id;     /* Working entry in the RREQ list */

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


/****************************************************

   read_flood_id_proc
----------------------------------------------------
prints out the flood id queue when the proc
file is read
****************************************************/
int read_flood_id_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data)
{
    int len;
    static char *my_buffer;
    char temp_buffer[200];
    flood_id *tmp_entry;
    char temp_ip[16];
    u_int64_t remainder, numerator;
    u_int64_t tmp_time, currtime;
  
    //lock table
    flood_id_read_lock();

    tmp_entry=flood_id_queue;

    my_buffer=buffer;
    currtime = getcurrtime();
  
    sprintf(my_buffer,"\nFlood Id Queue\n---------------------------------\n");

    while (tmp_entry!=NULL)
    {
        strcpy(temp_ip,inet_ntoa(tmp_entry->dst_ip));
        sprintf(temp_buffer,"Src IP: %-16s  Dst IP: %-16s Flood ID: %-10u ", inet_ntoa(tmp_entry->src_ip),temp_ip,tmp_entry->id);
        strcat(my_buffer,temp_buffer);

	tmp_time=tmp_entry->lifetime - currtime;
	numerator = (tmp_time);
	remainder = do_div( numerator, 1000 );
	if (time_before(tmp_entry->lifetime, currtime) )
	{
	    sprintf(temp_buffer," Expired!\n");
	}
	else
	{
            sprintf(temp_buffer," sec/msec: %lu/%lu \n", (unsigned long)numerator, (unsigned long)remainder);
	}
        strcat(my_buffer,temp_buffer);

        tmp_entry=tmp_entry->next;
    }

    //unlock table
    flood_id_read_unlock();

    sprintf(temp_buffer,"\n---------------------------------\n");
    strcat(my_buffer,temp_buffer);

    *buffer_location=my_buffer;
    len = strlen(my_buffer);
    if (len <= offset+buffer_length) *eof = 1;
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len>buffer_length) len = buffer_length;
    if (len<0) len = 0;

    return len;

}




/****************************************************

   clean_up_flood_id_queue
----------------------------------------------------
Deletes everything in the flood id queue
****************************************************/
void cleanup_flood_id_queue()
{
    flood_id *tmp_flood_id, *dead_flood_id;
    int count = 0;

    flood_id_write_lock();
    tmp_flood_id = flood_id_queue;

    //print_flood_id_queue();

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

/****************************************************

   insert_flood_id_queue_entry
----------------------------------------------------
Inserts an entry into the flood ID queue
****************************************************/
int insert_flood_id(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t id, u_int64_t lt)
{
    flood_id *new_flood_id;     /* Pointer to the working entry */

    /* Allocate memory for the new entry */
    if ((new_flood_id = (flood_id *) kmalloc(sizeof(flood_id), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Not enough memory to create Flood ID queue\n");

        /* Failed to allocate memory for new Flood ID queue */
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
    flood_id *tmp_flood_id, *prev_flood_id, *dead_flood_id;
    u_int64_t curr_time = getcurrtime();        /* Current time */
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
    
    insert_timer( TASK_CLEANUP, HELLO_INTERVAL, g_my_ip);
    update_timer_queue();
    
    return id_count;
}
