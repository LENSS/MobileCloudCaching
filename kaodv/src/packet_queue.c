/*
 * This is a module which is used for queueing IPv4 packets and
 * communicating with userspace via netlink.
 *

 * (C) 2000 James Morris, this code is GPL.
 *
 * 2002-01-10: I modified this code slight to offer more comparisions... - Luke Klein-Berndt
 * 2000-03-27: Simplified code (thanks to Andi Kleen for clues).
 * 2000-05-20: Fixed notifier problems (following Miguel Freitas' report).
 * 2000-06-19: Fixed so nfmark is copied to metadata (reported by Sebastian
 *             Zander).
 * 2000-08-01: Added Nick Williams' MAC support.
 *
 */

#include "packet_queue.h"

#define IPQ_QMAX_DEFAULT 1024
//#define IPQ_PROC_FS_NAME "ip_queue"
//#define NET_IPQ_QMAX 2088
//#define NET_IPQ_QMAX_NAME "ip_queue_maxlen"

/*typedef struct ipq_rt_info {
    __u8 tos;
    __u32 daddr;
    __u32 saddr;
} ipq_rt_info_t;

typedef struct ipq_queue_element {
    struct nf_queue_entry *entry;
    struct list_head list;		// Links element into queue 
    int verdict;			// Current verdict 
    //struct nf_info *info;		// Extra info from netfilter 
    //struct sk_buff *skb;		// Packet inside 
    ipq_rt_info_t rt_info;		// May need post-mangle routing 
} ipq_queue_element_t;*/

//typedef int (*ipq_send_cb_t)(ipq_queue_element_t *e);

static LIST_HEAD(q_list);
static DEFINE_RWLOCK(q_lock);

typedef struct ipq_queue {
    int len;			/* Current queue len */
    int *maxlen;			/* Maximum queue len, via sysctl */
    unsigned char flushing;		/* If queue is being flushed */
    unsigned char terminate;	/* If the queue is being terminated */
    //struct list_head list;		/* Head of packet queue */
    //spinlock_t lock;		/* Queue spinlock */
} ipq_queue_t;

static ipq_queue_t *q;
static int netfilter_receive(struct nf_queue_entry *entry, unsigned int queuenum);

static const struct nf_queue_handler nf_handler = {
    .name = "ipq_queue",
    .outfn = & netfilter_receive,
};

/****************************************************************************
 *
 * Packet queue
 *
 ****************************************************************************/
/* Dequeue a packet if matched by cmp, or the next available if cmp is NULL */
//static ipq_queue_element_t * ipq_dequeue( int (*cmp)(ipq_queue_element_t *, u_int32_t), u_int32_t data)
static struct nf_queue_entry * ipq_dequeue( int (*cmp)(struct nf_queue_entry *, u_int32_t), u_int32_t data)
{
    //struct list_head *i;
    struct nf_queue_entry *i;
    //struct ipq_queue_element_t *i;

    //spin_lock_bh(&q->lock);
    write_lock_bh(&q_lock);
    //list_for_each_entry(i, &q, list)
    //for (i = q->list.prev; i != &q->list; i = i->prev)
    //for (i = q_list.prev; i != &q_list; i = i->prev)
    list_for_each_entry(i, &q_list, list)
    {
        //ipq_queue_element_t *e = (ipq_queue_element_t *)i;
        //struct nf_queue_entry *e = (struct nf_queue_entry *)i;
        struct nf_queue_entry *e = i;
        //ipq_queue_element_t *e = i;

        /*if (e->hook == NF_IP_POST_ROUTING)
        {
            struct iphdr *iph = (struct iphdr *)skb_network_header(e->skb);
            printk(KERN_INFO "Comparing with packet from %s to %s, queue length is %d", inet_ntoa(iph->saddr), inet_ntoa(iph->daddr), q->len);
        }*/

        if (!cmp || cmp(e, data))
        {
            //list_del(&(e->entry)->list);
            list_del(&e->list);
            q->len--;
            //spin_unlock_bh(&q->lock);
            write_unlock_bh(&q_lock);
            return e;
        }
    }
    write_unlock_bh(&q_lock);
    //spin_unlock_bh(&q->lock);
    return NULL;
}


