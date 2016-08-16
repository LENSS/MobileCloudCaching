/***************************************************************************
                          rreq.c  -  description
                             -------------------
    begin                : Mon Aug 4 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/


 
#include "rreq.h"
extern aodv_route * g_my_route;
extern u_int8_t 		g_aodv_gateway;
extern u_int32_t 		g_broadcast_ip;

void convert_rreq_to_host(rreq * tmp_rreq)
{
    tmp_rreq->rreq_id = ntohl(tmp_rreq->rreq_id);
    tmp_rreq->dst_seq = ntohl(tmp_rreq->dst_seq);
    tmp_rreq->src_seq = ntohl(tmp_rreq->src_seq);
}

void convert_rreq_to_network(rreq * tmp_rreq)
{
    tmp_rreq->rreq_id = htonl(tmp_rreq->rreq_id);
    tmp_rreq->dst_seq = htonl(tmp_rreq->dst_seq);
    tmp_rreq->src_seq = htonl(tmp_rreq->src_seq);
}


/****************************************************

   recv_rreq
----------------------------------------------------
Handles the recieving of RREQs
****************************************************/

int check_rreq(rreq * tmp_rreq, u_int32_t last_hop_ip)
{
    aodv_neigh *tmp_neigh;
    aodv_route *src_route;

    char src_ip[16];
    char dst_ip[16];
    char pkt_ip[16];

    if (find_flood_id(tmp_rreq->src_ip, tmp_rreq->rreq_id))
    {
        return 0;
    }

    src_route = find_aodv_route(tmp_rreq->src_ip);

    if (!valid_aodv_neigh(last_hop_ip))
    {
       return 0;
    }
	
    if ((src_route != NULL) && (src_route->next_hop != last_hop_ip) && (src_route->next_hop == src_route->ip))
    {
        strcpy(src_ip, inet_ntoa(tmp_rreq->src_ip));
        strcpy(dst_ip, inet_ntoa(tmp_rreq->dst_ip));
        strcpy(pkt_ip, inet_ntoa(last_hop_ip));
        //printk("Last Hop of: %s Doesn't match route: %s dst: %s RREQ_ID: %u, expires: %d\n", pkt_ip, src_ip, dst_ip, tmp_rreq->rreq_id, src_route->lifetime - getcurrtime());
    }

    return 1;
}


int reply_to_rreq(rreq * tmp_rreq)
{
    char src_ip[16];
    char dst_ip[16];
    char pkt_ip[16];
    aodv_route *dst_route;

    dst_route = find_aodv_route(tmp_rreq->dst_ip);

    if ( !aodv_subnet_test(tmp_rreq->dst_ip))
    {
	if (g_aodv_gateway)
	{
	    printk("Gatewaying for address: %s, ", inet_ntoa( tmp_rreq->dst_ip ));
	    if (dst_route == NULL)
	    {
	    	printk("creating route for: %s \n",inet_ntoa( tmp_rreq->dst_ip ));
		dst_route = create_aodv_route(tmp_rreq->dst_ip);
		dst_route->seq = g_my_route->seq;
    		dst_route->next_hop = tmp_rreq->dst_ip;
		dst_route->metric = 1;
		dst_route->dev = g_my_route->dev;
            }
            else
	    {
		printk("using route: %s \n",inet_ntoa( dst_route->ip ));
	    }

	    dst_route->lifetime =  getcurrtime() +  ACTIVE_ROUTE_TIMEOUT;
            dst_route->route_valid = TRUE;
            dst_route->route_seq_valid = TRUE;

	    return 1;
            }

	    //it is a local subnet and we need to create a route for it before we can reply...
	    if ((dst_route!=NULL) && (dst_route->netmask!=g_broadcast_ip))
	    {
	    	printk("creating route for local address: %s \n",inet_ntoa( tmp_rreq->dst_ip ));
		dst_route = create_aodv_route(tmp_rreq->dst_ip);
		dst_route->seq = g_my_route->seq;
    		dst_route->next_hop = tmp_rreq->dst_ip;
		dst_route->metric = 1;
		dst_route->dev = g_my_route->dev;
	        dst_route->lifetime =  getcurrtime() +  ACTIVE_ROUTE_TIMEOUT;
		dst_route->route_valid = TRUE;
		dst_route->route_seq_valid = TRUE;

		return 1;
	     }
	}
	else
	{
            // if it is not outside of the AODV subnet and I am the dst...
            if (dst_route && dst_route->self_route)
            {
                if (seq_less_or_equal(dst_route->seq, tmp_rreq->dst_seq))
                {
                    dst_route->seq = tmp_rreq->dst_seq + 1;
                }

                strcpy(src_ip, inet_ntoa(tmp_rreq->src_ip));
                strcpy(dst_ip, inet_ntoa(tmp_rreq->dst_ip));
                //printk(KERN_INFO "AODV: Destination, Generating RREP -  src: %s dst: %s \n", src_ip, dst_ip);
                return 1;
            }
	}
		
    /* Test to see if we should send a RREP AKA we have or are the desired route */

    return 0;
}

