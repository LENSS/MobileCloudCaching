/***************************************************************************
                          aodv_dev.c  -  description
                             -------------------
    begin                : Thu Aug 7 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "aodv_dev.h"

//extern char *g_aodv_dev;
extern char g_aodv_dev[8];
extern aodv_route *g_my_route;
extern u_int32_t g_my_ip;

aodv_dev *aodv_dev_list;

aodv_dev *create_aodv_dev(struct net_device *dev, struct in_ifaddr *ifa)
{
    aodv_dev *new_dev;
    aodv_route *tmp_route;

    tmp_route = create_aodv_route(ifa->ifa_address);

    tmp_route->ip = ifa->ifa_address;
    //tmp_route->netmask = calculate_netmask(0); //ifa->ifa_mask;
    tmp_route->self_route = 1;
    tmp_route->seq = 1;
    tmp_route->old_seq = 0;
    tmp_route->rreq_id = 1;
    tmp_route->metric = 0;
    tmp_route->next_hop = tmp_route->ip;
    tmp_route->lifetime = -1;
    tmp_route->route_valid = 1;
    tmp_route->route_seq_valid = 1;
    tmp_route->dev = dev;

    if ((new_dev = (aodv_dev *) kmalloc(sizeof(aodv_dev), GFP_ATOMIC)) == NULL)
    {
        /* Couldn't create a new entry in the routing table */
        printk(KERN_WARNING "AODV: Not enough memory to create Route Table Entry\n");
        return NULL;
    }


    new_dev->ip = tmp_route->ip;
    new_dev->netmask = ifa->ifa_mask;
    new_dev->route_entry = tmp_route;
    new_dev->dev = dev;
    new_dev->next=NULL;
    

    g_my_route = tmp_route;
    g_my_ip = g_my_route->ip;

    insert_kernel_route_entry(tmp_route->ip, tmp_route->next_hop, tmp_route->netmask, dev->name);


    return new_dev;

}

int insert_aodv_dev(struct net_device *dev)
{
    //printk(KERN_INFO "dev name: %s\n", dev->name);
    aodv_dev *new_dev;
    int success=0, error=0;
    struct in_ifaddr *ifa;
    struct in_device *in_dev;
    char netmask[16];
    mm_segment_t oldfs;


    //read_lock(&in_dev->mc_list_lock);

    if ((in_dev=in_dev_get(dev)) == NULL)
    {
        //printk(KERN_INFO "in_dev 0\n");
        //read_unlock(&in_dev->mc_list_lock);
	return 0;
    }

    read_lock(&in_dev->mc_list_lock);
    /*if ((in_dev->ifa_list) == NULL)
    {
        printk("%s: ifa list empty\n", dev->name);
    }*/
    for_primary_ifa(in_dev)
    {
        //printk(KERN_INFO "entering for_primary ifa\n");
        if ((strcmp(g_aodv_dev,"")!=0) && (strncmp(dev->name, g_aodv_dev, IFNAMSIZ) == 0))
        {				
            if (ifa==NULL)
            {
                //printk(KERN_INFO "ifa 0\n");
                read_unlock(&in_dev->mc_list_lock);
           	return success;
            }

            new_dev = create_aodv_dev(dev, ifa);
            strcpy(netmask, inet_ntoa(new_dev->netmask & new_dev->ip));
            printk(KERN_INFO "INTERFACE LIST: Adding interface: %s  IP: %s Subnet: %s\n", dev->name, inet_ntoa(ifa->ifa_address), netmask);

            strncpy(new_dev->name, dev->name, IFNAMSIZ);

            new_dev->next = aodv_dev_list;
            aodv_dev_list = new_dev;
            
            oldfs = get_fs();
            set_fs(KERNEL_DS);
            error = sock_create(PF_INET, SOCK_DGRAM, 0, &(new_dev->sock));
            set_fs(oldfs);
            if (error < 0)
            {
                kfree(new_dev);
                read_unlock(&in_dev->mc_list_lock);
                printk(KERN_ERR "Error during creation of socket; terminating, %d\n", error);
                return error;
            }

            init_sock(new_dev->sock, new_dev->ip, dev->name);
            success++;
        }
	
    } endfor_ifa(in_dev);
    read_unlock(&in_dev->mc_list_lock);
    return success;
}