/* Flush all packets */
static void ipq_flush()
{
    struct nf_queue_entry *e;
    int verdict = NF_DROP;

    //spin_lock_bh(&q->lock);
    write_lock_bh(&q_lock);
    q->flushing = 1;
    //spin_unlock_bh(&q->lock);
    write_unlock_bh(&q_lock);
    //printk(KERN_INFO "before ipq_dequeue, queue length is %d\n", q->len);
    while ((e = ipq_dequeue( NULL, 0)))
    {
        if (e->hook == NF_INET_POST_ROUTING)
        //if (e->hook == NF_INET_LOCAL_OUT)
        {
            struct iphdr *iph = (struct iphdr *)skb_network_header(e->skb);
            //printk(KERN_INFO "Dequeuing packet to %s, queue length is %d", inet_ntoa(iph->daddr), q->len);
        }

        //e->verdict = NF_DROP;
        //printk(KERN_INFO "before nf_reinject\n");
        nf_reinject(e, verdict);
        //printk(KERN_INFO "after nf_reinject\n");
        //kfree(e);
    }
    //spin_lock_bh(&q->lock);
    write_lock_bh(&q_lock);
    q->flushing = 0;
    //spin_unlock_bh(&q->lock);
    write_unlock_bh(&q_lock);
}


static int ipq_create_queue(int *sysctl_qmax)
{
    int status;

    q = kmalloc(sizeof(ipq_queue_t), GFP_KERNEL);
    if (q == NULL)
    {
        return 0;
    }

    q->len = 0;
    q->maxlen = sysctl_qmax;
    q->flushing = 0;
    q->terminate = 0;
    //INIT_LIST_HEAD(&q->list);
    //spin_lock_init(&q->lock);

    status = nf_register_queue_handler(PF_INET, &nf_handler);
    //status = nf_register_queue_handler(NFPROTO_IPV4, &nf_handler);
    if (status < 0)
    {
        printk("Could not create Packet Queue! %d\n",status);
        kfree(q);
        return 0;
    }
    return 1;
}

