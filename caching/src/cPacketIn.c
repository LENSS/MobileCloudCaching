
#include "cPacketIn.h"
#include "olsr_protocol.h"
#include "aodv.h"

extern u_int32_t   g_my_ip;
extern u_int32_t	cEnergy;
extern u_int32_t	cPrefetch;
extern u_int32_t	fOverhead;
extern u_int32_t	cOverhead;
extern u_int32_t	cSuccess;
extern u_int32_t	rtHello_c;
extern u_int32_t	rtHello;
extern u_int32_t	rtTc;
extern u_int32_t	rtHeader;
extern u_int32_t	aodv_hello;
extern u_int32_t	aodv_else;
extern char g_act[8];

int valid_cache_packet(int numbytes, int type, char *data)
{
    fileReq *tmp_flreq;
    fileRep *tmp_flrep;
    fragReq *tmp_frreq;
    exchReq *tmp_exreq;
    exchRep *tmp_exrep;
    addCache *tmp_add;

    switch (type)
    {
        //FILE_REQ_MESSAGE
        case 1:
	    tmp_flreq = (fileReq *) data;
	    if (numbytes == sizeof(fileReq)) {
		fOverhead += (numbytes / 4);
		if (ntohl(tmp_flreq -> src_ip) != g_my_ip) return 1;
	    }
            break;

        //FILE_REP_MESSAGE
        case 2:
	    tmp_flrep = (fileRep *) data;
	    if (numbytes == (sizeof(fileRep) + (sizeof(u_int32_t) * tmp_flrep -> frag_count))) {
		fOverhead += (numbytes / 4);
		return 1;
	    }		
	    break;

        //FRAG_REQ_ORI_MESSAGE
        case 3:
            tmp_frreq = (fragReq *) data;
	    if (numbytes == sizeof(fragReq))
            {
		fOverhead += (numbytes / 4);
                return 1;
            }
            break;
	
	//FRAG_REQ_FWD_MESSAGE
        case 4:
	    tmp_frreq = (fragReq *) data;
	    if (numbytes == sizeof(fragReq))
            {
		fOverhead += (numbytes / 4);
	        return 1;
	    }
            break;

	//EXCHANGE_REQ_MESSAGE
	case 5:
	    tmp_exreq = (exchReq *) data;
	    if (numbytes == sizeof(exchReq))
	    {
		cOverhead += (numbytes / 4);
		return 1;
	    }
	    break;

	//EXCHANGE_REP_MESSAGE
	case 6:
	    tmp_exrep = (exchRep *) data;
	    if (numbytes == sizeof(exchRep))
	    {
		cOverhead += (numbytes / 4);
		return 1;
	    }
	    break;

	//ADD_CACHE_MESSAGE
	case 7:
	    tmp_add = (addCache *) data;
	    if (numbytes == sizeof(addCache))
	    {
		cOverhead += (numbytes / 4);
		return 1;
	    }
	    break;

        default:
            break;
    }

    return 0;
}

int packet_in(struct sk_buff *packet)
{
    //printk("entering packet_in\n");
    struct iphdr *ip;

    // Create cache message types
    u_int8_t cache_type;

    //The packets which come in still have their headers from the IP and UDP
    int start_point = sizeof(struct udphdr) + sizeof(struct iphdr);

    //get pointers to the important parts of the message
    ip = (struct iphdr *)skb_network_header(packet);

    //For all cache packets the type is the first byte.
    cache_type = (int) packet->data[start_point];

    printk("cache_type = %d\n", cache_type);

    if (!valid_cache_packet(packet->len - start_point, cache_type, packet->data + start_point))
    {
        //printk(KERN_NOTICE "Cache Daemon: Packet of type: %d and of size %u from: %s failed packet check!\n", cache_type, packet->len - start_point, inet_ntoa(ip->saddr));
        return NF_DROP;
    }

    //place packet in the event queue!
    insert_task(cache_type, packet);

//#ifdef CACHE_ENABLED
    if (strcmp(g_act, "on") == 0 && cache_type == 3) {
	u_int32_t file_id = (u_int32_t) packet->data[start_point + 3 * sizeof(u_int32_t)];
	u_int32_t frag_id = (u_int32_t) packet->data[start_point + 4 * sizeof(u_int32_t)];
	reqList *tmp_creq = find_creq(file_id, frag_id);
	if (tmp_creq != NULL && reachThreshold(tmp_creq) == 1) {
	    return NF_DROP;
	}
    }
//#endif

    return NF_ACCEPT;
}

