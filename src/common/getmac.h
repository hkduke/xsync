/**
 * getmac.h
 *
 * 2014-07-08: init created
 */
#ifndef GETMAC_H_INCLUDED
#define GETMAC_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/param.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <unistd.h>


#ifdef SOLARIS
# include <sys/sockio.h>
#endif

#define GETMAC_MAX_INTERFACES   16

#define GETMAC_MAX_MSGLEN      256


#define GETMAC_NOERROR    0
#define GETMAC_SUCCESS  GETMAC_NOERROR
#define GETMAC_ERROR    (-1)
#define GETMAC_EATTR    (-2)


typedef int GETMAC_BOOL;

#define GETMAC_TRUE    1
#define GETMAC_FALSE   0


typedef struct
{
    int fd;
    struct ifreq buf[GETMAC_MAX_INTERFACES];
    struct arpreq arp;
    int ifaces;
    char separator;   /* default: ':' */
} getmac_info_t;


typedef struct
{
    int errcode;
    char errmsg[GETMAC_MAX_MSGLEN];
} getmac_error_t;


#define GETMAC_ATTR_IFF_UP          1       /* Interface is up */
#define GETMAC_ATTR_IFF_BROADCAST   2       /* Broadcast address valid */
#define GETMAC_ATTR_IFF_DEBUG       3       /* Turn on debugging */
#define GETMAC_ATTR_IFF_LOOPBACK    4       /* Is a loopback net */
#define GETMAC_ATTR_IFF_POINTOPOINT 5       /* Interface is point-to-point link */
#define GETMAC_ATTR_IFF_NOTRAILERS  6       /* Avoid use of trailers */
#define GETMAC_ATTR_IFF_RUNNING     7       /* Resources allocated */
#define GETMAC_ATTR_IFF_NOARP       8       /* No address resolution protocol */
#define GETMAC_ATTR_IFF_PROMISC     9       /* Receive all packets */

#define GETMAC_ATTR_IFNAME         10       /* Interface name, e.g. "en0".  */
#define GETMAC_ATTR_IPADDR         11       /* Address of interface */
#define GETMAC_ATTR_HWADDR         12       /* MAC address */


extern int getmac_init (getmac_info_t * mi, const char *ipaddr, unsigned short port, char separator, getmac_error_t * err);


extern int getmac_attr (getmac_info_t * mi, int i, int attr, void * value, getmac_error_t * err);


extern void getmac_fini (getmac_info_t * mi);


#if defined(__cplusplus)
}
#endif


#endif /* GETMAC_H_INCLUDED */
