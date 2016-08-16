 
#include "cFragReq.h"

extern u_int32_t 		g_my_ip;
extern char g_act[8];

void convert_frreq_to_host(fragReq *tmp_frreq)
{
    tmp_frreq -> src_ip = ntohl(tmp_frreq -> src_ip);
    tmp_frreq -> dst_ip = ntohl(tmp_frreq -> dst_ip);
    tmp_frreq -> file_id = ntohl(tmp_frreq -> file_id);
    tmp_frreq -> frag_id = ntohl(tmp_frreq -> frag_id);
}

void convert_frreq_to_network(fragReq *tmp_frreq)
{
    tmp_frreq -> src_ip = htonl(tmp_frreq -> src_ip);
    tmp_frreq -> dst_ip = htonl(tmp_frreq -> dst_ip);
    tmp_frreq -> file_id = htonl(tmp_frreq -> file_id);
    tmp_frreq -> frag_id = htonl(tmp_frreq -> frag_id);
}

int data_transmit(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id) {
    // dummy transmit...
    //printk("entering data_transmit\n");
    return 0;
}

int recv_frreq_ori(cTask *tmp_task) {
    //printk("entering recv_frreq_ori\n");
    fragReq *tmp_frreq = (fragReq *)tmp_task -> data;
    convert_frreq_to_host(tmp_frreq);

    u_int32_t fileId = tmp_frreq -> file_id;
    u_int32_t fragId = tmp_frreq -> frag_id;
    //printk("fileId = %d, fragId = %d\n", fileId, fragId);
    reqList *tmp_creq;

    if (tmp_task -> dst_ip == g_my_ip) {
//#ifdef CACHE_ENABLED	
	//printk("recv_frreq_ori: %d-%d", fileId, fragId);
	if (strcmp(g_act, "on") == 0) update_ref(fileId, fragId);
//#endif
	data_transmit(g_my_ip, tmp_frreq -> src_ip, fileId, fragId);
	return 1;
    }

//#ifdef CACHE_ENABLED
    if (strcmp(g_act, "on") == 0) {
	tmp_creq = find_creq(fileId, fragId);
	if (tmp_creq == NULL) {
	//printk("receiving a new fragment request\n");
	    tmp_creq = create_creq(fileId, fragId);
	    update_count(tmp_creq);
	}
	else if (reachThreshold(tmp_creq) == 1) {
	//printk("reaching threshold\n");
	    flush_count(tmp_creq);
	    gen_frreq_by_task(tmp_task);
	    gen_exchReq(g_my_ip, tmp_task -> dst_ip, fileId, fragId);
	    insert_timer(TASK_ADD_CACHE_SEND, WAIT_EXCH_REP_TIME, tmp_task -> dst_ip, fileId, fragId);
	    update_timer_queue();
	}
	else update_count(tmp_creq);
    }
//#endif

    return 0;
}

int recv_frreq_fwd(cTask *tmp_task) {
    fragReq *tmp_frreq = (fragReq *)tmp_task -> data;
    convert_frreq_to_host(tmp_frreq);

    u_int32_t fileId = tmp_frreq -> file_id;
    u_int32_t fragId = tmp_frreq -> frag_id;
    reqList *tmp_creq;

    if (tmp_task -> dst_ip == g_my_ip) {	
	update_ref(fileId, fragId);
	data_transmit(g_my_ip, tmp_frreq -> src_ip, fileId, fragId);
	return 1;
    }

    tmp_creq = find_creq(fileId, fragId);
    if (tmp_creq != NULL) {
	flush_count(tmp_creq);
    }

    return 0;
}

int gen_frreq(u_int32_t src_ip, u_int32_t dst_ip, u_int32_t file_id, u_int32_t frag_id) {
    fragReq *tmp_frreq;
    
    if ((tmp_frreq = (fragReq *)kmalloc(sizeof(fragReq), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Can't allocate new fragment request\n");
        return 0;
    }

    tmp_frreq -> type = FRAG_REQ_ORI_MESSAGE;
    tmp_frreq -> reserved1 = 0;
    tmp_frreq -> reserved2 = 0;
    tmp_frreq -> reserved3 = 0;
    tmp_frreq -> src_ip = htonl(src_ip);
    tmp_frreq -> dst_ip = htonl(dst_ip);
    tmp_frreq -> file_id = htonl(file_id);
    tmp_frreq -> frag_id = htonl(frag_id);

    send_message(dst_ip, NET_DIAMETER, tmp_frreq, sizeof(fragReq));
    return 0;
}

int gen_frreq_by_task(cTask *tmp_task) {
    fragReq *tmp_frreq, *base;
    base = (fragReq *)tmp_task -> data;
    
    if ((tmp_frreq = (fragReq *)kmalloc(sizeof(fragReq), GFP_ATOMIC)) == NULL)
    {
        printk(KERN_WARNING "Cache Daemon: Can't allocate new fragment request\n");
        return 0;
    }

    tmp_frreq -> type = FRAG_REQ_FWD_MESSAGE;
    tmp_frreq -> reserved1 = 0;
    tmp_frreq -> reserved2 = 0;
    tmp_frreq -> reserved3 = 0;
    tmp_frreq -> src_ip = htonl(base -> src_ip);
    tmp_frreq -> dst_ip = htonl(base -> dst_ip);
    tmp_frreq -> file_id = htonl(base -> file_id);
    tmp_frreq -> frag_id = htonl(base -> frag_id);

    send_message(tmp_frreq -> dst_ip, tmp_task -> ttl - 1, tmp_frreq, sizeof(fragReq));
    return 0;
}


