
#include "cSocket.h"

static struct sockaddr_in sin;
extern u_int32_t g_broadcast_ip;
extern struct socket *g_sock;
extern char g_dev[8];

int init_sock(u_int32_t ip, char *dev_name)
{
    int error;
    struct ifreq interface;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = sock_create(PF_INET, SOCK_DGRAM, 0, &g_sock);
    set_fs(oldfs);
    if (error < 0)
    {
        printk(KERN_ERR "Error during creation of socket; terminating, %d\n", error);
        return error;
    }

    //set the address we are sending from
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip;
    sin.sin_port = htons(CACHEPORT);

    g_sock->sk->__sk_common.skc_reuse = 1;
    g_sock->sk->sk_allocation = GFP_ATOMIC;
    g_sock->sk->sk_priority = GFP_ATOMIC;

    error = g_sock->ops->bind(g_sock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
    strncpy(interface.ifr_ifrn.ifrn_name, dev_name, IFNAMSIZ);

    if (error < 0)
    {
        printk(KERN_ERR "binding failed!\n");
        return 0;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = sock_setsockopt(g_sock, SOL_SOCKET, SO_BINDTODEVICE, (char *) &interface, sizeof(interface));
    set_fs(oldfs);

    if (error < 0)
    {
        printk(KERN_ERR "Cache Daemon: Error, %d  binding socket. This means that some other \n", error);
        printk(KERN_ERR "        daemon is (or was a short time ago) using port %i.\n", CACHEPORT);
        return 0;
    }

    int broadcast_enabled = 1;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = sock_setsockopt(g_sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enabled, sizeof(broadcast_enabled));
    set_fs(oldfs);

    if (error < 0)
    {
        printk(KERN_ERR "broadcast setting failed!\n");
        return 0;
    }
    return 0;
}

int local_broadcast(u_int8_t ttl, void *data, const size_t datalen)
{
    //printk("entering local_broadcast\n");
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    int len = 0;
    int error;

    if (ttl < 1)
    {
        return 0;
    }
	
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = g_broadcast_ip;
    sin.sin_port = htons((unsigned short) CACHEPORT);

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

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = g_sock->ops->setsockopt(g_sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
    set_fs(oldfs);
    if (error < 0)
    {
        printk(KERN_ERR "local broadcast ttl setting failed!\n");
        return 0;
    }

    if (sock_wspace(g_sock->sk) >= datalen)
    {
        oldfs = get_fs();
        set_fs(KERNEL_DS);
       
        len = sock_sendmsg(g_sock, &msg,(size_t) datalen);			
        if (len < 0)
            printk(KERN_WARNING "Cache Daemon: Error sending! err no: %d, on interface: %s\n", len, g_dev);
        set_fs(oldfs);
    }

    return len;
}

int send_message(u_int32_t dst_ip, u_int8_t ttl, void *data, const size_t datalen)
{
    //printk("datalen = %d\n", datalen);
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;
    u_int32_t space;
    int len;
    int error;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = dst_ip;
    sin.sin_port = htons((unsigned short) CACHEPORT);

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

    space = sock_wspace(g_sock->sk);

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    error = g_sock->ops->setsockopt(g_sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
    set_fs(oldfs);
    if (error < 0)
    {
        printk(KERN_ERR "ttl setting failed!\n");
        return 0;
    }

    if (space < datalen)
    {
        printk(KERN_WARNING "Cache Daemon: Space: %d, Data: %d \n", space, datalen);
        return -ENOMEM;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    len = sock_sendmsg(g_sock, &msg,(size_t) datalen);
    if (len < 0)
    {
        printk(KERN_WARNING "Cache Daemon: Error sending! err no: %d, Dst: %s\n", len, inet_ntoa(dst_ip));
    }
    set_fs(oldfs);
    return 0;
}


