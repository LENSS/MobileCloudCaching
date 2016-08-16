
#ifndef SFS_H
#define SFS_H


//#include <linux/netdevice.h>


#define CACHEPORT		655
#define TRUE			1
#define FALSE 			0

// Message Types

#define FILE_REQ_MESSAGE        1

// Structures

typedef struct {
    u_int8_t type;

#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned int a:1;
    unsigned int reserved1:7;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned int reserved1:7;
    unsigned int a:1;
#else

#endif
    u_int8_t reserved2;
    u_int8_t reserved3;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t file_id;
    u_int32_t seq;
    u_int32_t metric;
} fileReq;

#endif
