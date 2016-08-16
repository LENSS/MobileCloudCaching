/***************************************************************************
                          aodv_neigh.c  -  description
                             -------------------
    begin                : Thu Jul 31 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "aodv_neigh.h"

aodv_neigh *aodv_neigh_list;
#ifdef LINK_LIMIT
extern int g_link_limit;
#endif
rwlock_t neigh_lock = RW_LOCK_UNLOCKED;


inline void neigh_read_lock()
{
    read_lock_bh(&neigh_lock);
}

inline void neigh_read_unlock()
{
    read_unlock_bh(&neigh_lock);
}

inline void neigh_write_lock()
{
    write_lock_bh(&neigh_lock);
}

inline void neigh_write_unlock()
{
    write_unlock_bh(&neigh_lock);
}



void init_aodv_neigh_list()
{
    aodv_neigh_list=NULL;
}


aodv_neigh *find_aodv_neigh(u_int32_t target_ip)
{
    aodv_neigh *tmp_neigh; 

    neigh_read_lock();
    tmp_neigh = aodv_neigh_list;
		
    while ((tmp_neigh != NULL) && (tmp_neigh->ip <= target_ip))
    {
        if (tmp_neigh->ip == target_ip)
        {
	    neigh_read_unlock();
            return tmp_neigh;
        }
        tmp_neigh = tmp_neigh->next;
    }

    neigh_read_unlock();
    return NULL;
}

int valid_aodv_neigh(u_int32_t target_ip)
{
    aodv_neigh *tmp_neigh;

    tmp_neigh = find_aodv_neigh(target_ip);

    if (tmp_neigh)
    {
        if (tmp_neigh->valid_link && time_before(getcurrtime(),tmp_neigh->lifetime))
            return 1;
	else
	    return 0;
    }
    else
	return 0;
}

aodv_neigh *find_aodv_neigh_by_hw(char *hw_addr)
{
    aodv_neigh *tmp_neigh = aodv_neigh_list;

    while (tmp_neigh != NULL)
    {
        if (!memcmp(&(tmp_neigh->hw_addr), hw_addr, ETH_ALEN))
            return tmp_neigh;
        tmp_neigh = tmp_neigh->next;
    }

    return NULL;
}

void update_aodv_neigh_link(char *hw_addr, u_int8_t link)
{
    //printk(KERN_INFO "entering update_aodv_neigh\n");
    aodv_neigh *tmp_neigh;
    u_int8_t link_temp;

    neigh_write_lock();

    tmp_neigh = aodv_neigh_list;

    //search the interface list for a device with the same ip
    while (tmp_neigh)
    {
        if (!memcmp(&(tmp_neigh->hw_addr), hw_addr, ETH_ALEN))
        {
            if (link)
                tmp_neigh->link = 0x100 - link;
            else
                tmp_neigh->link = 0;
            break;
        }
        tmp_neigh = tmp_neigh->next;
    }
    neigh_write_unlock();
}

int delete_aodv_neigh(u_int32_t ip)
{
    aodv_neigh *tmp_neigh;
    aodv_neigh *prev_neigh = NULL;
    aodv_route *tmp_route;

    neigh_write_lock();
    tmp_neigh = aodv_neigh_list;
    
    while (tmp_neigh != NULL)
    {
        if (tmp_neigh->ip == ip)
        {
            if (prev_neigh != NULL)
            {
                prev_neigh->next = tmp_neigh->next;
            }
            else
            {
                aodv_neigh_list = tmp_neigh->next;
            }

            kfree(tmp_neigh);
            delete_timer(ip, TASK_NEIGHBOR);
            update_timer_queue();
	    neigh_write_unlock();
        
            tmp_route=find_aodv_route(ip);
   	    if (tmp_route && (tmp_route->next_hop == tmp_route->ip))
    	    {
      	        delete_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask);
	        gen_rerr(tmp_route->ip);
     		if (tmp_route->route_valid)
        	{
	            expire_aodv_route(tmp_route);     			  
		}
		else
		{
		    remove_aodv_route(tmp_route);
		}
	    }	
            return 0;
        }
        prev_neigh = tmp_neigh;
        tmp_neigh = tmp_neigh->next;
    }

    neigh_write_unlock();

    return -ENODATA;
}

aodv_neigh *create_aodv_neigh(u_int32_t ip)
{
    aodv_neigh *new_neigh;
    aodv_neigh *prev_neigh = NULL;
    aodv_neigh *tmp_neigh = NULL;
    int i = 0;

    neigh_write_lock();

    tmp_neigh = aodv_neigh_list;
    while ((tmp_neigh != NULL) && (tmp_neigh->ip < ip))
    {
        prev_neigh = tmp_neigh;
        tmp_neigh = tmp_neigh->next;
    }

    if (tmp_neigh && (tmp_neigh->ip == ip))
    {
        printk(KERN_WARNING "AODV_NEIGH: Creating a duplicate neighbor\n");
        neigh_write_unlock();
        return NULL;
    }

    if ((new_neigh = kmalloc(sizeof(aodv_neigh), GFP_ATOMIC)) == NULL)
    {  
        printk(KERN_WARNING "NEIGHBOR_LIST: Can't allocate new entry\n");
        neigh_write_unlock();
        return NULL;
    }

    new_neigh->ip = ip;
    new_neigh->lifetime = 0;
    new_neigh->route_entry = NULL;
    new_neigh->link = 0;
    
#ifdef LINK_LIMIT
    new_neigh->valid_link = FALSE;
#else
    new_neigh->valid_link = TRUE;
#endif

    new_neigh->next = NULL;
	
    if (prev_neigh == NULL)
    {
        new_neigh->next = aodv_neigh_list;
        aodv_neigh_list = new_neigh;
    }
    else
    {
        new_neigh->next = prev_neigh->next;
        prev_neigh->next = new_neigh;
    }
    
    neigh_write_unlock();
   
    return new_neigh;
}

void update_aodv_neigh_route(aodv_neigh * tmp_neigh, rrep *tmp_rrep)
{
    aodv_route *tmp_route;
    tmp_route = find_aodv_route(tmp_neigh->ip);
	
    if (tmp_route)
    {
        /*if (!tmp_route->route_valid)
	{
	    insert_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask,tmp_route->dev->name);
	}

	if (tmp_route->next_hop!=tmp_route->ip)
	{*/
	    delete_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask);
	    insert_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask,tmp_route->dev->name);
	//}
    }
    else        
    {	    
	//printk(KERN_INFO "AODV: Creating route for neighbor: %s Link: %d\n", inet_ntoa(tmp_neigh->ip), tmp_neigh->link);
	tmp_route = create_aodv_route(tmp_neigh->ip);
	delete_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask);
	insert_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask,tmp_route->dev->name);
    }
		
    tmp_route->lifetime = getcurrtime() + tmp_rrep->lifetime;
    tmp_route->dev = tmp_neigh->dev;
    tmp_route->next_hop = tmp_neigh->ip;
    tmp_route->route_seq_valid = TRUE;
    tmp_route->route_valid = TRUE;
    tmp_route->seq = tmp_rrep->dst_seq;
    tmp_neigh->route_entry = tmp_route;
    //tmp_route->metric = find_metric(tmp_route->ip);
    tmp_route->metric = 1;
}