static int ipq_enqueue(ipq_queue_t *g,struct nf_queue_entry *entry)
{
    //printk(KERN_INFO "entering ipq_enqueue\n");
    
    //ipq_queue_element_t *e;

    if (q!=g)
    {
        printk("trying to enqueue but do not have right queue!!!\n");
        return 0;
    }

    /*struct nf_queue_entry *e;
    e = kmalloc(sizeof(*e), GFP_ATOMIC);
    if (e == NULL)
    {
        printk(KERN_ERR "ip_queue: OOM in enqueue\n");
        return -ENOMEM;
    }
    e = entry;*/

    //e->verdict = NF_DROP;
    //e->entry = entry;

    /*if (entry->hook == NF_INET_POST_ROUTING)
    //if (e->hook == NF_INET_LOCAL_OUT)
    {
        //struct iphdr *iph = skb->nh.iph;
        struct iphdr *iph = (struct iphdr *)skb_network_header(entry->skb);
        printk(KERN_INFO "1-Enqueuing packet from %s to %s, queue length is %d", inet_ntoa(iph->saddr), inet_ntoa(iph->daddr), q->len);
        //e->rt_info.tos = iph->tos;
        //e->rt_info.daddr = iph->daddr;
        //e->rt_info.saddr = iph->saddr;
    }*/

    //spin_lock_bh(&q->lock);
    write_lock_bh(&q_lock);
    if (q->len >= *q->maxlen)
    {
        //spin_unlock_bh(&q->lock);
        write_unlock_bh(&q_lock);
        if (net_ratelimit())
            printk(KERN_WARNING "ip_queue: full at %d entries, "
                   "dropping packet(s).\n", q->len);
        goto free_drop;
    }

    if (q->flushing || q->terminate)
    {
        //spin_unlock_bh(&q->lock);
        write_unlock_bh(&q_lock);
        goto free_drop;
    }

    //list_add(&(e->entry)->list, &q->list);
    //list_add(&entry->list, &q->list);
    list_add_tail(&entry->list, &q_list);
    //list_add_tail(&e->list, &q->list);
    q->len++;
    //spin_unlock_bh(&q->lock);
    write_unlock_bh(&q_lock);

    if (entry->hook == NF_INET_POST_ROUTING)
    //if (e->hook == NF_INET_LOCAL_OUT)
    {
        struct iphdr *iph = (struct iphdr *)skb_network_header(entry->skb);
        //printk(KERN_INFO "Enqueuing packet to %s, queue length is %d", inet_ntoa(iph->daddr), q->len);
    }

    //printk(KERN_INFO "Enqueuing packet to %s, queue length is %d\n", inet_ntoa(e->rt_info.daddr), q->len);

    struct list_head *i;
    //spin_lock_bh(&q->lock);
    write_lock_bh(&q_lock);

    //for (i = q->list.prev; i != &q->list; i = i->prev)
    /*for (i = q_list.prev; i != &q_list; i = i->prev)
    //list_for_each_entry(i, &q_list, list)
    {
        //ipq_queue_element_t *e = (ipq_queue_element_t *)i;
        struct nf_queue_entry *e = (struct nf_queue_entry *)i;
        //ipq_queue_element_t *e = i;

        if (e->hook == NF_IP_POST_ROUTING)
        {
            struct iphdr *iph = (struct iphdr *)skb_network_header(e->skb);
            printk(KERN_INFO "Comparing with packet from %s to %s, queue length is %d", inet_ntoa(iph->saddr), inet_ntoa(iph->daddr), q->len);
        }
    }*/

    //list_for_each_entry(i, &q, list)
    //for (i = q->list.prev; i != &q->list; i = i->prev)
    /*for (i = q->list.next; i != &q->list; i = i->next)
    {
        ipq_queue_element_t *e = (ipq_queue_element_t *)i;
        //ipq_queue_element_t *e = i;
        printk(KERN_INFO "Comparing with packet from %s to %s, queue length is %d", inet_ntoa(e->rt_info.saddr), inet_ntoa(e->rt_info.daddr), q->len);
    }*/
    //spin_unlock_bh(&q->lock);
    write_unlock_bh(&q_lock);

    return q->len;

free_drop:
    kfree(entry);
    //kfree(entry);
    //printk(KERN_INFO "ipq_enqueue free drop\n");
    return -EBUSY;

}

static void ipq_destroy_queue()
{
    //nf_unregister_queue_handler(NFPROTO_IPV4, &nf_handler);
    //nf_unregister_queue_handlers(&nf_handler);
    nf_unregister_queue_handler(PF_INET, &nf_handler);
    //printk(KERN_INFO "after nf_unregister_queue_handler\n");

    //spin_lock_bh(&q->lock);
    write_lock_bh(&q_lock);
    q->terminate = 1;
    //spin_unlock_bh(&q->lock);
    write_unlock_bh(&q_lock);
    ipq_flush();
    //printk(KERN_INFO "after ipq_flush\n");
    kfree(q);
}

