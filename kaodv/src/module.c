/***************************************************************************
                          module.c  -  description
                             -------------------
    begin                : Wed Aug 20 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "module.h"
 
struct nf_hook_ops input_filter;
struct nf_hook_ops output_filter;

extern struct timer_list aodv_timer;

aodv_route *g_my_route;  
u_int32_t   g_my_ip;  
u_int32_t 	g_broadcast_ip;
u_int32_t	aodv_overhead;

#ifdef LINK_LIMIT
int		g_link_limit = 15; //signal - noise, needs to be above this to be a neighbor...
#endif

//int32_t	g_score_threshold = HELLO_INTERVAL;
//u_int64_t   g_last_hello;
u_int32_t g_local_subnet;
u_int8_t g_aodv_gateway;
static struct proc_dir_entry *aodv_dir, *route_table_proc, *neigh_proc;

char *use_dev;
//char *aodv_subnet;
u_int8_t aodv_gateway;
char *local_subnet;

module_param(use_dev, charp, 0);
module_param(aodv_gateway, int, 0);
module_param(local_subnet, charp, 0);
//MODULE_PARM(use_dev,"s");
//MODULE_PARM(aodv_gateway,"i");
//MODULE_PARM(local_subnet,"s");



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luke Klein-Berndt");
MODULE_DESCRIPTION("A AODV ad-hoc routing kernel module");



char g_aodv_dev[8];


/****************************************************

	init_module
----------------------------------------------------
This is called by insmod when things get
started up!
****************************************************/