int update_aodv_neigh( aodv_neigh *tmp_neigh, rrep *tmp_rrep)
{

    if (tmp_neigh==NULL)
    {
	printk(KERN_WARNING "AODV: NULL neighbor passed!\n");
	return 0;
    }

#ifdef LINK_LIMIT
    if (tmp_neigh->link > (g_link_limit))
    {
	tmp_neigh->valid_link = TRUE;
    }
    /*if (tmp_neigh->link < (g_link_limit - 5))
    {
	tmp_neigh->valid_link = FALSE;
    }*/
#endif

    if (tmp_neigh->valid_link)
    {
	update_aodv_neigh_route(tmp_neigh, tmp_rrep);
    }

    return 1;
}


aodv_neigh *first_aodv_neigh()
{
    return aodv_neigh_list;
}


int read_neigh_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length,int *eof,void *data)
{
    char tmp_buffer[200];
    aodv_neigh *tmp_neigh;
    u_int64_t remainder, numerator;
    u_int64_t tmp_time;
    int len;
    u_int64_t currtime;

    currtime=getcurrtime();

    neigh_read_lock();

    tmp_neigh = aodv_neigh_list;

    sprintf(buffer,"\nAODV Neighbors \n---------------------------------------------------------------------------------\n");
    sprintf(tmp_buffer,"        IP        |   Link    |    Valid    |  Lifetime \n");
    strcat( buffer,tmp_buffer);
    sprintf(tmp_buffer,"---------------------------------------------------------------------------------\n");
    strcat( buffer,tmp_buffer);
    while (tmp_neigh!=NULL)
    {
        sprintf(tmp_buffer,"   %-16s     %d        ",inet_ntoa(tmp_neigh->ip) ,tmp_neigh->link );
        strcat(buffer,tmp_buffer);

	if (tmp_neigh->valid_link)
	{
	    strcat( buffer, "+       ");
	}
	else
	{
	    strcat( buffer, "-       ");
	}
			
	tmp_time=tmp_neigh->lifetime-currtime;
	numerator = (tmp_time);
	remainder = do_div( numerator, 1000 );
	if (time_before(tmp_neigh->lifetime, currtime) )
	{
	    sprintf(tmp_buffer," Expired!\n");
	}
	else
	{
	    sprintf(tmp_buffer," sec/msec: %lu/%lu \n", (unsigned long)numerator, (unsigned long)remainder );
        }
  
	strcat(buffer,tmp_buffer);
        tmp_neigh=tmp_neigh->next;
    }
    strcat(buffer,"---------------------------------------------------------------------------------\n\n");

    len = strlen(buffer);
    *buffer_location = buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;

    neigh_read_unlock();

    return len;
}

