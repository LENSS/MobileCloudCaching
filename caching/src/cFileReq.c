 
#include "cFileReq.h"

extern u_int32_t 		g_broadcast_ip;
extern u_int32_t   g_my_ip;

void convert_flreq_to_host(fileReq *tmp_flreq)
{
    tmp_flreq -> src_ip = ntohl(tmp_flreq -> src_ip);
    tmp_flreq -> dst_ip = ntohl(tmp_flreq -> dst_ip);
    tmp_flreq -> file_id = ntohl(tmp_flreq -> file_id);
    tmp_flreq -> seq = ntohl(tmp_flreq -> seq);
    tmp_flreq -> metric = ntohl(tmp_flreq -> metric);
}

void convert_flreq_to_network(fileReq *tmp_flreq)
{
    tmp_flreq -> src_ip = htonl(tmp_flreq -> src_ip);
    tmp_flreq -> dst_ip = htonl(tmp_flreq -> dst_ip);
    tmp_flreq -> file_id = htonl(tmp_flreq -> file_id);
    tmp_flreq -> seq = htonl(tmp_flreq -> seq);
    tmp_flreq -> metric = htonl(tmp_flreq -> metric);
}

int recv_flreq(cTask *tmp_task) {
    //printk("entering recv_flreq\n");
    fileReq *tmp_flreq;
    u_int64_t current_time;

    tmp_flreq = tmp_task -> data;
    convert_flreq_to_host(tmp_flreq);
    //printk("src_ip = %s\n", inet_ntoa(tmp_flreq -> src_ip));
    //printk("dst_ip = %s\n", inet_ntoa(tmp_flreq -> dst_ip));
    //printk("file_id = %d\n", tmp_flreq -> file_id);
    //printk("seq = %d\n", tmp_flreq -> seq);
    current_time = getcurrtime();

    if (tmp_task -> ttl <= 1)
    {
        printk(KERN_INFO "TTL for file request from: %s expired\n", inet_ntoa(tmp_flreq -> src_ip));
        return -ETIMEDOUT;
    }

    if (find_flood_id(tmp_flreq -> src_ip, tmp_flreq -> seq))
    {
	//printk("already received the same file request before\n");
        return 1;
    }

    tmp_flreq -> metric++;

    insert_flood_id(tmp_flreq -> src_ip, tmp_flreq -> dst_ip, tmp_flreq -> seq, current_time + FILE_REQUEST_LIFETIME);
    
    gen_flrep(g_my_ip, tmp_flreq -> src_ip, tmp_flreq -> file_id, tmp_flreq -> metric);

    forward_flreq(tmp_flreq, tmp_task -> ttl);

    return 0;
}

int forward_flreq(fileReq *tmp_flreq, int ttl)
{
    //printk("entering forward_flreq\n");
    convert_flreq_to_network(tmp_flreq);
		
    /* Call send_datagram to send and forward the file request */
    local_broadcast(ttl - 1, tmp_flreq, sizeof(fileReq));

    return 0;
}