int init_module(void)
{
    int result;
    char* local_subnet_netmask_str;
    u_int32_t local_subnet_ip=0;
    u_int32_t local_subnet_netmask=0;
    aodv_route *tmp_route;
    
    
    if (use_dev==NULL)
    {
    	printk("You need to specify which device to use, ie:\n");
      	printk("    insmod kernel_aodv use_dev=wlan0    \n");
       	return(-1);
        strcpy(g_aodv_dev,"");
    }
    else
        strcpy(g_aodv_dev,use_dev);

    g_aodv_gateway = aodv_gateway;

    aodv_overhead = 0;

    inet_aton("255.255.255.255",&g_broadcast_ip);
    //inet_aton("192.168.0.255", &g_broadcast_ip);
    printk("\n-=: Kernel AODV v2.2 :=-\nLuke Klein-Berndt\nWireless Communications Technologies Group\nNational Institue of Standards and Technology\n");
    printk("---------------------------------------------\n");
    
    //init_aodv_dev();
    
    //netfilter stuff
    
    // input hook 
    input_filter.list.next = NULL;
    input_filter.list.prev = NULL;
    input_filter.hook = input_handler;
    input_filter.owner = THIS_MODULE;
    input_filter.pf = PF_INET; // IPv4
    input_filter.hooknum = NF_INET_PRE_ROUTING;
    input_filter.priority = NF_IP_PRI_FILTER;

    // output hook 
    output_filter.list.next = NULL;
    output_filter.list.prev = NULL;
    output_filter.hook = output_handler;
    output_filter.owner = THIS_MODULE;
    output_filter.pf = PF_INET; // IPv4
    //output_filter.pf = NFPROTO_IPV4;
    output_filter.hooknum = NF_INET_POST_ROUTING;
    output_filter.priority = NF_IP_PRI_FILTER;


    init_aodv_route_table();

    if (local_subnet==NULL)
        g_local_subnet=0;
    else
    {
    	local_subnet_netmask_str = strchr(local_subnet,'/');
      	if ( local_subnet_netmask_str)
       	{	
            (*local_subnet_netmask_str)='\0';
            local_subnet_netmask_str++;
            printk("Adding Localy Routed Subnet %s/%s to the routing table...\n",local_subnet, local_subnet_netmask_str);
	    inet_aton(local_subnet, &local_subnet_ip);
	    inet_aton(local_subnet_netmask_str, &local_subnet_netmask);
	    tmp_route = create_aodv_route(local_subnet_ip);
	    tmp_route->route_valid = TRUE;
	    tmp_route->route_seq_valid = TRUE;
	    tmp_route->netmask = local_subnet_netmask;
	    tmp_route->self_route = TRUE;
	    tmp_route->metric = 1;
	    tmp_route->seq = 1;
	    tmp_route->rreq_id = 1;
	 }
    }
    
    init_task_queue();
    init_timer_queue();
    init_flood_id_queue();
    init_aodv_neigh_list();
    init_packet_queue();

    /*init_aodv_dev();
    startup_aodv();
    printk(KERN_INFO "before return\n");
    return 0;
    printk(KERN_INFO "after init_aodv_dev\n");*/

    
    if (!init_aodv_dev())
    {
   	del_timer(&aodv_timer);
        printk("Kernel_AODV: Removed timer...\n");
        printk("Kernel_AODV: Killed router thread...\n");
	cleanup_packet_queue();
        cleanup_task_queue();
        cleanup_flood_id_queue();
	printk("Kernel_AODV: Cleaned up AODV Queues...\n");
        cleanup_aodv_route_table();
	return(-1);
    }

#ifdef AODV_SIGNAL
    //init_iw_sock();
    printk("Kernel_AODV: A Maximum of: %d neighboring nodes' signals can be monitored\n",IW_MAX_SPY);
#endif

    startup_aodv();

    result = nf_register_hook(&output_filter);
    result = nf_register_hook(&input_filter);
    
#ifdef MESSAGES
    printk("Kernel_AODV: Principal IP address - %s\n",inet_ntoa(g_my_ip));
    if (g_aodv_gateway)
    {
    	printk("Kernel_AODV: Set as AODV Gateway\n");
    }
#endif


    insert_timer( TASK_CLEANUP, HELLO_INTERVAL, g_my_ip);
    update_timer_queue();


    aodv_dir = proc_mkdir("aodv",NULL);
    //aodv_dir->owner = THIS_MODULE;
    
    route_table_proc=create_proc_read_entry("aodv/routes", 0, NULL, read_route_table_proc, NULL);
    //route_table_proc->owner=THIS_MODULE;

    route_table_proc=create_proc_read_entry("aodv/neigh", 0, NULL, read_neigh_proc, NULL);
    //route_table_proc->owner=THIS_MODULE;
  
    route_table_proc=create_proc_read_entry("aodv/timers", 0, NULL, read_timer_queue_proc, NULL);
    //route_table_proc->owner=THIS_MODULE;

    route_table_proc=create_proc_read_entry("aodv/flood_id", 0, NULL, read_flood_id_proc, NULL);
    //route_table_proc->owner=THIS_MODULE;

    route_table_proc=create_proc_read_entry("aodv/overhead", 0, NULL, read_overhead_proc, NULL);
    //printk(KERN_INFO "init_module finished\n");
    return 0;
}


/****************************************************

   cleanup_module
----------------------------------------------------
cleans up the module. called by rmmod
****************************************************/
void cleanup_module(void)
{
    remove_proc_entry("aodv/routes",NULL);
    remove_proc_entry("aodv/flood_id",NULL);
    remove_proc_entry("aodv/timers",NULL);
    remove_proc_entry("aodv/neigh",NULL);
    remove_proc_entry("aodv/overhead",NULL);
    remove_proc_entry("aodv", NULL);

    printk("Kernel_AODV: Shutting down...\n");

    nf_unregister_hook(&input_filter);
    nf_unregister_hook(&output_filter);
  
    printk("Kernel_AODV: Unregistered NetFilter hooks...\n");

    close_sock();

//#ifdef AODV_SIGNAL
    //close_iw_sock();
//#endif

    printk("Kernel_AODV: Closed sockets...\n");

    del_timer(&aodv_timer);
    printk("Kernel_AODV: Removed timer...\n");


    kill_aodv();
    printk("Kernel_AODV: Killed router thread...\n");


    cleanup_task_queue();
    cleanup_flood_id_queue();
    cleanup_packet_queue();
    printk("Kernel_AODV: Cleaned up AODV Queues...\n");

    cleanup_aodv_route_table();
    printk("Kernel_AODV: Cleaned up Route Table...\n");

    printk("Kernel_AODV: Shutdown complete!\n");
}
