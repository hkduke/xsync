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
 * @version: 0.4.2
 *
 * @create: 2018-01-29
 *
 * @update: 2018-11-07 10:20:15
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"
#include "../redisapi/redis_api.h"


int epcb_event_trace (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_warn (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_error (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_new_peer (struct epollet_event_t *event)
{
    // 接受
    return 1;
}


int epcb_event_accept (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_reject (struct epollet_event_t *event)
{
    return 0;
}


int epcb_event_pollin (struct epollet_event_t *event)
{
    off_t total = 0;

    int next = 1;
    int count = 0;

    int len = sizeof(event->msg) - 1;

    int sfd = event->clientfd;

    // 读光缓冲区

    while (next && (count = readlen_next(sfd, event->msg, sizeof event->msg, &next)) >= 0) {
        if (count > 0) {
            total += count;
        }
    }

    if (! next) {
        printf("read %ju bytes ok.\n", total);

        if (total == XS_CONNECT_REQ_SIZE) {
            XSConnectReq_t xconReq;

            XS_BOOL isOK = XSConnectRequestParse((ub1 *) event->msg, &xconReq);

            if ( isOK ) {
                printf("%s\n", XSConnectRequestOutput(&xconReq, xconReq.password, event->msg, sizeof event->msg));
            }
        }
    } else if (count < 0) {
        // Closing the descriptor will make epoll remove it from
        //  the set of descriptors which are monitored
        snprintf(event->msg, len, "client close socket(%d)", sfd);
        event->msg[len] = 0;

        printf(event->msg);

        close(sfd);

        return 1;
    }

    // rearm the socket. 注册事件用于 write
    if (epollout_mod(event->epollfd, event->clientfd, event->msg, sizeof event->msg) == -1) {
        close(sfd);

        printf(event->msg);

        exit(-1);
    }

    printf("epcb_event_pollin success\n");

    return 1;
}


int epcb_event_pollout (struct epollet_event_t *event)
{
    return 0;
}
