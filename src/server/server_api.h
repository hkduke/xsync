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
 * @file: server_api.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.7
 *
 * @create: 2018-01-29
 *
 * @update: 2018-10-22 10:56:41
 */

#ifndef SERVER_API_H_INCLUDED
#define SERVER_API_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#define LOGGER_CATEGORY_NAME  XSYNC_SERVER_APPNAME
#include "../common/log4c_logger.h"

#include "../common/threadlock.h"
#include "../common/common_util.h"
#include "../redisapi/redis_api.h"

#include "../xsync-error.h"
#include "../xsync-config.h"
#include "../xsync-protocol.h"

#include "../common/epollet.h"

#define XS_SERVERID_MINID   1
#define XS_SERVERID_MAXID   65535

#define XS_SERVERID_MAXLEN  5


typedef struct xs_server_t * XS_server;


typedef struct xs_appopts_t
{
    // singleton
    char *startcmd;

    char serverid[XS_SERVERID_MAXLEN + 1];

    ub4 magic;

    int isdaemon;
    int interactive;

    int threads;
    int queues;
    int somaxconn;
    int maxevents;
    int timeout_ms;

    char redis_cluster[1020];
    char redis_auth[33];

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    char port[XSYNC_PORTNUMB_MAXLEN + 1];

    char config[XSYNC_PATHFILE_MAXLEN + 1];
} xs_appopts_t;


extern void event_task (thread_context_t *thread_ctx);

/**
 * epollet event 回调函数
 */
extern int epcb_event_trace(epollet_msg epmsg, void *arg);

extern int epcb_event_warn(epollet_msg epmsg, void *arg);

extern int epcb_event_fatal(epollet_msg epmsg, void *arg);

extern int epcb_event_peer_open(epollet_msg epmsg, void *arg);

extern int epcb_event_peer_close(epollet_msg epmsg, void *arg);

extern int epcb_event_accept(epollet_msg epmsg, void *arg);

extern int epcb_event_reject(epollet_msg epmsg, void *arg);

extern int epcb_event_pollin(epollet_msg epmsg, void *arg);




/**
 * 生成 redis XCON 表的 key = 'xs:$serverid:xcon:$connfd'
 *   size of key[] specified by len must > 30
 */
static inline const char * XCON_redis_table_key(const char *serverid, int connfd, char key[], int len) {
    len = snprintf(key, 30, "xs:%s:xcon:%d", serverid, connfd);
    key[len] = 0;
    return key;
}


/**
 * XS_server application api
 */
extern XS_RESULT XS_server_create (xs_appopts_t *opts, XS_server *outServer);

extern XS_VOID XS_server_release (XS_server *pServer);

extern int XS_server_lock (XS_server server);

extern void XS_server_unlock (XS_server server);

extern XS_VOID XS_server_bootstrap (XS_server server);

/**
 * XS_server configuration api
 *
 * implemented in:
 *   server_conf.c
 */



/**
 * XS_server internal api
 */

extern XS_VOID XS_server_clear_client_sessions (XS_server server);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_API_H_INCLUDED */