int forward_rreq(rreq * tmp_rreq, int ttl)
{
    convert_rreq_to_network(tmp_rreq);
		
    /* Call send_datagram to send and forward the RREQ */
    local_broadcast(ttl - 1, tmp_rreq, sizeof(rreq));

    return 0;
}


int recv_rreq(task * tmp_packet)
{
    rreq *tmp_rreq;
    aodv_route *src_route;      /* Routing table entry */
    u_int64_t current_time;     /* Current time */
    int size_out;

    tmp_rreq = tmp_packet->data;
    convert_rreq_to_host(tmp_rreq);
    current_time = getcurrtime();       /* Get the current time */


    /* Look in the route request list to see if the node has
       already received this request. */

    if (tmp_packet->ttl <= 1)
    {
        printk(KERN_INFO "AODV TTL for RREQ from: %s expired\n", inet_ntoa(tmp_rreq->src_ip));
        return -ETIMEDOUT;
    }

    if (!check_rreq(tmp_rreq, tmp_packet->src_ip))
    {
        return 1;
    }

    tmp_rreq->metric++;

    /* Have not received this RREQ within BCAST_ID_SAVE time */
    /* Add this RREQ to the list for further checks */

    /* UPDATE REVERSE */
    update_aodv_route(tmp_rreq->src_ip, tmp_packet->src_ip, tmp_rreq->metric, tmp_rreq->src_seq, tmp_packet->dev);
    insert_flood_id(tmp_rreq->src_ip, tmp_rreq->dst_ip, tmp_rreq->rreq_id, current_time + PATH_DISCOVERY_TIME);

    switch (reply_to_rreq(tmp_rreq))
    {
        case 1:
            gen_rrep(tmp_rreq->src_ip, tmp_rreq->dst_ip, tmp_rreq);
            return 0;
            break;
    }

    forward_rreq(tmp_rreq, tmp_packet->ttl);

    return 0;
}


