/***************************************************************************
                          packet_in.c  -  description
                             -------------------
    begin                : Mon Jul 29 2002
    copyright            : (C) 2002 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "packet_in.h"

extern u_int32_t	aodv_overhead;

int valid_aodv_packet(int numbytes, int type, char *data)
{
    rerr *tmp_rerr;
    rreq *tmp_rreq;
    rrep *tmp_rrep;
    switch (type)
    {
        //RREQ
        case 1:
	    tmp_rreq = (rreq *) data;
            //If it is a normal route rreq
	    if (numbytes == sizeof(rreq)) {
		aodv_overhead++;
		return 1;
	    }
            break;

        //RREP
        case 2:
	    tmp_rrep = (rrep *) data;
	    if (numbytes == sizeof(rrep)) {
		aodv_overhead++;
		return 1;
	    }		
	    break;

        //RERR
        case 3:
            // Normal RERR
            tmp_rerr = (rerr *) data;
            if (numbytes == (sizeof(rerr) + (sizeof(aodv_dst) * tmp_rerr->dst_count)))
            {
                return 1;
            }
            break;

        case 4:                    //Normal RREP-ACK
            if (numbytes == sizeof(rrep_ack))
	        return 1;
            break;

        default:
            break;
    }

    return 0;
}


int packet_in(struct sk_buff *packet)
{
    //struct net_device *dev;
    struct iphdr *ip;
    aodv_route *tmp_route;
    aodv_neigh *tmp_neigh;
    u_int32_t tmp_ip;

    // Create aodv message types
    u_int8_t aodv_type;

    //The packets which come in still have their headers from the IP and UDP
    int start_point = sizeof(struct udphdr) + sizeof(struct iphdr);

    //get pointers to the important parts of the message
    //ip = packet->nh.iph;
    ip = (struct iphdr *)skb_network_header(packet);
    //dev = packet->dev;

    /*if (strcmp(dev->name, "lo") == 0)
    {
        return NF_DROP;
    }*/
    //For all AODV packets the type is the first byte.
    aodv_type = (int) packet->data[start_point];

    if (!valid_aodv_packet(packet->len - start_point, aodv_type, packet->data + start_point))
    {
        printk(KERN_NOTICE
               "AODV: Packet of type: %d and of size %u from: %s failed packet check!\n", aodv_type, packet->len - start_point, inet_ntoa(ip->saddr));
        return NF_DROP;
    }

    /*tmp_neigh = find_aodv_neigh_by_hw(&(packet->mac.ethernet->h_source));

    if (tmp_neigh != NULL)
    {
        delete_timer(tmp_neigh->ip, TASK_NEIGHBOR);
  	insert_timer(TASK_NEIGHBOR, HELLO_INTERVAL * (1 + ALLOWED_HELLO_LOSS) + 100, tmp_neigh->ip);
    	update_timer_queue();
    }*/

    //place packet in the event queue!
    insert_task(aodv_type, packet);

    return NF_ACCEPT;
}



unsigned int input_handler(unsigned int hooknum, struct sk_buff *skb,
                           const struct net_device *in, const struct net_device *out, int (*okfn) (struct sk_buff *))
{
    //struct iphdr *ip = (*skb)->nh.iph;
    const struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
    struct iphdr *dev_ip = in->ip_ptr;
    void *p = (uint32_t *) ip + ip->ihl;
    struct udphdr *udp = p; //(struct udphdr *) ip + ip->ihl;
    //struct ethhdr *mac = (*skb)->mac.ethernet;  //Thanks to Randy Pitz for adding this extra check...
    struct ethhdr *mac = (struct ethhdr *)skb_mac_header(skb);
    
    //if ((*skb)->h.uh != NULL)
    if (skb_transport_header(skb) != NULL)
    {
        if ((udp->dest == htons(AODVPORT)) && (mac->h_proto == htons(ETH_P_IP)))
        {
            if (dev_ip->saddr != ip->saddr)
            {
                //printk(KERN_INFO "input_handler: packet from %s to %s\n", inet_ntoa(ip->saddr), inet_ntoa(ip->daddr));
		//aodv_overhead++;
                return packet_in(skb);
            }
            else
            {
            	//printk("dropping packet from: %s\n",inet_ntoa(ip->saddr));
            	return NF_DROP;
            }
        }
    }

    return NF_ACCEPT;

}

int read_overhead_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    static char *my_buffer;
    int len;

    my_buffer=buffer;
    sprintf(my_buffer, "%d\n", aodv_overhead);

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}


