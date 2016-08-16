/***************************************************************************
                          signal.c  -  description
                             -------------------
    begin                : Thu Aug 7 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/

#include "signal.h"

static struct socket *iw_sock;

#ifdef AODV_SIGNAL

/*void init_iw_sock(void)
{
    int error;

    error = sock_create(AF_INET, SOCK_DGRAM, 0, &iw_sock);
    if (error < 0)
    {
        printk(KERN_ERR "Error during creation of socket; terminating, %d\n", error);
    }
}

void close_iw_sock(void)
{
    sock_release(iw_sock);
}*/

int set_spy()
{
    //printk(KERN_INFO "entering set_spy\n");
    int errno;
    int i;
    aodv_neigh *tmp_neigh;
    aodv_dev *tmp_dev;
    struct sockaddr iw_sa[IW_MAX_SPY];
    struct iwreq wrq;
    
#if WIRELESS_EXT > 15
    iw_handler handler;
    struct iw_request_info info;
#endif

    tmp_dev = first_aodv_dev();
    while (tmp_dev != NULL)
    {
#if WIRELESS_EXT > 15
    if ( tmp_dev->dev->wireless_handlers )
#else
    if ((tmp_dev->dev->get_wireless_stats!=NULL) && (tmp_dev->dev->do_ioctl!=NULL))
#endif
    {
        i = 0;

        tmp_neigh = first_aodv_neigh();
        while (tmp_neigh != NULL)
        {
            if (tmp_dev->dev == tmp_neigh->dev)
            {
                if (i < IW_MAX_SPY)
                {
                    memcpy((char *) &(iw_sa[i].sa_data), (char *) &(tmp_neigh->hw_addr), sizeof(struct sockaddr));
                    i++;
                    tmp_neigh->link = 0;
                }
                else
                {
                    tmp_neigh->link = 0;
                }
            }
            tmp_neigh = tmp_neigh->next;
        }
            strncpy(wrq.ifr_name, tmp_dev->name, IFNAMSIZ);
            wrq.u.data.pointer = (caddr_t) & (iw_sa);
            wrq.u.data.length = i;
            wrq.u.data.flags = 0;

#if WIRELESS_EXT > 15
	    info.cmd = SIOCSIWSPY;
	    info.flags = 0;

	    handler = tmp_dev->dev->wireless_handlers->standard[SIOCSIWSPY - SIOCIWFIRST];
	    if (handler)
	    {
		errno = handler(tmp_dev->dev, &info, &(wrq.u),(char *) iw_sa);
	        if (errno<0)
                    printk(KERN_WARNING "AODV: Error with SIOCSIWSPY: %d\n", errno);
	    }
            else
            {
                printk(KERN_INFO "No wireless handlers\n");
            }
#else
            mm_segment_t oldfs;
            oldfs = get_fs();
            set_fs(KERNEL_DS);
            errno = tmp_dev->dev->do_ioctl(tmp_dev->dev, (struct ifreq *) &wrq, SIOCSIWSPY);
            set_fs(oldfs);

            if (errno < 0)
                printk(KERN_WARNING "AODV: Error with SIOCSIWSPY: %d\n", errno);
#endif
         
        }

        tmp_dev = tmp_dev->next;
    }
}


void get_wireless_stats()
{
    //printk(KERN_INFO "entering get_wireless_stats\n");
    int n, i, errno = 0;
    char buffer[(sizeof(struct iw_quality) + sizeof(struct sockaddr)) * IW_MAX_SPY];
    //u_int8_t temp;

    struct iwreq wrq;
    //aodv_neigh *tmp_neigh;
    aodv_dev *tmp_dev;

#if WIRELESS_EXT > 15    
    iw_handler handler;
    struct iw_request_info info;   
#endif	
		
    struct sockaddr hwa[IW_MAX_SPY];
    struct iw_quality qual[IW_MAX_SPY];

    tmp_dev = first_aodv_dev();


    while (tmp_dev != NULL)
    {
#if WIRELESS_EXT > 15
        if ( tmp_dev->dev->wireless_handlers )
#else
        if ((tmp_dev->dev->get_wireless_stats!=NULL) && (tmp_dev->dev->do_ioctl!=NULL))
#endif		
        {
            strncpy(wrq.ifr_name, tmp_dev->name, IFNAMSIZ);
            wrq.u.data.pointer = (caddr_t) buffer;
            wrq.u.data.length = 0;
            wrq.u.data.flags = 0;

#if WIRELESS_EXT > 15
	    info.cmd = SIOCGIWSPY;
	    info.flags = 0;

	    handler = tmp_dev->dev->wireless_handlers->standard[SIOCGIWSPY - SIOCIWFIRST];

	    if (handler)
	    {
	        errno = handler(tmp_dev->dev, &info, &(wrq.u), buffer);
	        if (errno<0)
		    return;
	    }
#else
            mm_segment_t oldfs;
            oldfs = get_fs();
            set_fs(KERNEL_DS);
            errno = tmp_dev->dev->do_ioctl(tmp_dev->dev,(struct ifreq * ) &wrq,SIOCGIWSPY );
            set_fs(oldfs);

            if (errno < 0)
                return;
#endif	          	    			
				    	
            n = wrq.u.data.length;
            memcpy(hwa, buffer, n * sizeof(struct sockaddr));
            memcpy(qual, buffer + n * sizeof(struct sockaddr), n * sizeof(struct iw_quality));

            for (i = 0; i < n; i++)
            {
                update_aodv_neigh_link(hwa[i].sa_data, (u_int8_t) qual[i].noise - qual[i].level);//level);// - 0x100);
            }
        }
        tmp_dev = tmp_dev->next;
    }
}

#endif
