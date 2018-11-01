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
 * @file: server_conf.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.3.8
 *
 * @create: 2018-02-02
 *
 * @update: 2018-10-31 16:03:02
 */

#ifndef SERVER_CONF_H_INCLUDED
#define SERVER_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "client_session.h"

/**
 * xs_server_t type
 */
typedef struct xs_server_t
{
    EXTENDS_REFOBJECT_TYPE();

    pthread_cond_t  condition;

    ref_counter_t session_counter;

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    int port;

    /**
     * an unique identifier for xs_server in redis-cluster
     * xs:serverid:xcon:sockfd
     */
    char serverid[XS_SERVERID_MAXLEN + 1];

    /**
     * server magic
     */
    ub4 magic;

    int timeout_ms;

    int ssl;

    /**
     * redis cluster
     */
    RedisConn_t redisconn;

    /**
     * epollet
     */
    epollet_conf_t epconf;

    /**
     * thread pool specific
     */
    int queues;

    /** number of threads */
    int threads;

    /** thread pool for handlers */
    threadpool_t *pool;
    void        **thread_args;

    /**
     * hlist for client_session:
     *   it's key also known as session_id is the address of
     *   pointer to XS_client_session object.
     *
     * 根据客户端传过来的 session_id 查找得到真正的 client_session 对象
     * 如果不存在则拒绝客户端的访问，断开 socket 连接。
     */
    struct hlist_head client_hlist[XSYNC_CLIENT_SESSION_HASHMAX + 1];

    /**
     * msg buffer
     */
    char msgbuf[XSYNC_ERRBUF_MAXLEN + 1];
} xs_server_t;


extern void xs_server_delete (void *pv);

#if defined(__cplusplus)
}
#endif

#endif /* SERVER_CONF_H_INCLUDED */
