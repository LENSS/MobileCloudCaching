
#ifndef KAODV_H
#define KAODV_H


#include <linux/netdevice.h>


// Message Types

#define RREQ_MESSAGE        1
#define RREP_MESSAGE        2
#define RERR_MESSAGE        3
#define RREP_ACK_MESSAGE 		4


// Structures

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
