/***************************************************************************
                          hello.c  -  description
                             -------------------
    begin                : Wed Aug 13 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "hello.h"

extern aodv_route *g_my_route;
extern u_int32_t 	g_my_ip;
#ifdef LINK_LIMIT
extern int g_link_limit;
#endif

rrep * create_hello()
{
    rrep *new_rrep;
    return new_rrep;
}

int send_hello()
{
    rrep *tmp_rrep;
    int i;
    aodv_dst * tmp_dst;
    struct interface_list_entry *tmp_interface;

    if ((tmp_rrep = (rrep *) kmalloc(sizeof(rrep) , GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Can't allocate new rreq\n");
        //neigh_read_unlock();
        return 0;
    }

    tmp_rrep->type = RREP_MESSAGE;
    tmp_rrep->reserved1 = 0;
		
    tmp_rrep->src_ip = g_my_route->ip;
    tmp_rrep->dst_ip = g_my_route->ip;
    tmp_rrep->dst_seq = htonl(g_my_route->seq);
    tmp_rrep->lifetime = htonl(MY_ROUTE_TIMEOUT);

    local_broadcast(1, tmp_rrep, sizeof(rrep) );

    kfree (tmp_rrep);
    insert_timer(TASK_HELLO, HELLO_INTERVAL, g_my_ip);
    update_timer_queue();
}

int recv_hello(rrep * tmp_rrep, task * tmp_packet)
{
    aodv_route *recv_route;
    aodv_neigh *tmp_neigh;

    u_int32_t neigh_ip = tmp_rrep -> dst_ip;
    tmp_neigh = find_aodv_neigh(tmp_rrep->dst_ip);
    if (tmp_neigh == NULL)
    {
        tmp_neigh = create_aodv_neigh(tmp_rrep->dst_ip);
        if (!tmp_neigh)
        {
            printk(KERN_WARNING "AODV: Error creating neighbor: %s\n", inet_ntoa(tmp_rrep->dst_ip));
            return -1;
        }
        memcpy(&(tmp_neigh->hw_addr), &(tmp_packet->src_hw_addr), sizeof(unsigned char) * ETH_ALEN);
        tmp_neigh->dev = tmp_packet->dev;

#ifdef AODV_SIGNAL
        set_spy();
#endif		

    }
    
    delete_timer(tmp_neigh->ip, TASK_NEIGHBOR);
    insert_timer(TASK_NEIGHBOR, HELLO_INTERVAL * (1 + ALLOWED_HELLO_LOSS) + 100, tmp_neigh->ip);
    update_timer_queue();
    tmp_neigh->lifetime = tmp_rrep->lifetime + getcurrtime() + 20;
    update_aodv_neigh(tmp_neigh, tmp_rrep); 
  
    return 0;
}