/* With a chainsaw... */
/*static int route_me_harder(struct sk_buff *skb)
{
    //struct iphdr *iph = skb->nh.iph;
    struct net *net = dev_net(skb_dst(skb)->dev);
    struct iphdr *iph = (struct iphdr *) skb_network_header(skb);
    struct rtable *rt;
    struct flowi fl;
    //struct rt_key key = {
    fl.nl_u.ip4_u.daddr = iph->daddr;
    fl.nl_u.ip4_u.saddr = iph->saddr;
    fl.oif = skb->sk? skb->sk->__sk_common.skc_bound_dev_if : 0;
    fl.nl_u.ip4_u.tos = RT_TOS(iph->tos)|RTO_CONN;
    //dst:iph->daddr, src:iph->saddr,
    //oif:skb->sk ? skb->sk->bound_dev_if : 0,
    //tos:RT_TOS(iph->tos)|RTO_CONN,

#ifdef CONFIG_IP_ROUTE_FWMARK
    //fwmark:skb->nfmark
    fl.mark = skb->nfmark;
#endif
    //};

    if (ip_route_output_key(net, &rt, &fl) != 0) {
        printk("route_me_harder: No more route.\n");
        return -EINVAL;
    }*/

    /* Drop old route. */
    //dst_release(skb->dst);
    //skb->dst = &rt->u.dst;
    /*skb_dst_drop(skb);
    skb_dst_set(skb, &rt->u.dst);
    return 0;
}*/

//static inline int id_cmp(ipq_queue_element_t *e,u_int32_t id)
/*static inline int id_cmp(struct nf_queue_entry *e,u_int32_t id)
{
    return (id == ((unsigned long)(e->id)));
}*/

//static inline int dev_cmp(ipq_queue_element_t *e, u_int32_t ifindex)
static inline int dev_cmp(struct nf_queue_entry *e, u_int32_t ifindex)
{
    if (e->indev)
        if (e->indev->ifindex == ifindex)
            return 1;
    if (e->outdev)
        if (e->outdev->ifindex == ifindex)
            return 1;
    return 0;
}

//static inline int ip_cmp(ipq_queue_element_t *e, u_int32_t ip)
static inline int ip_cmp(struct nf_queue_entry *e, u_int32_t ip)
{
    if (e->hook == NF_INET_POST_ROUTING)
    //if (e->hook == NF_INET_LOCAL_OUT)
    {
        struct iphdr *iph = (struct iphdr *)skb_network_header(e->skb);
        if (iph->daddr == ip)
            return 1;
    }

    /*if (e->rt_info.daddr == ip)
        return 1;*/
    return 0;
}


/* Drop any queued packets associated with device ifindex */
static void ipq_dev_drop(int ifindex)
{
    //ipq_queue_element_t *e;
    struct nf_queue_entry *e;
    int verdict = NF_DROP;

    while ((e = ipq_dequeue(dev_cmp, ifindex)))
    {
        //e->verdict = NF_DROP;
        nf_reinject(e, verdict);
        //kfree(e);
    }
}

void ipq_send_ip(u_int32_t ip)
{
    //ipq_queue_element_t *e;
    struct nf_queue_entry *e;
    //printk(KERN_INFO "entering ipq_send_ip\n");
    int verdict = NF_ACCEPT;
    while ((e = ipq_dequeue(ip_cmp, ip)))
    {
        //printk(KERN_INFO "after ipq_dequeue\n");
        //e->verdict = NF_ACCEPT;
	
        //e->entry->skb->nfcache |= NFC_ALTERED;
        //route_me_harder(e->skb);
        ip_route_me_harder(e->skb, RTN_LOCAL);
        //printk(KERN_INFO "after route_me_harder");
        nf_reinject(e, verdict);
        //printk(KERN_INFO "after nf_reinject\n");

        //kfree(e);
    }
}

