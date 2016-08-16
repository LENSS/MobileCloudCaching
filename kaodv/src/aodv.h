/***************************************************************************
                          aodv.h  -  description
                             -------------------
    begin                : Tue Jul 1 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/


#ifndef AODV_H
#define AODV_H


#include <linux/netdevice.h>

//#define AODV_SIGNAL
//#define LINK_LIMIT


#define AODVPORT		654
#define TRUE			1
#define FALSE 			0


// See section 10 of the AODV draft
// Times in milliseconds

#define ACTIVE_ROUTE_TIMEOUT 	3000
#define ALLOWED_HELLO_LOSS 		2
#define BLACKLIST_TIMEOUT 		RREQ_RETRIES * NET_TRAVERSAL_TIME
#define DELETE_PERIOD         ALLOWED_HELLO_LOSS * HELLO_INTERVAL
#define HELLO_INTERVAL        1000
#define LOCAL_ADD_TTL         2
#define MAX_REPAIR_TTL        0.3 * NET_DIAMETER
#define MY_ROUTE_TIMEOUT      ACTIVE_ROUTE_TIMEOUT
#define NET_DIAMETER          10
#define NODE_TRAVERSAL_TIME   40
#define NET_TRAVERSAL_TIME 		2 * NODE_TRAVERSAL_TIME * NET_DIAMETER
#define NEXT_HOP_WAIT         NODE_TRAVERSAL_TIME + 10
#define PATH_DISCOVERY_TIME   2 * NET_TRAVERSAL_TIME
#define RERR_RATELIMIT        10
#define RING_TRAVERSAL_TIME   2 * NODE_TRAVERSAL_TIME * ( TTL_VALUE + TIMEOUT_BUFFER)
#define RREQ_RETRIES 			    2
#define RREQ_RATELIMIT 		    10
#define TIMEOUT_BUFFER 		    2
#define TTL_START 			      2
#define TTL_INCREMENT 		    2
#define TTL_THRESHOLD         7
#define TTL_VALUE             3


// Message Types

#define RREQ_MESSAGE        1
#define RREP_MESSAGE        2
#define RERR_MESSAGE        3
#define RREP_ACK_MESSAGE 		4

// Tasks

#define TASK_RREQ           1
#define TASK_RREP           2
#define TASK_RERR 			    3
#define TASK_RREP_ACK		    4
#define TASK_RESEND_RREQ 	  101
#define TASK_HELLO				  102
#define TASK_NEIGHBOR			  103
#define TASK_CLEANUP			  104
#define TASK_ROUTE_CLEANUP  105


// Structures

// Route table

struct _flood_id {
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t id;
    u_int64_t lifetime;
    struct _flood_id *next;
};
typedef struct _flood_id flood_id;


struct _aodv_route {
    u_int32_t ip;
    u_int32_t netmask;
    u_int32_t seq;
    u_int32_t old_seq;
    u_int8_t  metric;
    u_int32_t next_hop;
    u_int32_t rreq_id;
    u_int64_t lifetime;

    struct net_device *dev;
    u_int8_t route_valid:1;
    u_int8_t route_seq_valid:1;
    u_int8_t self_route:1;

    struct _aodv_route *next;
    struct _aodv_route *prev;
};
typedef struct _aodv_route aodv_route;

struct _aodv_dev {
    struct net_device *dev;
    aodv_route *route_entry;
    int index;
    u_int32_t ip;
    u_int32_t netmask;
    char name[IFNAMSIZ];
    struct _aodv_dev *next;
    struct socket *sock;
};
typedef struct _aodv_dev aodv_dev;



struct _aodv_neigh {
    u_int32_t ip;
    u_int32_t seq;
    u_int64_t lifetime;
    unsigned char hw_addr[ETH_ALEN];
    struct net_device *dev;
    aodv_route *route_entry;
    int link;
    u_int8_t valid_link;

    struct _aodv_neigh *next;
};
typedef struct _aodv_neigh aodv_neigh;


struct _task {

    int type;
    u_int32_t id;
    u_int64_t time;
    u_int32_t dst_ip;
    u_int32_t src_ip;
    struct net_device *dev;
    u_int8_t ttl;
    u_int16_t retries;

    unsigned char src_hw_addr[ETH_ALEN];

    unsigned int data_len;
    void *data;

    struct _task *next;
    struct _task *prev;

};
typedef struct _task task;



//Route reply message type
typedef struct {
    u_int8_t type;

} rrep_ack;

typedef struct {
    u_int8_t type;

#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned int a:1;
    unsigned int reserved1:7;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned int reserved1:7;
    unsigned int a:1;
#else
#error "Please fix <asm/byteorder.h>"
#endif
    u_int8_t reserved2;
    u_int8_t metric;
    u_int32_t dst_ip;
    u_int32_t dst_seq;
    u_int32_t src_ip;
    u_int32_t lifetime;
} rrep;


//Endian handling based on DSR implemetation by Alex Song s369677@student.uq.edu.au

typedef struct {
    u_int8_t type;

#if defined(__BIG_ENDIAN_BITFIELD)
    u_int8_t j:1;
    u_int8_t r:1;
    u_int8_t g:1;
    u_int8_t d:1;
    u_int8_t u:1;
    u_int8_t reserved:3;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    u_int8_t reserved:3;
    u_int8_t u:1;
    u_int8_t d:1;
    u_int8_t g:1;
    u_int8_t r:1;
    u_int8_t j:1;
#else
#error "Please fix <asm/byteorder.h>"
#endif
    u_int8_t second_reserved;
    u_int8_t metric;
    u_int32_t rreq_id;
    u_int32_t dst_ip;
    u_int32_t dst_seq;
    u_int32_t src_ip;
    u_int32_t src_seq;
} rreq;



typedef struct {
    u_int8_t type;

#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned int n:1;
    unsigned int reserved:15;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned int reserved:15;
    unsigned int n:1;
#else
#error "Please fix <asm/byteorder.h>"
#endif
    unsigned int dst_count:8;
} rerr;


typedef struct {
    u_int32_t ip;
    u_int32_t seq;
} aodv_dst;



struct _rerr_route {
    u_int32_t ip;
    u_int32_t seq;
    struct _rerr_route *next;
};

typedef struct _rerr_route rerr_route;

#endif