int resend_rreq(task * tmp_packet)
{
    aodv_route *tmp_route;
    rreq *out_rreq;
    u_int8_t out_ttl;

    if (tmp_packet->retries <= 0)
    { 
	ipq_drop_ip(tmp_packet->dst_ip);
	return 0;
    }

    //printk(KERN_INFO "Resending a RREQ\n");
	
    if ((out_rreq = (rreq *) kmalloc(sizeof(rreq), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Can't allocate new rreq\n");
        return 0;
    }

    /* Get routing table entry for destination */
    tmp_route = find_aodv_route(tmp_packet->dst_ip);

    if (tmp_route == NULL)
    {
        /* Entry does not exist -> set to initial values */
        out_rreq->dst_seq = htonl(0);
        out_rreq->u = TRUE;
        //out_ttl = TTL_START;
    }
    else
    {
        /* Entry does exist -> get value from rt */
        out_rreq->dst_seq = htonl(tmp_route->seq);
        out_rreq->u = FALSE;
        //out_ttl = tmp_route->metric + TTL_INCREMENT;
    }

    out_ttl = NET_DIAMETER;
    
    /* Get routing table entry for source, when this is ourself this one should always exist */

    tmp_route = find_aodv_route(tmp_packet->src_ip);

    if (tmp_route == NULL)
    {
        printk(KERN_WARNING "AODV: Can't get route to source: %s\n", inet_ntoa(tmp_packet->src_ip));
        kfree(out_rreq);
        return -EHOSTUNREACH;
    }

    /* Get our own sequence number */
    tmp_route->rreq_id = tmp_route->rreq_id + 1;
    tmp_route->seq = tmp_route->seq + 1;
    out_rreq->src_seq = htonl(tmp_route->seq);
    out_rreq->rreq_id = htonl(tmp_route->rreq_id);

    /* Fill in the package */
    out_rreq->dst_ip = tmp_packet->dst_ip;
    out_rreq->src_ip = tmp_packet->src_ip;
    out_rreq->type = RREQ_MESSAGE;
    out_rreq->metric = htonl(0);
    out_rreq->j = 0;
    out_rreq->r = 0;
    out_rreq->d = 0;
    out_rreq->reserved = 0;
    out_rreq->second_reserved = 0;
    out_rreq->g = 1;

    /* Get the broadcast address and ttl right */

    if (insert_flood_id(tmp_packet->src_ip, tmp_packet->dst_ip, tmp_route->rreq_id, getcurrtime() + PATH_DISCOVERY_TIME) < 0)
    {
        kfree(out_rreq);
        printk(KERN_WARNING "AODV: Can't add to broadcast list\n");
        return -ENOMEM;
    }
  
    //    insert_timer_queue_entry(getcurrtime() + NET_TRAVERSAL_TIME,timer_rreq, sizeof(struct rreq),out_rreq->dst_ip,0,out_ttl, EVENT_RREQ);
    //insert_timer_queue_entry(getcurrtime() + NET_TRAVERSAL_TIME,timer_rreq, sizeof(struct rreq),out_rreq->dst_ip,RREQ_RETRIES,out_ttl, EVENT_RREQ);
    insert_rreq_timer(out_rreq, tmp_packet->retries - 1);
    update_timer_queue();

    //local_broadcast(out_ttl,out_rreq,sizeof(struct rreq));
    //printk(KERN_INFO "resend_rreq: before local_broadcast\n");
    local_broadcast(out_ttl, out_rreq, sizeof(rreq));
    //printk(KERN_INFO "resend_rreq: after local_broadcast\n");

    /*struct net_device *dev;
    const char *dev_name = "wlan0";
    dev = dev_get_by_name(&init_net, dev_name);
    aodv_route *sec_route;
    const char *cp = "192.168.0.40";
    uint32_t sec_ip;
    inet_aton(cp, &sec_ip);
    printk(KERN_INFO "adding route to %s, unformated: %d\n", inet_ntoa(sec_ip), sec_ip);
    sec_route = create_aodv_route(sec_ip);

    delete_kernel_route_entry(sec_route->ip, sec_route->next_hop, sec_route->netmask);
    insert_kernel_route_entry(sec_route->ip, sec_route->next_hop, sec_route->netmask, dev->name);
    sec_route->lifetime = getcurrtime() + 20000;
    sec_route->route_seq_valid = TRUE;
    sec_route->route_valid = TRUE;
    sec_route->metric = 1;
    ipq_send_ip(sec_ip);*/

    kfree(out_rreq);

    return 0;

}

/****************************************************

   gen_rreq
----------------------------------------------------
Generates a RREQ! wahhooo!
****************************************************/
int gen_rreq(u_int32_t src_ip, u_int32_t dst_ip)
{
    aodv_route *tmp_route;
    rreq *out_rreq;
    u_int8_t out_ttl;

    if (find_timer(dst_ip, TASK_RESEND_RREQ) != NULL)
    {
        return 0;
    }

    /* Allocate memory for the rreq message */

    if ((out_rreq = (rreq *) kmalloc(sizeof(rreq), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Can't allocate new rreq\n");
        return 0;
    }

    printk(KERN_INFO "Generating a RREQ for: %s\n", inet_ntoa(dst_ip));
    if ((out_rreq = (rreq *) kmalloc(sizeof(rreq), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "AODV: Can't allocate new rreq\n");
        return -ENOMEM;
    }

    /* Get routing table entry for destination */
    tmp_route = find_aodv_route(dst_ip);

    if (tmp_route == NULL)
    {
        /* Entry does not exist -> set to initial values */
        out_rreq->dst_seq = htonl(0);
        out_rreq->u = TRUE;
        //out_ttl = TTL_START;
    }
    else
    {
        /* Entry does exist -> get value from rt */
        out_rreq->dst_seq = htonl(tmp_route->seq);
        out_rreq->u = FALSE;
        //out_ttl = tmp_route->metric + TTL_INCREMENT;
    }

    out_ttl = NET_DIAMETER;    
    
    /* Get routing table entry for source, when this is ourself this one should always exist */

    tmp_route = find_aodv_route(src_ip);

    if (tmp_route == NULL)
    {
        printk(KERN_WARNING "AODV: Can't get route to source: %s\n", inet_ntoa(src_ip));
        kfree(out_rreq);
        return -EHOSTUNREACH;
    }

    /* Get our own sequence number */
    tmp_route->rreq_id = tmp_route->rreq_id + 1;
    tmp_route->seq = tmp_route->seq + 1;
    out_rreq->src_seq = htonl(tmp_route->seq);
    out_rreq->rreq_id = htonl(tmp_route->rreq_id);

    /* Fill in the package */
    out_rreq->dst_ip = dst_ip;
    out_rreq->src_ip = src_ip;
    out_rreq->type = RREQ_MESSAGE;
    out_rreq->metric = htonl(0);
    out_rreq->j = 0;
    out_rreq->r = 0;
    out_rreq->d = 1;
    out_rreq->reserved = 0;
    out_rreq->second_reserved = 0;
    out_rreq->g = 1;

    //printk(KERN_INFO "before insert flood id\n");

    if (insert_flood_id(src_ip, dst_ip, tmp_route->rreq_id, getcurrtime() + PATH_DISCOVERY_TIME) < 0)
    {
        kfree(out_rreq);
        printk(KERN_WARNING "AODV: Can't add to broadcast list\n");
        return -ENOMEM;
    }

    //printk(KERN_INFO "before insert timer\n");

    insert_rreq_timer(out_rreq, RREQ_RETRIES);
    update_timer_queue();

    local_broadcast(out_ttl, out_rreq, sizeof(rreq));
    //local_broadcast(1, out_rreq, sizeof(rreq));

    kfree(out_rreq);

    return 0;

}
