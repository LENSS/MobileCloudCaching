
/***************************************************************************
                          aodv_route.c  -  description
                             -------------------
    begin                : Mon Jul 29 2002
    copyright            : (C) 2002 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "aodv_route.h"

aodv_route *aodv_route_table;
extern u_int32_t g_broadcast_ip;
rwlock_t route_lock = RW_LOCK_UNLOCKED;


inline void route_read_lock()
{
    read_lock_bh(&route_lock);
}

inline void route_read_unlock()
{
    read_unlock_bh(&route_lock);
}

inline void route_write_lock()
{
    write_lock_bh(&route_lock);
}

inline void route_write_unlock()
{	
    write_unlock_bh(&route_lock);
}

aodv_route *first_aodv_route()
{
    return aodv_route_table;
}

int valid_aodv_route(aodv_route * tmp_route)
{
    if (time_after(tmp_route->lifetime,  getcurrtime()) && tmp_route->route_valid)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int init_aodv_route_table(void)
{
    aodv_route *tmp_route;
    u_int32_t		lo;
    aodv_route_table = NULL;

    inet_aton("127.0.0.1",&lo);
    tmp_route = create_aodv_route(lo);
    tmp_route->next_hop = lo;
    tmp_route->metric = 0;
    tmp_route->self_route = TRUE;
    tmp_route->route_valid = TRUE;
    tmp_route->seq = 0;
    return 0;
}

void expire_aodv_route(aodv_route * tmp_route)
{
    //marks a route as expired
    tmp_route->lifetime = (getcurrtime() + DELETE_PERIOD);
    tmp_route->seq++;
    tmp_route->route_valid = FALSE;
}

void remove_aodv_route(aodv_route * dead_route)
{
    route_write_lock();

    if (aodv_route_table == dead_route)
    {
        aodv_route_table = dead_route->next;
    }
    if (dead_route->prev != NULL)
    {
        dead_route->prev->next = dead_route->next;
    }
    if (dead_route->next!=NULL)
    {
        dead_route->next->prev = dead_route->prev;
    }

    route_write_unlock();

    kfree(dead_route);
}


int cleanup_aodv_route_table()
{
    aodv_route *dead_route, *tmp_route;
    route_write_lock();

    tmp_route = aodv_route_table;
    while (tmp_route!=NULL) 
    {
        delete_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask);
	dead_route = tmp_route;
        tmp_route = tmp_route->next;
	kfree(dead_route);
    }
    aodv_route_table = NULL;
    route_write_unlock();
    return 0;
}

int flush_aodv_route_table()
{
    u_int64_t currtime = getcurrtime();
    aodv_route *dead_route, *tmp_route, *prev_route=NULL;
    route_read_lock();

    tmp_route = aodv_route_table;
    while (tmp_route!=NULL)
    {
        if (prev_route != tmp_route->prev)
	{
	    printk("AODV: Routing table error! %s prev is wrong!\n",inet_ntoa(tmp_route->ip));
	}
   	prev_route = tmp_route;
 
	if (time_before(tmp_route->lifetime, currtime) && (!tmp_route->self_route))
        {
            //printk("looking at route: %s\n",inet_ntoa(tmp_route->ip));
  	    if (tmp_route->route_valid )
            {
                expire_aodv_route(tmp_route);
                tmp_route = tmp_route->next;
            }
            else
            {
		delete_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask);
                dead_route = tmp_route;
                prev_route = tmp_route->prev;
                tmp_route = tmp_route->next;
		route_read_unlock();
                remove_aodv_route(dead_route);
                route_read_lock();
            }
     	}
        else
        {
            tmp_route = tmp_route->next;
        }
    }

    route_read_unlock();
    return 0;
}


int delete_aodv_route(u_int32_t target_ip)
{
    aodv_route *dead_route;
    route_read_lock();

    dead_route = aodv_route_table;
    while ((dead_route != NULL) && (dead_route->ip <= target_ip))
    {
        if (dead_route->ip == target_ip)
        {
            route_read_unlock();
            remove_aodv_route(dead_route);
            return 1;
        }
        dead_route = dead_route->next;
    }
    route_read_unlock();
    return 0;
}




void insert_aodv_route(aodv_route * new_route)
{
    aodv_route *tmp_route;
    aodv_route *prev_route = NULL;

    route_write_lock();

    tmp_route = aodv_route_table;

    while ((tmp_route != NULL) && (tmp_route->ip < new_route->ip))
    {
        prev_route = tmp_route;
        tmp_route = tmp_route->next;
    }

    if (aodv_route_table && (tmp_route == aodv_route_table))   // if it goes in the first spot in the table
    {
        aodv_route_table->prev = new_route;
        new_route->next = aodv_route_table;
        aodv_route_table = new_route;
    } 

    if (aodv_route_table == NULL)       // if the routing table is empty
    {
        aodv_route_table = new_route;
    }

    if (prev_route!=NULL)
    {
        if (prev_route->next)
        {
	    prev_route->next->prev = new_route;
	}
	      
	new_route->next = prev_route->next;
        new_route->prev = prev_route;
        prev_route->next = new_route;

    }

    route_write_unlock();
    return;
}


aodv_route *create_aodv_route(uint32_t ip)
{
    aodv_route *tmp_entry;

    /* Allocate memory for new entry */

    if ((tmp_entry = (aodv_route *) kmalloc(sizeof(aodv_route), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Error getting memory for new route\n");
        return NULL;
    }

    tmp_entry->self_route = FALSE;
    tmp_entry->rreq_id = 0;
    tmp_entry->metric = 0;
    tmp_entry->seq = 0;
    tmp_entry->ip = ip;
    tmp_entry->next_hop = ip;
    tmp_entry->dev = NULL;
    tmp_entry->route_valid = FALSE;
    tmp_entry->netmask = g_broadcast_ip;
    tmp_entry->route_seq_valid = FALSE;
    tmp_entry->prev = NULL;
    tmp_entry->next = NULL;

    if (ip)
    {
        insert_aodv_route(tmp_entry);
    }

    return tmp_entry;
}

int update_aodv_route(u_int32_t ip, u_int32_t next_hop_ip, u_int8_t metric, u_int32_t seq, struct net_device *dev)
{
    aodv_route *tmp_route;
    u_int64_t curr_time;

    /*lock table */

    tmp_route = find_aodv_route(ip);    /* Get eprev_route->next->prev = new_route;ntry from RT if there is one */

    if (!valid_aodv_neigh(next_hop_ip))
    {
        printk(KERN_INFO "AODV: Failed to update route: %s \n", inet_ntoa(ip));
        return 0;
    }

    if (tmp_route && seq_greater(tmp_route->seq, seq))
    {
        return 0;
    }
    
    if (tmp_route && tmp_route->route_valid)
    {
        if ((seq == tmp_route->seq) && (metric >= tmp_route->metric))
        {
            return 0;
        }
    }
    
    if (tmp_route == NULL)
    {
        tmp_route = create_aodv_route(ip);

        if (tmp_route == NULL)
            return -ENOMEM;
        tmp_route->ip = ip;
    } 

    if (tmp_route->self_route)
    {
        printk("updating a SELF-ROUTE!!! %s hopcount %d\n", inet_ntoa(next_hop_ip), metric);
        if (!tmp_route->route_valid)
            printk("because route was invalid!\n");

        if (!tmp_route->route_seq_valid)
            printk("because seq was invalid!\n");

        if (seq_greater(seq, tmp_route->seq))
            printk("because seq of route was lower!\n");

        if ((seq == tmp_route->seq) && (metric < tmp_route->metric))
            printk("becase seq same but hop lower!\n");
    }

    /* Update values in the RT entry */
    tmp_route->seq = seq;
    tmp_route->next_hop = next_hop_ip;
    tmp_route->metric = metric;
    tmp_route->dev =   dev;
    tmp_route->route_valid = TRUE;
    tmp_route->route_seq_valid = TRUE;
    delete_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask);  
    insert_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask, tmp_route->dev->name);
    ipq_send_ip(ip);

    curr_time = getcurrtime();  /* Get current time */
    /* Check if the lifetime in RT is valid, if not update it */

    tmp_route->lifetime =  curr_time +  ACTIVE_ROUTE_TIMEOUT;

    return 0;
}


