/***************************************************************************
                          packet_out.c  -  description
                             -------------------
    begin                : Mon Aug 4 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "packet_out.h"

extern u_int32_t g_broadcast_ip;
extern u_int32_t g_my_ip;
extern u_int8_t g_aodv_gateway;

unsigned int output_handler(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn) (struct sk_buff *))
{
    //printk("KAODV: entering output_handler\n");
    //struct iphdr *ip= (*skb)->nh.iph;
    const struct iphdr *ip = ip_hdr(skb);
    //struct iphdr *ip = ip_hdr(skb);
    //printk(KERN_INFO "got ip header\n");
    //struct iphdr *dev_ip = out->ip_ptr;
    //printk(KERN_INFO "dev_ip: %s\n", inet_ntoa(dev_ip->saddr));

    if (ip->daddr == g_broadcast_ip)
    {
        //printk(KERN_INFO "outgoing broadcast message\n");
        //struct inet_sock *inet = inet_sk(skb->sk);
        //printk(KERN_INFO "hello message from %s to %s\n", inet_ntoa(inet->saddr), inet_ntoa(inet->daddr));
        return NF_ACCEPT;
    }
    //printk(KERN_INFO "compared with g_broadcast_ip\n");

    struct net_device *dev= skb->dev;
    aodv_route *tmp_route;
    aodv_neigh *tmp_neigh;
    //void *p = (uint32_t *) ip + ip->ihl;
    //struct udphdr *udp = p; //(struct udphdr *) ip + ip->ihl;
    //struct ethhdr *mac = (*skb)->mac.ethernet;  //Thanks to Randy Pitz for adding this extra check...
    struct ethhdr *mac = (struct ethhdr *)skb_mac_header(skb);
	
    //Need this additional check, otherwise users on the
    //gateway will not be able to access the externel 
    //network. Remote user located outside the aodv_subnet
    //also require this check if they are access services on
    //the gateway node. 28 April 2004 pb 
    if(g_aodv_gateway && !aodv_subnet_test(ip->daddr))
    {
        return NF_ACCEPT;
    }
	
    //Try to get a route to the destination
    tmp_route = find_aodv_route(ip->daddr);
    if ((tmp_route == NULL) || !(tmp_route->route_valid))
    {
        //struct inet_sock *inet = inet_sk(skb->sk);
        //printk(KERN_INFO "output packet to %s\n", inet_ntoa(ip->daddr));
        gen_rreq(g_my_ip, ip->daddr);
        return NF_QUEUE;
    }

    if ((tmp_route != NULL) && (tmp_route->route_valid))        //Found after an LONG search by Jon Anderson
    {
	if (!tmp_route->self_route)
        //{printk(KERN_INFO "1-routed packet from %s to %s\n", inet_ntoa(ip->saddr), inet_ntoa(ip->daddr));
        tmp_route->lifetime =  getcurrtime() + ACTIVE_ROUTE_TIMEOUT;
        //struct inet_sock *inet = inet_sk(skb->sk);
        //printk(KERN_INFO "2-routed packet from %s to %s\n", inet_ntoa(inet->saddr), inet_ntoa(inet->daddr));}
    }
    
    return NF_ACCEPT;
}