unsigned int input_handler(unsigned int hooknum, struct sk_buff *skb,
                           const struct net_device *in, const struct net_device *out, int (*okfn) (struct sk_buff *))
{
    const struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
    struct iphdr *dev_ip = in->ip_ptr;
    void *p = (uint32_t *) ip + ip->ihl;
    struct udphdr *udp = p;
    struct ethhdr *mac = (struct ethhdr *)skb_mac_header(skb);
    
    if (skb_transport_header(skb) != NULL)
    {
        if ((udp->dest == htons(CACHEPORT)) && (mac->h_proto == htons(ETH_P_IP)))
        {
            if (dev_ip->saddr != ip->saddr)
            {
		return packet_in(skb);
            }
            else
            {
            	return NF_DROP;
            }
        }

	/* The following are used for measuring olsr performance */
	else if ((udp->dest == htons(OLSRPORT)) && (mac->h_proto == htons(ETH_P_IP)))
	{
	    if (dev_ip->saddr != ip->saddr)
	    {
		int start_point = sizeof(struct udphdr) + sizeof(struct iphdr);
		struct olsr *olsr = (struct olsr *)&(skb -> data[start_point]);
		union olsr_message *m = (union olsr_message *)olsr -> olsr_msg;
		int size = skb -> len - start_point;
		uint32_t msgsize;

		uint32_t count = size - ((char *)m - (char *)olsr);
		// minimum packet size is 4
		if (count < 4 || ntohs(olsr -> olsr_packlen) != (uint16_t) size) return NF_DROP;
		//rtOverhead += olsr -> olsr_packlen;

		rtHeader++;
		for (; count > 0; m = (union olsr_message *)((char *)m + (msgsize))) {		    
		    msgsize = ntohs(m -> v4.olsr_msgsize);
		    count -= msgsize;
		    if (m -> v4.olsr_msgtype == LQ_HELLO_MESSAGE) {
			rtHello_c++;
			rtHello += (msgsize / 4);
		    }
		    else if (m -> v4.olsr_msgtype == LQ_TC_MESSAGE) rtTc += (msgsize / 4);
		}
	    }
	}

	/* The following are used for measuring kaodv performance */
	else if ((udp->dest == htons(AODVPORT)) && (mac->h_proto == htons(ETH_P_IP)))
	{
	    if (dev_ip->saddr != ip->saddr)
	    {
		int start_point = sizeof(struct udphdr) + sizeof(struct iphdr);
		u_int8_t aodv_type = (int) skb -> data[start_point];
		int numbytes = skb -> len - start_point;
		if (aodv_type == RREP_MESSAGE) {
		    rrep *tmp_rrep = (rrep *) (skb -> data + start_point);
		    if (numbytes == sizeof(rrep) && tmp_rrep -> src_ip == tmp_rrep -> dst_ip) aodv_hello += (numbytes / 4);
		    else aodv_else += (numbytes / 4);
		}
		else {
		    aodv_else += (numbytes / 4);
		}
	    }
	}

    }

    return NF_ACCEPT;

}

/*int read_overhead_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    static char *my_buffer;
    int len;

    my_buffer=buffer;
    sprintf(my_buffer, "#packets for framework: %d\n#packets for caching: %d\n", fOverhead, cOverhead);

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}*/

int read_metric_proc(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    static char *my_buffer;
    int len;

    my_buffer=buffer;
    //sprintf(my_buffer, "\nenergy: %d, prefetch: %d\nframework: %d, caching: %d\nsuccess: %d\n#hello messages: %d, hello overhead: %d, tc overhead: %d, header overhead: %d\n\n", cEnergy, cPrefetch, fOverhead, cOverhead, cSuccess, rtHello_c, rtHello, rtTc, rtHeader);

    sprintf(my_buffer, "\nenergy: %d, prefetch: %d\nframework: %d, caching: %d\nsuccess: %d\nhello overhead: %d, other overhead: %d\n\n", cEnergy, cPrefetch, fOverhead, cOverhead, cSuccess, aodv_hello, aodv_else);

    len = strlen(my_buffer);
    *buffer_location = my_buffer + offset;
    len -= offset;
    if (len > buffer_length)
        len = buffer_length;
    else if (len < 0)
        len = 0;
    return len;
}

