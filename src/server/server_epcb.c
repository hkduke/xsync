/***********************************************************************
* COPYRIGHT (C) 2018 PEPSTACK, PEPSTACK.COM
*
* THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES
* ARISING FROM THE USE OF THIS SOFTWARE.
*
* PERMISSION IS GRANTED TO ANYONE TO USE THIS SOFTWARE FOR ANY PURPOSE,
* INCLUDING COMMERCIAL APPLICATIONS, AND TO ALTER IT AND REDISTRIBUTE IT
* FREELY, SUBJECT TO THE FOLLOWING RESTRICTIONS:
*
*  THE ORIGIN OF THIS SOFTWARE MUST NOT BE MISREPRESENTED; YOU MUST NOT
*  CLAIM THAT YOU WROTE THE ORIGINAL SOFTWARE. IF YOU USE THIS SOFTWARE
*  IN A PRODUCT, AN ACKNOWLEDGMENT IN THE PRODUCT DOCUMENTATION WOULD
*  BE APPRECIATED BUT IS NOT REQUIRED.
*
*  ALTERED SOURCE VERSIONS MUST BE PLAINLY MARKED AS SUCH, AND MUST NOT
*  BE MISREPRESENTED AS BEING THE ORIGINAL SOFTWARE.
*
*  THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
***********************************************************************/

/**
 * @file: server_epcb.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.2
 *
 * @create: 2018-01-29
 *
 * @update: 2018-09-05 15:26:43
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"
#include "../redisapi/redis_api.h"


int epcb_event_trace(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_TRACE(%d): %s", epmsg->clientfd, epmsg->buf);
    return 1;
}


int epcb_event_warn(epollet_msg epmsg, void *arg)
{
    LOGGER_WARN("EPEVT_WARN(%d): %s", epmsg->clientfd, epmsg->buf);
    return 1;
}


int epcb_event_fatal(epollet_msg epmsg, void *arg)
{
    LOGGER_FATAL("EPEVT_FATAL(%d): %s", epmsg->clientfd, epmsg->buf);
    return 0;
}


int epcb_event_accept_new(epollet_msg epmsg, void *arg)
{
    LOGGER_DEBUG("EPEVT_ACCEPT_NEW(%d): %s:%s", epmsg->clientfd, epmsg->hbuf, epmsg->sbuf);
    return 1;
}


int epcb_event_accepted(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_ACCEPTED(%d)", epmsg->clientfd);
    return 0;
}


int epcb_event_pollin(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_POLLIN(%d): %s", epmsg->clientfd, epmsg->buf);
    return 0;
}


int epcb_event_peer_close(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_PEER_CLOSE(%d): %s", epmsg->clientfd, epmsg->buf);
    return 0;
}
