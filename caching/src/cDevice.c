
#include "cDevice.h"

extern char g_dev[8];
extern u_int32_t g_my_ip;


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
        printk(KERN_WARNING "Cache Daemon: FIND_DEV_IP gotta NULL DEV! ");
        return -EFAULT;
    }
    //make sure that dev has an IP section
    if (dev->ip_ptr == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: FIND_DEV_IP gotta NULL ip_ptr!! ");
        return -EFAULT;
    }
    //find that ip!
    tmp_indev = (struct in_device *) dev->ip_ptr;
    if (tmp_indev && (tmp_indev->ifa_list != NULL))
        return (tmp_indev->ifa_list->ifa_address);
    else
        return 0;
}