int init_aodv_dev()
{
    aodv_route *tmp_route;
    aodv_dev *new_dev = NULL;
    struct net_device *dev;
    struct in_device *tmp_indev;
    struct in_device *in_dev;
    int result = 0;
    uint32_t final = 0;
    //int error;

    //dev_base is a kernel variable pointing to a list of
    //all the available netdevices and is maintained
    //by the kernel

    aodv_dev_list = NULL;
    read_lock(&dev_base_lock);
    rcu_read_lock();
    //read_lock(&inetdev_lock);
    //printk(KERN_INFO "before searching netdevice\n");
    //for_each_netdev(&init_net, dev)
    const char *dev_name = g_aodv_dev;
    dev = dev_get_by_name(&init_net, dev_name);
    result = insert_aodv_dev(dev);
    /*for (dev = dev_base; dev!=NULL; dev = dev->next)
    {
        i=i+insert_aodv_dev(dev);
    }*/
    //read_unlock(&inetdev_lock);
    rcu_read_unlock();
    read_unlock(&dev_base_lock);

    if((result < 0) || (result == 0))
    {
    	printk("Unable to locate device: %s, Bailing!\n",g_aodv_dev);
     	return 0;
     }
    insert_timer(TASK_HELLO, HELLO_INTERVAL, g_my_ip);
    update_timer_queue();
    
    return result;
}




aodv_dev *first_aodv_dev()
{
    return aodv_dev_list;
}



/****************************************************

   find_dev_ip
----------------------------------------------------
It will find the IP for a dev
****************************************************/
u_int32_t find_dev_ip(struct net_device * dev)
{
    struct in_device *tmp_indev;

    //make sure we get a valid DEV
    if (dev == NULL)
    {
        printk(KERN_WARNING "AODV: FIND_DEV_IP gotta NULL DEV! ");
        return -EFAULT;
    }
    //make sure that dev has an IP section
    if (dev->ip_ptr == NULL)
    {
        printk(KERN_WARNING "AODV: FIND_DEV_IP gotta NULL ip_ptr!! ");
        return -EFAULT;
    }
    //find that ip!
    tmp_indev = (struct in_device *) dev->ip_ptr;
    if (tmp_indev && (tmp_indev->ifa_list != NULL))
        return (tmp_indev->ifa_list->ifa_address);
    else
        return 0;
}


aodv_dev *find_aodv_dev_by_dev(struct net_device * dev)
{
    aodv_dev *tmp_dev = aodv_dev_list;
    struct in_device *tmp_indev;
    u_int32_t tmp_ip;


    //Make sure the dev is legit
    if (dev == NULL)
    {
        printk(KERN_WARNING "AODV: FIND_INTERFACE_BY_DEV gotta NULL DEV! \n");
        return NULL;
    }
    //make sure it has a legit ip section
    if (dev->ip_ptr == NULL)
    {
        printk(KERN_WARNING "AODV: FIND_INTERFACE_BY_DEV gotta NULL ip_ptr!! \n");
        return NULL;
    }
    //find the ip address for the dev
    tmp_indev = (struct in_device *) dev->ip_ptr;

    if (tmp_indev->ifa_list == NULL)
    {
        printk(KERN_WARNING "AODV: FIND_INTERFACE_BY_DEV gotta NULL ifa_list!! \n");
        return NULL;
    }

    tmp_ip = tmp_indev->ifa_list->ifa_address;

    //search the interface list for a device with the same ip
    while (tmp_dev != NULL)
    {
        if (tmp_dev->ip == (u_int32_t) tmp_ip)
            return tmp_dev;
        tmp_dev = tmp_dev->next;
    }

    printk(KERN_WARNING
           "AODV: Failed search for matching interface for: %s which has an ip of: %s\n", dev->name, inet_ntoa(tmp_ip));

    return NULL;
}

/*
struct interface_list_entry *find_interface_by_ip(u_int32_t ip)
{
    struct interface_list_entry *tmp_entry = interface_list;

    //go through the whole list
    while (tmp_entry != NULL)
      {
	  //if it finds a match
	  if (tmp_entry->ip == ip)
	      return tmp_entry;
	  tmp_entry = tmp_entry->next;
      }
    return NULL;
}
*/
