/***********************************************************************
* Copyright (c) 2018 pepstack, pepstack.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*   claim that you wrote the original software. If you use this software
*   in a product, an acknowledgment in the product documentation would be
*   appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
*   misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/**
 * server_conn.h
 *   server connection
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-02-12
 * update: 2018-02-12
 *
 */

#ifndef SERVER_CONN_H_INCLUDED
#define SERVER_CONN_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "server_opts.h"


typedef struct xs_server_conn_t
{
    EXTENDS_REFOBJECT_TYPE();

    /* socket fd connected to xsync-server */
    int fd;


    xs_server_opts srvopts[0];
} xs_server_conn_t;


__no_warning_unused(static)
inline void server_conn_delete (void *pv)
{
    xs_server_conn_t *sconn = (xs_server_conn_t *) pv;

    LOGGER_TRACE0();

    //TODO:
    int sfd = sconn->fd;

    if (sfd != -1) {
        sconn->fd = -1;
        close(sfd);
    }

    free(pv);
}


extern XS_RESULT XS_server_conn_create (const xs_server_opts *servOpts, XS_server_conn *outSConn);

extern XS_VOID XS_server_conn_release (XS_server_conn *inSConn);


#if defined(__cplusplus)
}
#endif

#endif /* SERVER_CONN_H_INCLUDED */
