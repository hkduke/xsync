/**
 * getmac.c
 *
 * 2014-07-08: init created
 */
#include "getmac.h"


static int opensock (const char *ipaddr, unsigned short port)
{
    struct sockaddr_in   saddr;

    /* set saddr */
    saddr.sin_addr.s_addr = inet_addr (ipaddr);
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons ((u_short) port);

    /* create a new socket and attempt to connect to saddr */
    return socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
}


int getmac_init (getmac_info_t * mi, const char *ipaddr, unsigned short port, char sep, getmac_error_t * err)
{
    int fd;
    struct ifconf ifc;

    bzero (mi, sizeof(getmac_info_t));
    mi->fd = -1;

    if (! ipaddr) {
        fd = socket (PF_INET, SOCK_DGRAM, 0);
    } else {
        fd = opensock (ipaddr, port);
    }

    if (fd == -1) {
        err->errcode = errno;
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "socket() error (%d): %s", errno, strerror(errno));
        return GETMAC_ERROR;
    }

    /* prepare to get mac numb */
    ifc.ifc_len = sizeof(mi->buf);
    ifc.ifc_buf = (caddr_t) mi->buf;

    if (ioctl (fd, SIOCGIFCONF, (char *) &ifc)) {
        err->errcode = errno;
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl() error (%d): %s", errno, strerror(errno));
        close (fd);
        return GETMAC_ERROR;
    }

    mi->ifaces = ifc.ifc_len / sizeof (struct ifreq);
    mi->fd = fd;
    mi->separator = sep;

    /* return numb of mac */
    return mi->ifaces;
}


int getmac_attr (getmac_info_t * mi, int i, int attr, void * value, getmac_error_t * err)
{
    if (mi->fd == -1) {
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "getmac_init should be invoked first");
        return GETMAC_ERROR;
    }

    /* initialize if not a valid name */
    if (! mi->buf[i].ifr_name[0]) {
        if (ioctl (mi->fd, SIOCGIFFLAGS, (char *) & mi->buf[i])) {
            err->errcode = errno;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl(SIOCGIFFLAGS) error (%d): %s", errno, strerror(errno));
            return GETMAC_ERROR;
        }


        if (ioctl (mi->fd, SIOCGIFADDR, (char *) & mi->buf[i])) {
            err->errcode = errno;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl(SIOCGIFADDR) error (%d): %s", errno, strerror(errno));
            return GETMAC_ERROR;
        }

        /* get Hardware Address */
#ifdef SOLARIS
        do {
            mi->arp.arp_pa.sa_family = AF_INET;
            mi->arp.arp_ha.sa_family = AF_INET;
            ((struct sockaddr_in *) & mi->arp.arp_pa)->sin_addr.s_addr = ((struct sockaddr_in*)(& buf[i].ifr_addr))->sin_addr.s_addr;

            if ((ioctl (mi->fd, SIOCGARP, (char *) & mi->arp))) {
                err->errcode = errno;
                snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl(SIOCGARP) error (%d): %s", errno, strerror(errno));
                return GETMAC_ERROR;
            }
        } while (0);
#else
    #if 0
        do {
            /* get HW ADDRESS of the net card */
            if (ioctl (mi->fd, SIOCGENADDR, (char *) & buf[i])) {
                err->errcode = errno;
                snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl(SIOCGENADDR) error (%d): %s", errno, strerror(errno));
                return GETMAC_ERROR;
            }
        } while (0);
    #else
        do {
            if (ioctl (mi->fd, SIOCGIFHWADDR, (char *) & mi->buf[i])) {
                err->errcode = errno;
                snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl(SIOCGIFHWADDR) error (%d): %s", errno, strerror(errno));
                return GETMAC_ERROR;
            }
        } while (0);
    #endif
#endif
    }

    err->errcode = GETMAC_NOERROR;

    /* get net attribute */
    switch (attr) {
    case GETMAC_ATTR_IFF_UP:    /* Interface is up */
        if (mi->buf[i].ifr_flags & IFF_UP) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Interface is up");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Interface is not up");
        }
        break;

    case GETMAC_ATTR_IFF_BROADCAST:    /* Broadcast address valid */
        if (mi->buf[i].ifr_flags & IFF_BROADCAST) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Broadcast address valid");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Broadcast address invalid");
        }
        break;

    case GETMAC_ATTR_IFF_DEBUG:    /* Turn on debugging */
        if (mi->buf[i].ifr_flags & IFF_DEBUG) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Turn on debugging");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Turn off debugging");
        }
        break;

    case GETMAC_ATTR_IFF_LOOPBACK:    /* Is a loopback net */
        if (mi->buf[i].ifr_flags & IFF_LOOPBACK) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Is a loopback net");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Not a loopback net");
        }
        break;

    case GETMAC_ATTR_IFF_POINTOPOINT:    /* Interface is point-to-point link */
        if (mi->buf[i].ifr_flags & IFF_POINTOPOINT) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Interface is point-to-point link");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Interface is not point-to-point link");
        }
        break;

    case GETMAC_ATTR_IFF_NOTRAILERS:    /* Avoid use of trailers */
        if (mi->buf[i].ifr_flags & IFF_NOTRAILERS) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Avoid use of trailers");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Not Avoid use of trailers");
        }
        break;

    case GETMAC_ATTR_IFF_RUNNING:    /* Resources allocated */
        if (mi->buf[i].ifr_flags & IFF_RUNNING) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Resources allocated");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Resources not allocated");
        }
        break;

    case GETMAC_ATTR_IFF_NOARP:    /* No address resolution protocol */
        if (mi->buf[i].ifr_flags & IFF_NOARP) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "No address resolution protocol");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Has address resolution protocol");
        }
        break;

    case GETMAC_ATTR_IFF_PROMISC:    /* Receive all packets */
        if (mi->buf[i].ifr_flags & IFF_PROMISC) {
            * ((GETMAC_BOOL *) value) = GETMAC_TRUE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Receive all packets");
        } else {
            * ((GETMAC_BOOL *) value) = GETMAC_FALSE;
            snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Not a promisc mode");
        }
        break;

    case GETMAC_ATTR_IFNAME:    /* Interface name, e.g. "en0" */
        strcpy (value, mi->buf[i].ifr_name);
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Interface name");
        break;

    case GETMAC_ATTR_IPADDR:    /* Interface address */
        strcpy (value, inet_ntoa (((struct sockaddr_in*) (& mi->buf[i].ifr_addr))->sin_addr));
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "Interface address");
        break;

    case GETMAC_ATTR_HWADDR:
