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
 * @file: server_conn.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.6
 *
 * @create: 2018-02-12
 *
 * @update: 2018-10-22 10:56:41
 */

#ifndef SERVER_CONN_H_INCLUDED
#define SERVER_CONN_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "server_opts.h"

#include "../xsync-protocol.h"


typedef struct xs_server_conn_t
{
    EXTENDS_REFOBJECT_TYPE();

    /* socket fd connected to xsync-server */
    int sockfd;

    randctx  rctx;
    time_t client_utctime;

    xs_server_opts srvopts[0];
} xs_server_conn_t;


__no_warning_unused(static)
inline void server_conn_delete (void *pv)
{
    xs_server_conn_t *sconn = (xs_server_conn_t *) pv;

    LOGGER_TRACE0();

    //TODO:
    int sfd = sconn->sockfd;

    if (sfd != -1) {
        sconn->sockfd = -1;
        close(sfd);
    }

    free(pv);
}


extern XS_RESULT XS_server_conn_create (const xs_server_opts *servOpts, char clientid[40], XS_server_conn *outSConn);

extern XS_VOID XS_server_conn_release (XS_server_conn *inSConn);


#if defined(__cplusplus)
}
#endif

#endif /* SERVER_CONN_H_INCLUDED */
