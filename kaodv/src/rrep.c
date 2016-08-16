/***************************************************************************
                          rrep.c  -  description
                             -------------------
    begin                : Wed Aug 6 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "rrep.h"

extern u_int32_t	aodv_overhead;

void convert_rrep_to_host(rrep * tmp_rrep)
{
    tmp_rrep->dst_seq = ntohl(tmp_rrep->dst_seq);
    tmp_rrep->lifetime = ntohl(tmp_rrep->lifetime);
}


void convert_rrep_to_network(rrep * tmp_rrep)
{
    tmp_rrep->dst_seq = htonl(tmp_rrep->dst_seq);
    tmp_rrep->lifetime = htonl(tmp_rrep->lifetime);
}


int check_rrep(rrep * tmp_rrep, task * tmp_packet)
{
    char dst_ip[16];
    char src_ip[16];
    u_int32_t tmp;
    u_int64_t timer;

    if (tmp_rrep->src_ip == tmp_rrep->dst_ip)
    {  		
        //its a hello messages! HELLO WORLD!
	aodv_overhead--;
        recv_hello(tmp_rrep, tmp_packet);
        return 0;
    }

    //printk(KERN_INFO "receiving a rrep\n");

    if (!valid_aodv_neigh(tmp_packet->src_ip))
    {
        //printk(KERN_INFO "AODV: Not processing RREP from %s, not a valid neighbor\n", inet_ntoa(tmp_packet->src_ip));
        return 0;
    }

    delete_timer(tmp_rrep->dst_ip, TASK_RESEND_RREQ);
    update_timer_queue();
    strcpy(src_ip, inet_ntoa(tmp_packet->src_ip));
    strcpy(dst_ip, inet_ntoa(tmp_rrep->dst_ip));
    //printk(KERN_INFO "AODV: recv a route to: %s next hop: %s \n", dst_ip, src_ip);

    return 1;
}


int recv_rrep(task * tmp_packet)
{
    aodv_route *send_route;
    aodv_route *recv_route;
    char dst_ip[16];
    char src_ip[16];

    rrep *tmp_rrep;

    tmp_rrep = tmp_packet->data;
    convert_rrep_to_host(tmp_rrep);

    if (!check_rrep(tmp_rrep, tmp_packet))
    {
        return 0;
    }

    tmp_rrep->metric++;

    update_aodv_route(tmp_rrep->dst_ip, tmp_packet->src_ip, tmp_rrep->metric, tmp_rrep->dst_seq, tmp_packet->dev);
    send_route = find_aodv_route(tmp_rrep->src_ip);
	    
    if (!send_route)
    {
        strcpy(src_ip, inet_ntoa(tmp_rrep->src_ip));
        strcpy(dst_ip, inet_ntoa(tmp_rrep->dst_ip));
        printk(KERN_WARNING "AODV: No reverse-route for RREP from: %s to: %s", dst_ip, src_ip);
        return 0;
    }

    if (!send_route->self_route)
    {
        strcpy(src_ip, inet_ntoa(tmp_rrep->src_ip));
        strcpy(dst_ip, inet_ntoa(tmp_rrep->dst_ip));
        //printk(KERN_INFO "AODV: Forwarding a route to: %s from node: %s \n", dst_ip, src_ip);

        convert_rrep_to_network(tmp_rrep);
        send_message(send_route->next_hop, NET_DIAMETER, tmp_rrep, sizeof(rrep), NULL);
    }

    /* If I'm not the destination of the RREP I forward it */
    return 0;
}



int send_rrep(aodv_route * src_route, aodv_route * dst_route, rreq * tmp_rreq)
{
    rrep *tmp_rrep;

    if ((tmp_rrep = (rrep *) kmalloc(sizeof(rrep), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Can't allocate new rreq\n");
        return 0;
    }

    tmp_rrep->type = RREP_MESSAGE;

    tmp_rrep->reserved1 = 0;
    tmp_rrep->reserved2 = 0;
    tmp_rrep->src_ip = src_route->ip;
    tmp_rrep->dst_ip = dst_route->ip;
    tmp_rrep->dst_seq = htonl(dst_route->seq);
    tmp_rrep->metric = dst_route->metric;
    if (dst_route->self_route)
    {
        tmp_rrep->lifetime = htonl(MY_ROUTE_TIMEOUT);
    }
    else
    {
        tmp_rrep->lifetime = htonl(dst_route->lifetime - getcurrtime());
    }

    send_message(src_route->next_hop, NET_DIAMETER, tmp_rrep, sizeof(rrep) , NULL);
    kfree(tmp_rrep);
}


int gen_rrep(u_int32_t src_ip, u_int32_t dst_ip, rreq *tmp_rreq)
{
    rrep tmp_rrep;
    aodv_route *src_route;
    aodv_route *dst_route;
    //struct interface_list_entry *tmp_interface=NULL;

    char src_ip_str[16];
    char dst_ip_str[16];

    /* Get the source and destination IP address from the RREQ */
    if (!(src_route = find_aodv_route(src_ip)))
    {
        printk(KERN_WARNING "AODV: RREP  No route to Source! src: %s\n", inet_ntoa(src_ip));
        return -EHOSTUNREACH;
    }

    if (!(dst_route = find_aodv_route(dst_ip)))
    {
        printk(KERN_WARNING "AODV: RREP  No route to Dest! dst: %s\n", inet_ntoa(dst_ip));
        return -EHOSTUNREACH;
    }

    send_rrep(src_route, dst_route, tmp_rreq);

    return 0;
}