#ifdef SOLARIS
        do {
            mi->arp.arp_pa.sa_family = AF_INET;
            mi->arp.arp_ha.sa_family = AF_INET;

            ((struct sockaddr_in *) & mi->arp.arp_pa)->sin_addr.s_addr = ((struct sockaddr_in*)(& buf[i].ifr_addr))->sin_addr.s_addr;

            if ((ioctl (mi->fd, SIOCGARP, (char *) & mi->arp))) {
                err->errcode = errno;
                snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "ioctl(SIOCGARP) error (%d): %s", strerror(errno));
                return GETMAC_ERROR;
            } else {
                sprintf (value, "%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
                    (unsigned char) mi->arp.arp_ha.sa_data[0], mi->separator,
                    (unsigned char) mi->arp.arp_ha.sa_data[1], mi->separator,
                    (unsigned char) mi->arp.arp_ha.sa_data[2], mi->separator,
                    (unsigned char) mi->arp.arp_ha.sa_data[3], mi->separator,
                    (unsigned char) mi->arp.arp_ha.sa_data[4], mi->separator,
                    (unsigned char) mi->arp.arp_ha.sa_data[5] );
            }
        } while (0);
#else
    #if 0
        do {
            sprintf("%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
                (unsigned char) mi->buf[i].ifr_enaddr[0], mi->separator,
                (unsigned char) mi->buf[i].ifr_enaddr[1], mi->separator,
                (unsigned char) mi->buf[i].ifr_enaddr[2], mi->separator,
                (unsigned char) mi->buf[i].ifr_enaddr[3], mi->separator,
                (unsigned char) mi->buf[i].ifr_enaddr[4], mi->separator,
                (unsigned char) mi->buf[i].ifr_enaddr[5]);
        } while (0);
    #else
        do {
            sprintf (value, "%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
                (unsigned char) mi->buf[i].ifr_hwaddr.sa_data[0], mi->separator,
                (unsigned char) mi->buf[i].ifr_hwaddr.sa_data[1], mi->separator,
                (unsigned char) mi->buf[i].ifr_hwaddr.sa_data[2], mi->separator,
                (unsigned char) mi->buf[i].ifr_hwaddr.sa_data[3], mi->separator,
                (unsigned char) mi->buf[i].ifr_hwaddr.sa_data[4], mi->separator,
                (unsigned char) mi->buf[i].ifr_hwaddr.sa_data[5]);
        } while (0);
    #endif
#endif
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "MAC address");
        break;

    default:
        err->errcode = GETMAC_EATTR;
        snprintf (err->errmsg, GETMAC_MAX_MSGLEN, "error attr specified");
        return GETMAC_EATTR;
    }

    return GETMAC_SUCCESS;
}


void getmac_fini (getmac_info_t * mi)
{
    if (mi->fd != -1) {
        close (mi->fd);
        mi->fd = -1;
    }
}
