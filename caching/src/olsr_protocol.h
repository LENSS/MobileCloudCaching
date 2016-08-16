
#ifndef OLSR_H
#define OLSR_H

/*
 *Values and packet formats as proposed in RFC3626 and misc. values and
 *data structures used by the olsr.org OLSR daemon.
 */

struct olsr;

/*
 *Message Types
 */

#define HELLO_MESSAGE         1
#define TC_MESSAGE            2
#define MID_MESSAGE           3
#define HNA_MESSAGE           4
#define MAX_MESSAGE           4

#define LQ_HELLO_MESSAGE      201
#define LQ_TC_MESSAGE         202

/***********************************************
 *           OLSR packet definitions           *
 ***********************************************/

/*
 *The HELLO message
 */

/*
 *Hello info
 */
struct hellinfo {
  uint8_t link_code;
  uint8_t reserved;
  uint16_t size;
  uint32_t neigh_addr[1];              /* neighbor IP address(es) */
} __attribute__ ((packed));

struct hellomsg {
  uint16_t reserved;
  uint8_t htime;
  uint8_t willingness;
  struct hellinfo hell_info[1];
} __attribute__ ((packed));

/*
 *IPv6
 */

struct hellinfo6 {
  uint8_t link_code;
  uint8_t reserved;
  uint16_t size;
  struct in6_addr neigh_addr[1];       /* neighbor IP address(es) */
} __attribute__ ((packed));

struct hellomsg6 {
  uint16_t reserved;
  uint8_t htime;
  uint8_t willingness;
  struct hellinfo6 hell_info[1];
} __attribute__ ((packed));

/*
 * Topology Control packet
 */

struct neigh_info {
  uint32_t addr;
} __attribute__ ((packed));

struct olsr_tcmsg {
  uint16_t ansn;
  uint16_t reserved;
  struct neigh_info neigh[1];
} __attribute__ ((packed));

/*
 *IPv6
 */

struct neigh_info6 {
  struct in6_addr addr;
} __attribute__ ((packed));

struct olsr_tcmsg6 {
  uint16_t ansn;
  uint16_t reserved;
  struct neigh_info6 neigh[1];
} __attribute__ ((packed));

/*
 *Multiple Interface Declaration message
 */

/*
 * Defined as s struct for further expansion
 * For example: do we want to tell what type of interface
 * is associated whit each address?
 */
struct midaddr {
  uint32_t addr;
} __attribute__ ((packed));

struct midmsg {
  struct midaddr mid_addr[1];
} __attribute__ ((packed));

/*
 *IPv6
 */
struct midaddr6 {
  struct in6_addr addr;
} __attribute__ ((packed));

struct midmsg6 {
  struct midaddr6 mid_addr[1];
} __attribute__ ((packed));

/*
 * Host and Network Association message
 */
struct hnapair {
  uint32_t addr;
  uint32_t netmask;
} __attribute__ ((packed));

struct hnamsg {
  struct hnapair hna_net[1];
} __attribute__ ((packed));

/*
 *IPv6
 */

struct hnapair6 {
  struct in6_addr addr;
  struct in6_addr netmask;
} __attribute__ ((packed));

struct hnamsg6 {
  struct hnapair6 hna_net[1];
} __attribute__ ((packed));

/*
 * OLSR message (several can exist in one OLSR packet)
 */

struct olsrmsg {
  uint8_t olsr_msgtype;
  uint8_t olsr_vtime;
  uint16_t olsr_msgsize;
  uint32_t originator;
  uint8_t ttl;
  uint8_t hopcnt;
  uint16_t seqno;

  union {
    struct hellomsg hello;
    struct olsr_tcmsg tc;
    struct hnamsg hna;
    struct midmsg mid;
  } message;

} __attribute__ ((packed));

/*
 *IPv6
 */

struct olsrmsg6 {
  uint8_t olsr_msgtype;
  uint8_t olsr_vtime;
  uint16_t olsr_msgsize;
  struct in6_addr originator;
  uint8_t ttl;
  uint8_t hopcnt;
  uint16_t seqno;

  union {
    struct hellomsg6 hello;
    struct olsr_tcmsg6 tc;
    struct hnamsg6 hna;
    struct midmsg6 mid;
  } message;

} __attribute__ ((packed));

/*
 * Generic OLSR packet
 */

struct olsr {
  uint16_t olsr_packlen;               /* packet length */
  uint16_t olsr_seqno;
  struct olsrmsg olsr_msg[1];          /* variable messages */
} __attribute__ ((packed));

struct olsr6 {
  uint16_t olsr_packlen;               /* packet length */
  uint16_t olsr_seqno;
  struct olsrmsg6 olsr_msg[1];         /* variable messages */
} __attribute__ ((packed));

/* IPv4 <-> IPv6 compability */

union olsr_message {
  struct olsrmsg v4;
  struct olsrmsg6 v6;
} __attribute__ ((packed));

union olsr_packet {
  struct olsr v4;
  struct olsr6 v6;
} __attribute__ ((packed));

#endif /* _PROTOCOLS_OLSR_H */

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
