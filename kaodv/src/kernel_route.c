/***************************************************************************
                          kernel_route.c  -  description
                             -------------------
    begin                : Fri Aug 8 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "kernel_route.h"

extern u_int32_t g_broadcast_ip;

struct rtentry *create_kernel_route_entry(u_int32_t dst_ip, u_int32_t gw_ip, u_int32_t genmask_ip, char *interf)
{
    struct rtentry *new_rtentry;
    struct sockaddr_in dst;
    struct sockaddr_in gw;
    struct sockaddr_in genmask;

    if ((new_rtentry = kmalloc(sizeof(struct rtentry), GFP_ATOMIC)) == NULL)
    {
        printk("KRTABLE: Gen- Error generating a route entry\n");
        return NULL;            /* Malloc failed */
    }

    dst.sin_family = AF_INET;
    gw.sin_family = AF_INET;
    genmask.sin_family = AF_INET;

    //dst.sin_addr.s_addr = dst_ip;
    dst.sin_addr.s_addr = dst_ip & genmask_ip;

//JPA
    /*if (gw_ip == dst_ip)
        gw.sin_addr.s_addr = 0;
    else*/
        gw.sin_addr.s_addr = gw_ip;
    //gw.sin_addr.s_addr = 0;

//JPA
    //genmask.sin_addr.s_addr=g_broadcast_ip;
    genmask.sin_addr.s_addr = genmask_ip;

//JPA
    new_rtentry->rt_flags = RTF_UP | RTF_HOST | RTF_GATEWAY;
    /*new_rtentry->rt_flags = RTF_UP;

//JPA
    if (gw_ip != dst_ip)
        new_rtentry->rt_flags |= RTF_GATEWAY;
*/
    new_rtentry->rt_metric = 0;
    new_rtentry->rt_dev = interf;
    new_rtentry->rt_dst = *(struct sockaddr *) &dst;
    new_rtentry->rt_gateway = *(struct sockaddr *) &gw;
    new_rtentry->rt_genmask = *(struct sockaddr *) &genmask;

    return new_rtentry;
}


int insert_kernel_route_entry(u_int32_t dst_ip, u_int32_t gw_ip, u_int32_t genmask_ip, char *interf)
{
    struct rtentry *new_krtentry;
    mm_segment_t oldfs;
    int error;

    if ((new_krtentry = create_kernel_route_entry(dst_ip, gw_ip, genmask_ip, interf)) == NULL)
    {
        return -1;
    }
 
    oldfs = get_fs();
    set_fs(get_ds());
    error = ip_rt_ioctl(&init_net, SIOCADDRT, (void __user *) new_krtentry);
		
    set_fs(oldfs);
    /*if (error<0)
        printk("error %d trying to insert route\n",error);
    */
    kfree(new_krtentry);

    return 0;
}


int delete_kernel_route_entry(u_int32_t dst_ip, u_int32_t gw_ip, u_int32_t genmask_ip)
{
    struct rtentry *new_krtentry;
    mm_segment_t oldfs;
    char dst_str[16];
    char gw_str[16];
    
    int error;
  
    if ((new_krtentry = create_kernel_route_entry(dst_ip, 0, genmask_ip, NULL)) == NULL)
    {
        strcpy(dst_str, inet_ntoa(dst_ip));
	strcpy(gw_str, inet_ntoa(gw_ip));
	printk("error 47000 trying to delete route %s through %s\n",dst_str, gw_str);
        return 1;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    while (ip_rt_ioctl(&init_net, SIOCDELRT, (void __user *) new_krtentry) > 0);
    
    set_fs(oldfs);
    kfree(new_krtentry);

    return 0;
}
