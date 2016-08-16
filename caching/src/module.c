
#include "module.h"
 
struct nf_hook_ops input_filter;

extern struct timer_list cache_timer;

u_int32_t   g_my_ip;
u_int32_t 	g_broadcast_ip;

//char *use_dev;
char *act;
int kval;
int nval;
int theta;
int buf;

//module_param(use_dev, charp, 0);
module_param(act, charp, 0);
module_param(kval, int, 0);
module_param(nval, int, 0);
module_param(theta, int, 0);
module_param(buf, int, 0);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YING FENG");
MODULE_DESCRIPTION("A Cache Daemon kernel module");

char g_dev[8];
char g_act[8];
int KVAL;
int NVAL;
int THRESHOLD;
int BUF_SIZE;
struct socket *g_sock;
u_int32_t	cEnergy;
u_int32_t	cOverhead;
u_int32_t	fOverhead;
u_int32_t	cSuccess;
u_int32_t	cPrefetch;
u_int32_t	rtHeader;
u_int32_t	rtHello;
u_int32_t	rtHello_c;
u_int32_t	rtTc;
u_int32_t	aodv_hello;
u_int32_t	aodv_else;

//static struct proc_dir_entry *cache_dir, *cache_ctable_proc, *cache_creq_proc, *cache_energy_proc, *cache_overhead_proc, *cache_success_proc, *cache_prefetch_proc, *cache_olsr_proc;

static struct proc_dir_entry *cache_dir, *cache_ctable_proc, *cache_creq_proc, *cache_metric_proc;

/****************************************************

	init_module
----------------------------------------------------
This is called by insmod when things get
started up!
****************************************************/

int init_module(void)
{    
    if (act==NULL)
    {
    	printk("You need to specify whether to turn on caching, ie:\n");
      	printk("    insmod caching act=on    \n");
       	return(-1);
        strcpy(g_act, "");
    }
    else
        strcpy(g_act, act);

    //if (act) strcpy(g_act, act);
    //else strcpy(g_act, "");
    KVAL = kval;
    NVAL = nval;
    THRESHOLD = theta;
    BUF_SIZE = buf;

    cEnergy = 0;
    cOverhead = 0;
    fOverhead = 0;
    cSuccess = 0;
    cPrefetch = 0;
    rtHeader = 0;
    rtHello = 0;
    rtHello_c = 0;
    rtTc = 0;
    aodv_hello = 0;
    aodv_else = 0;

    inet_aton("255.255.255.255", &g_broadcast_ip);

    strcpy(g_dev, "wlan0");    
    const char *dev_name = g_dev;
    struct net_device *dev = dev_get_by_name(&init_net, dev_name);

    g_my_ip = find_dev_ip(dev);
    //printk("my device ip: %s\n", inet_ntoa(g_my_ip));

    printk(KERN_INFO "\n-=: Caching Daemon Initiated :=-\n");
    printk(KERN_INFO "---------------------------------------------\n");

    // netfilter stuff
    input_filter.list.next = NULL;
    input_filter.list.prev = NULL;
    input_filter.hook = input_handler;
    input_filter.owner = THIS_MODULE;
    input_filter.pf = PF_INET; // IPv4
    input_filter.hooknum = NF_INET_PRE_ROUTING;
    input_filter.priority = NF_IP_PRI_FILTER;

    //printk("after preparing input filter\n");

    /*if ((g_sock = (struct socket *) kmalloc(sizeof(struct socket), GFP_ATOMIC)) == NULL)
    {
        // Couldn't create a new socket
        printk(KERN_WARNING "Cache Daemon: Not enough memory to create a new socket\n");
        return NULL;
    }*/

    init_sock(g_my_ip, g_dev);
    
    init_task_queue();
    init_timer_queue();
    init_flood_id_queue();
    init_cache_table();
    init_req_list();
    init_rinfo();
    init_extable();

    startup_cache();

    nf_register_hook(&input_filter);

    insert_timer(TASK_CLEAN_UP, FLUSH_INTERVAL, g_my_ip, 0, 0);
    update_timer_queue();

    cache_dir = proc_mkdir("cache", NULL);
    cache_ctable_proc = create_proc_read_entry("cache/ctable", 0, NULL, read_ctable_proc, NULL);
    cache_creq_proc = create_proc_read_entry("cache/creq", 0, NULL, read_creq_proc, NULL);
    /*cache_energy_proc = create_proc_read_entry("cache/energy", 0, NULL, read_energy_proc, NULL);
    cache_overhead_proc = create_proc_read_entry("cache/overhead", 0, NULL, read_overhead_proc, NULL);
    cache_success_proc = create_proc_read_entry("cache/success", 0, NULL, read_success_proc, NULL);
    cache_prefetch_proc = create_proc_read_entry("cache/prefetch", 0, NULL, read_prefetch_proc, NULL);
    cache_olsr_proc = create_proc_read_entry("cache/rt", 0, NULL, read_rt_proc, NULL);*/
    cache_metric_proc = create_proc_read_entry("cache/metric", 0, NULL, read_metric_proc, NULL);

    //printk("after init_module\n");

    return 0;
}

/****************************************************

   cleanup_module
----------------------------------------------------
cleans up the module. called by rmmod
****************************************************/

void cleanup_module(void)
{
    remove_proc_entry("cache/ctable", NULL);
    remove_proc_entry("cache/creq", NULL);
    /*remove_proc_entry("cache/energy", NULL);
    remove_proc_entry("cache/overhead", NULL);
    remove_proc_entry("cache/success", NULL);
    remove_proc_entry("cache/prefetch", NULL);
    remove_proc_entry("cache/rt", NULL);*/
    remove_proc_entry("cache/metric", NULL);
    remove_proc_entry("cache", NULL);

    printk("Cache Daemon: Shutting down...\n");

    nf_unregister_hook(&input_filter);
  
    printk("Cache Daemon: Unregistered NetFilter hooks...\n");

    sock_release(g_sock);

    printk("Cache Daemon: Closed sockets...\n");

    del_timer(&cache_timer);
    printk("Cache Daemon: Removed timer...\n");

    kill_cache();
    printk("Cache Deamon: Killed cache thread...\n");

    cleanup_task_queue();
    cleanup_flood_id_queue();
    cleanup_cache_table();
    cleanup_req_list();
    cleanup_rinfo();
    cleanup_extable();

    printk("Cache Daemon: Shutdown complete!\n");
}