void ipq_drop_ip(u_int32_t ip)
{
    //ipq_queue_element_t *e;
    struct nf_queue_entry *e;
    int verdict = NF_DROP;

    printk(KERN_INFO "Dropping packets for unreachable destination\n");

    while ((e = ipq_dequeue( ip_cmp, ip)))
    {
        if (e->hook == NF_INET_POST_ROUTING)
        //if (e->hook == NF_INET_LOCAL_OUT)
        {
            struct iphdr *iph = (struct iphdr *)skb_network_header(e->skb);
            //printk(KERN_INFO "Dequeuing packet to %s, queue length is %d", inet_ntoa(iph->daddr), q->len);
        }

        //printk(KERN_INFO "Dequeuing packet to %s, queue length is %d\n", inet_ntoa(e->rt_info.daddr), q->len);
        //e->verdict = NF_DROP;
	icmp_send(e->skb,ICMP_DEST_UNREACH,ICMP_HOST_UNREACH,0);
        //kfree_skb(e->skb);
	nf_reinject(e, verdict);
        //kfree(e);
    }
    //printk(KERN_INFO "ipq_drop_ip finished\n");
}


/****************************************************************************
 *
 * Netfilter interface
 *
 ****************************************************************************/

/*
 * Packets arrive here from netfilter for queuing to userspace.
 * All of them must be fed back via nf_reinject() or Alexey will kill Rusty.
 */
/*int ipq_insert_packet(struct nf_queue_entry *entry)
{
    return ipq_enqueue(q, entry);
}*/


//static int netfilter_receive(struct sk_buff *skb,
//                             struct nf_info *info, void *data)
static int netfilter_receive(struct nf_queue_entry *entry, unsigned int queuenum)
{
    //return ipq_enqueue((ipq_queue_t *)queuenum, entry);
    //return ipq_insert_packet(entry);
    return ipq_enqueue(q, entry);
}

/****************************************************************************
 *
 * System events
 *
 ****************************************************************************/

static int receive_event(struct notifier_block *this,
                         unsigned long event, void *ptr)
{
    struct net_device *dev = ptr;

    /* Drop any packets associated with the downed device */
    if (event == NETDEV_DOWN)
        ipq_dev_drop(dev->ifindex);
    return NOTIFY_DONE;
}

struct notifier_block ipq_dev_notifier = {
            receive_event,
            NULL,
            0
};

/****************************************************************************
 *
 * Sysctl - queue tuning.
 *
 ****************************************************************************/

static int sysctl_maxlen = IPQ_QMAX_DEFAULT;

/*static struct ctl_table_header *ipq_sysctl_header;

static ctl_table ipq_table[] = {
                                   { NET_IPQ_QMAX, NET_IPQ_QMAX_NAME, &sysctl_maxlen,
                                     sizeof(sysctl_maxlen), 0644,  NULL, proc_dointvec },
                                   { 0 }
                               };

static ctl_table ipq_dir_table[] = {
                                       {NET_IPV4, "ipv4", NULL, 0, 0555, ipq_table, 0, 0, 0, 0, 0},
                                       { 0 }
                                   };

static ctl_table ipq_root_table[] = {
                                        {CTL_NET, "net", NULL, 0, 0555, ipq_dir_table, 0, 0, 0, 0, 0},
                                        { 0 }
                                    };*/

/****************************************************************************
 *
 * Module stuff.
 *
 ****************************************************************************/

int init_packet_queue(void)
{
    int status = 0;
    //struct proc_dir_entry *proc;

    status= ipq_create_queue(&sysctl_maxlen);
    if (status == 0) {
        printk(KERN_ERR "ip_queue: initialisation failed: unable to "
               "create queue\n");

        return status;
    }
    /*proc = proc_net_create(IPQ_PROC_FS_NAME, 0, ipq_get_info);
    if (proc) proc->owner = THIS_MODULE;
    else
    {
    	ipq_destroy_queue(nlq);
    	sock_release(nfnl->socket);
    	return -ENOMEM;
    }*/

    //ipq_sysctl_header = register_sysctl_table(ipq_root_table);
    return status;
}

void cleanup_packet_queue(void)
{
    //unregister_sysctl_table(ipq_sysctl_header);
    //proc_net_remove(IPQ_PROC_FS_NAME);

    ipq_destroy_queue();
}

