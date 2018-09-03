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
 * @file: server_conn.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.1
 *
 * @create: 2018-02-12
 *
 * @update: 2018-08-31 15:02:41
 */

#include "client_api.h"

#include "server_conn.h"

#include "../common/common_util.h"


extern XS_RESULT XS_server_conn_create (const xs_server_opts *servOpts, char clientid[40], XS_server_conn *outSConn)
{
    XS_server_conn xcon;

    xcon = (XS_server_conn) mem_alloc(1, sizeof(struct xs_server_conn_t) + sizeof(xs_server_opts));

    xcon->sockfd = -1;

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
    xcon->client_utctime = time(NULL);

    ub4 t32 = (ub4) (0x00000000FFFFFFFF & xcon->client_utctime);

    randctx_init(&xcon->rctx, t32 ^ servOpts->magic);

    do {
        int sockfd;

        char msg[256];

        sockfd = opensocket_v2(servOpts->host, servOpts->sport, servOpts->sockopts.timeosec, &servOpts->sockopts, msg);

        if (sockfd == -1) {
            LOGGER_ERROR("opensocket_v2 failed: %s", msg);
            return XS_ERROR;
        } else {
            LOGGER_INFO("opensocket_v2 success: %s:%s", servOpts->host, servOpts->sport);

            XSConnectReq_t xconReq;

            XSConnectRequestBuild(&xconReq, clientid, servOpts->magic, xcon->client_utctime, rand_gen(&xcon->rctx), (ub1*) msg);
            
            // 发送连接请求: XS_CONNECT_REQ_SIZE 字节
            int err = sendlen(sockfd, msg, XS_CONNECT_REQ_SIZE);

            if (err != XS_CONNECT_REQ_SIZE) {
                LOGGER_ERROR("sendlen error(%d): %s", errno, strerror(errno));
                close(sockfd);
                return XS_ERROR;
            }

            LOGGER_TRACE("%s", XSConnectRequestPrint(&xconReq, msg, sizeof(msg)));

            sleep(60);

            //TODO:
            LOGGER_DEBUG("TODO: recv");
        }

        // 保存当前连接描述符
        xcon->sockfd = sockfd;
    } while(0);

    memcpy(xcon->srvopts, servOpts, sizeof(xs_server_opts));

    *outSConn = (XS_server_conn) RefObjectInit(xcon);

    return XS_SUCCESS;
}


extern XS_VOID XS_server_conn_release (XS_server_conn *inSConn)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inSConn, server_conn_delete);
}
