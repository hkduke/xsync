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
 * server_conn.c
 *   server opts and server connection
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-02-12
 * update: 2018-02-12
 *
 */

#include "client_api.h"

#include "server_conn.h"

#include "../common/common_util.h"


extern XS_RESULT XS_server_conn_create (const xs_server_opts *servOpts, char clientid[40], XS_server_conn *outSConn)
{
    XS_server_conn sconn;

    sconn = (XS_server_conn) mem_alloc(1, sizeof(struct xs_server_conn_t) + sizeof(xs_server_opts));

    sconn->sockfd = -1;

    /**
     * http://man7.org/linux/man-pages/man2/time.2.html
     *
     * time(tloc) returns the time as the number of seconds since the Epoch,
     * 1970-01-01 00:00:00 +0000 (UTC).
     *
     * Applications intended to run after 2038 should use ABIs with time_t
     *   wider than 32 bits.
     *
     * The tloc argument is obsolescent and should always be NULL in new
     *   code.  When tloc is NULL, the call cannot fail.
     */
    sconn->client_utctime = time(NULL);

    ub4 t32 = (ub4) (0x00000000FFFFFFFF & sconn->client_utctime);

    randctx_init(&sconn->rctx, t32 ^ servOpts->magic);

    do {
        int sockfd, err;

        XSYNC_ConnectRequest connRequest;

        XSYNC_ConnectRequestBuild(&connRequest, servOpts->magic, t32, rand_gen(&sconn->rctx), clientid);

        LOGGER_TRACE("msgid=%d('%c%c%c%c'), magic=%d, version=%d('%d.%d.%d'), time=%d, clientid=%s, randnum=%d, crc32=%d",
                connRequest.msgid,
                connRequest.connect_request[0],
                connRequest.connect_request[1],
                connRequest.connect_request[2],
                connRequest.connect_request[3],
                connRequest.magic,
                connRequest.client_version,
                connRequest.connect_request[11],
                connRequest.connect_request[10],
                connRequest.connect_request[9],
                connRequest.client_utctime,
                connRequest.clientid,
                connRequest.randnum,
                connRequest.crc32_checksum);

        sockfd = opensocket(servOpts->host, servOpts->port, servOpts->sockopts.timeosec, servOpts->sockopts.nowait, &err);

        sconn->sockfd = sockfd;
    } while(0);

    memcpy(sconn->srvopts, servOpts, sizeof(xs_server_opts));

    *outSConn = (XS_server_conn) RefObjectInit(sconn);

    return XS_SUCCESS;
}


extern XS_VOID XS_server_conn_release (XS_server_conn *inSConn)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inSConn, server_conn_delete);
}