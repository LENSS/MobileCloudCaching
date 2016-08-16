
#ifndef CACHE_H
#define CACHE_H


#include <linux/netdevice.h>

//#define CACHE_ENABLED

#define CACHEPORT		655
#define OLSRPORT		698
#define AODVPORT		654
#define TRUE			1
#define FALSE 			0

//#define KVAL	3
//#define NVAL	4
//#define THRESHOLD	4
//#define BUF_SIZE	3

#define FLUSH_INTERVAL		3000
#define WAIT_FILE_REP_TIME		10000
#define WAIT_EXCH_REP_TIME		2000
#define FILE_REQUEST_LIFETIME	5000
#define NET_DIAMETER          10

// Message Types

#define FILE_REQ_MESSAGE        1
#define FILE_REP_MESSAGE        2
#define FRAG_REQ_ORI_MESSAGE        3
#define FRAG_REQ_FWD_MESSAGE        4
#define EXCHANGE_REQ_MESSAGE        5
#define EXCHANGE_REP_MESSAGE        6
#define ADD_CACHE_MESSAGE        7

// Tasks

#define TASK_FILE_REQ_PROCESS           1
#define TASK_FILE_REP_PROCESS           2
#define TASK_FRAG_REQ_UPDATE           3
#define TASK_FRAG_REQ_FLUSH           4
#define TASK_EXCH_SEND		5
#define TASK_EXCH_UPDATE	6
#define TASK_ADD_CACHE_PROCESS		7
#define TASK_FRAG_REQ_SEND		8
#define TASK_ADD_CACHE_SEND		9
#define TASK_CLEAN_UP		10

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
#error "Please fix <asm/byteorder.h>"
#endif
    u_int8_t reserved2;
    u_int8_t reserved3;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t file_id;
    u_int32_t seq;
    u_int32_t metric;
} fileReq;

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
    unsigned int frag_count:8;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t file_id;
    u_int32_t dist;
} fileRep;

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
    u_int8_t reserved3;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t file_id;
    u_int32_t frag_id;
} fragReq;

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
    u_int8_t reserved3;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t file_id;
    u_int32_t frag_id;
} exchReq;

typedef struct {
    u_int8_t type;

#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned int n:1;
    unsigned int reserved1:7;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned int reserved1:7;
    unsigned int n:1;
#else
#error "Please fix <asm/byteorder.h>"
#endif
    u_int8_t reserved2;
    u_int8_t reserved3;
    u_int32_t file_id;
    u_int32_t frag_id;
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t score;
} exchRep;

typedef struct {
    u_int8_t type;

#if defined(__BIG_ENDIAN_BITFIELD)
    unsigned int n:1;
    unsigned int reserved1:7;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
    unsigned int reserved1:7;
    unsigned int n:1;
#else
#error "Please fix <asm/byteorder.h>"
#endif
    u_int8_t reserved2;
    u_int8_t reserved3;
    u_int32_t file_id;
    u_int32_t frag_id;
    u_int32_t src_ip;
    u_int32_t dst_ip;
} addCache;

struct _cFloodId {
    u_int32_t src_ip;
    u_int32_t dst_ip;
    u_int32_t id;
    u_int64_t lifetime;
    struct _cFloodId *next;
};
typedef struct _cFloodId cFloodId;

struct _cTask {
    int type;
    u_int64_t time;

    u_int32_t dst_ip;
    u_int32_t src_ip;
    u_int32_t file_id;
    u_int32_t frag_id;
    u_int32_t ttl;

    unsigned int data_len;
    void *data;

    struct _cTask *next;
    struct _cTask *prev;
};
typedef struct _cTask cTask;

struct _cacheTable {
    u_int32_t file_id;
    u_int32_t frag_id;
    u_int32_t refs;
    u_int32_t isServiceCenter;

    struct _cacheTable *next;
    struct _cacheTable *prev;
};
typedef struct _cacheTable cacheTable;

struct _fileRepInfo {
    u_int32_t file_id;
    u_int32_t dst_ip;
    u_int32_t dist;
    u_int32_t map[8];
    u_int32_t frag_count;
    u_int32_t expired;

    struct _fileRepInfo *next;
    struct _fileRepInfo *prev;
};
typedef struct _fileRepInfo fileRepInfo;

struct _reqList {
    u_int32_t file_id;
    u_int32_t frag_id;
    u_int32_t count;

    struct _reqList *next;
    struct _reqList *prev;
};
typedef struct _reqList reqList;

struct _exchTable {
    u_int32_t file_id;
    u_int32_t frag_id;
    u_int32_t dst_ip;
    u_int32_t neigh_ip;
    u_int32_t score;
    u_int32_t expired;

    struct _exchTable *next;
    struct _exchTable *prev;
};
typedef struct _exchTable exchTable;

#endif