inline int compare_aodv_route(aodv_route * tmp_route, u_int32_t target_ip)
{
    if ((tmp_route->ip & tmp_route->netmask) == (target_ip & tmp_route->netmask))
        return 1;
    else
        return 0;
}


aodv_route *find_aodv_route(u_int32_t target_ip)
{
    aodv_route *tmp_route, *dead_route;
    aodv_route *possible_route = NULL;
    u_int64_t curr_time = getcurrtime();

    /*lock table */
    route_read_lock();

    tmp_route = aodv_route_table;

    while ((tmp_route != NULL) && (tmp_route->ip <= target_ip))
    {
        if (time_before( tmp_route->lifetime, curr_time) && (!tmp_route->self_route) && (tmp_route->route_valid))
        {
            expire_aodv_route(tmp_route);
	}
        
        if (compare_aodv_route(tmp_route, target_ip))
        {
        //printk("it looks like the route %s",inet_ntoa(tmp_route->ip));
        //printk("is equal to: %s\n",inet_ntoa(target_ip));
            possible_route = tmp_route;
        }
        tmp_route = tmp_route->next;
    }
    /*unlock table */
    route_read_unlock();

    return possible_route;
}

/*int find_metric(u_int32_t tmp_ip)
{
    return 1;
}*/

int read_route_table_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data)
{
    static char *my_buffer;
    char temp_buffer[200];
    char temp[100];
    aodv_route *tmp_entry;
    u_int64_t remainder, numerator;
    u_int64_t tmp_time;
    int len,i;
    u_int64_t currtime;
    char dst[16];
    char hop[16];

    currtime=getcurrtime();

    /*lock table*/
    route_read_lock();

    tmp_entry = aodv_route_table;

    my_buffer=buffer;

    sprintf(my_buffer,"\nRoute Table \n---------------------------------------------------------------------------------\n");
    sprintf(temp_buffer,"        IP        |    Seq    |   Hop Count  |     Next Hop  \n");
    strcat(my_buffer,temp_buffer);
    sprintf(temp_buffer,"---------------------------------------------------------------------------------\n");
    strcat(my_buffer,temp_buffer);

    while (tmp_entry!=NULL)
    {
        strcpy(hop,inet_ntoa(tmp_entry->next_hop));
        strcpy(dst,inet_ntoa(tmp_entry->ip));
        sprintf(temp_buffer,"  %-16s     %5u       %3d         %-16s ",dst ,tmp_entry->seq,tmp_entry->metric,hop);
        strcat(my_buffer,temp_buffer);

	if (tmp_entry->self_route)
	{
	    strcat( my_buffer, " Self Route \n");
	}
	else
	{
	    if (tmp_entry->route_valid)
	        strcat(my_buffer, " Valid ");

	    tmp_time=tmp_entry->lifetime-currtime;
	    numerator = (tmp_time);
	    remainder = do_div( numerator, 1000 );
	    if (time_before(tmp_entry->lifetime, currtime) )
	    {
		sprintf(temp," Expired!\n");
	    }
	    else
	    {
		sprintf(temp," sec/msec: %lu/%lu \n", (unsigned long)numerator, (unsigned long)remainder);
	    }
	    strcat(my_buffer,temp);
	}
	
        tmp_entry=tmp_entry->next;
    }

    /*unlock table*/
    route_read_unlock();

    strcat(my_buffer,"---------------------------------------------------------------------------------\n\n");

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}

	
