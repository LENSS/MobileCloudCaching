/***************************************************************************
                          socket.c  -  description
                             -------------------
    begin                : Wed Aug 13 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#include "socket.h"

static struct sockaddr_in sin;
extern u_int32_t g_broadcast_ip;

int init_sock(struct socket *sock, u_int32_t ip, char *dev_name)
{

    int error;
    struct ifreq interface;
    mm_segment_t oldfs;

    //set the address we are sending from
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip;
    sin.sin_port = htons(AODVPORT);

    sock->sk->__sk_common.skc_reuse = 1;
    sock->sk->sk_allocation = GFP_ATOMIC;
    sock->sk->sk_priority = GFP_ATOMIC;

    error = sock->ops->bind(sock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
    strncpy(interface.ifr_ifrn.ifrn_name, dev_name, IFNAMSIZ);

    if (error < 0)
    {
        printk(KERN_ERR "binding failed!\n");
        return 0;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);          //thank to Soyeon Anh and Dinesh Dharmaraju for spotting this bug!
    error = sock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *) &interface, sizeof(interface));
    set_fs(oldfs);

    if (error < 0)
    {
        printk(KERN_ERR "Kernel AODV: Error, %d  binding socket. This means that some other \n", error);
        printk(KERN_ERR "        daemon is (or was a short time ago) using port %i.\n", AODVPORT);
        return 0;
    }

    int broadcast_enabled = 1;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = sock_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enabled, sizeof(broadcast_enabled));
    set_fs(oldfs);

    if (error < 0)
    {
        printk(KERN_ERR "broadcast setting failed!\n");
        return 0;
    }
    //printk(KERN_INFO "init_sock finished\n");
    return 0;
}

void close_sock(void)
{
    aodv_dev *tmp_dev, *dead_dev;

    tmp_dev = first_aodv_dev();
    while (tmp_dev != NULL)
    {
        sock_release(tmp_dev->sock);
        dead_dev = tmp_dev;
        tmp_dev = tmp_dev->next;
        kfree(dead_dev);
    }
}


int local_broadcast(u_int8_t ttl, void *data,const size_t datalen)
{
    //printk(KERN_INFO "entering local_broadcast\n");
    aodv_dev *tmp_dev;
    struct msghdr msg;
    struct iovec iov;
    u_int64_t curr_time = getcurrtime();
    mm_segment_t oldfs;
    int len = 0;
    //int broadcast_enabled = 1;
    int error;


    if (ttl < 1)
    {
        return 0;
    }

	
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = g_broadcast_ip;
    sin.sin_port = htons((unsigned short) AODVPORT);

    //define the message we are going to be sending out
    msg.msg_name = (void *) &(sin);
    msg.msg_namelen = sizeof(sin);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
//    msg.msg_flags = MSG_NOSIGNAL;
    msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;
    msg.msg_iov->iov_len = (__kernel_size_t) datalen;
    msg.msg_iov->iov_base = (char *) data;

    tmp_dev = first_aodv_dev();
    //struct inet_sock *inet;
    //printk(KERN_INFO "before setting ttl\n");
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = tmp_dev->sock->ops->setsockopt(tmp_dev->sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
    set_fs(oldfs);
    if (error < 0)
    {
        printk(KERN_ERR "local broadcast ttl setting failed!\n");
        return 0;
    }

    while ((tmp_dev) && (tmp_dev->sock) && (sock_wspace(tmp_dev->sock->sk) >= datalen))
    {
        /*if (sock_setsockopt(tmp_dev->sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enabled, sizeof(broadcast_enabled)) < 0)
        {
            printk(KERN_ERR "setsockopt() failed!\n");
            return 0;
        }*/

        //((struct inet_sock *)(tmp_dev->sock->sk->sk_protinfo))->uc_ttl=ttl;
        //inet = inet_sk(tmp_dev->sock->sk);
        //inet->uc_ttl = ttl;

        //tmp_dev->sock->sk->broadcast = 1;
        //tmp_dev->sock->sk->protinfo.af_inet.ttl = ttl;

        oldfs = get_fs();
        set_fs(KERNEL_DS);
       
        len = sock_sendmsg(tmp_dev->sock, &msg,(size_t) datalen);			
        if (len < 0)
            printk(KERN_WARNING "AODV: Error sending! err no: %d, on interface: %s\n", len, tmp_dev->dev->name);
        set_fs(oldfs);

        tmp_dev = tmp_dev->next;
    }

    return len;
}


int send_message(u_int32_t dst_ip, u_int8_t ttl, void *data, const size_t datalen, struct net_device *dev)
{
    //printk(KERN_INFO "entering send_message\n");
    struct msghdr msg;
    struct iovec iov;
    aodv_dev *tmp_dev;
    aodv_route *tmp_route;
    mm_segment_t oldfs;
    u_int64_t curr_time;
    u_int32_t space;
    int len;
    //int broadcast_enabled = 0;
    int error;


    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = dst_ip;
    sin.sin_port = htons((unsigned short) AODVPORT);



    //define the message we are going to be sending out
    msg.msg_name = (void *) &(sin);
    msg.msg_namelen = sizeof(sin);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = MSG_DONTWAIT | MSG_NOSIGNAL;
    msg.msg_iov->iov_len = (__kernel_size_t) datalen;
    msg.msg_iov->iov_base = (char *) data;


    if (ttl == 0)
        return 0;

    curr_time = getcurrtime();
		
    if (dev)
    {
	tmp_dev = find_aodv_dev_by_dev(dev);
    }
    else
    {
        tmp_route = find_aodv_route(dst_ip);
        if (tmp_route == NULL)
        {
            printk(KERN_WARNING "AODV: Can't find route to: %s \n", inet_ntoa(dst_ip));
            return -EHOSTUNREACH;
        }
        tmp_dev = find_aodv_dev_by_dev(tmp_route->dev);
    }

    if (tmp_dev == NULL)
    {

        printk(KERN_WARNING "AODV: Error sending! Unable to find interface!\n");
        return -ENODEV;
    }

    space = sock_wspace(tmp_dev->sock->sk);

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = tmp_dev->sock->ops->setsockopt(tmp_dev->sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
    set_fs(oldfs);
    if (error < 0)
    {
        printk(KERN_ERR "ttl setting failed!\n");
        return 0;
    }

    if (space < datalen)
    {
        printk(KERN_WARNING "AODV: Space: %d, Data: %d \n", space, datalen);
        return -ENOMEM;
    }

    /*if (sock_setsockopt(tmp_dev->sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcast_enabled, sizeof(broadcast_enabled)) < 0)
    {
        printk(KERN_ERR "setsockopt() failed!\n");
        return 0;
    }*/
    //((struct inet_sock *)(tmp_dev->sock->sk->sk_protinfo))->uc_ttl=ttl;

    //tmp_dev->sock->sk->broadcast = 0;
    //tmp_dev->sock->sk->sk_protinfo.af_inet.ttl = ttl;
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    len = sock_sendmsg(tmp_dev->sock, &msg,(size_t) datalen);
    if (len < 0)
    {
        printk(KERN_WARNING "AODV: Error sending! err no: %d, Dst: %s\n", len, inet_ntoa(dst_ip));
    }
    set_fs(oldfs);
    return 0;
}
