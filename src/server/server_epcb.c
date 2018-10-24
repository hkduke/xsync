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
 * @version: 0.2.8
 *
 * @create: 2018-01-29
 *
 * @update: 2018-10-22 10:56:41
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

    /* always return 0 */
    return 0;
}


int epcb_event_peer_open(epollet_msg epmsg, void *arg)
{
    LOGGER_DEBUG("EPEVT_PEER_OPEN(%d): %s:%s", epmsg->clientfd, epmsg->hbuf, epmsg->sbuf);

    XS_server server = (XS_server) arg;

    // xs:1:xcon:10 host port clientid
    XCON_redis_table_key(server->serverid, epmsg->clientfd, server->msgbuf, sizeof server->msgbuf);

    const char * flds[] = {
        "clientid",
        "host",
        "port",
        0
    };

    const char * vals[] = {
        0, // delete it
        epmsg->hbuf,
        epmsg->sbuf,
        0
    };

    if (RedisHashMultiSet(&server->redisconn, server->msgbuf, flds, vals, 0, 60 * 1000) != 0) {
        LOGGER_ERROR("RedisHashMultiSet(%s): %s", server->msgbuf, server->redisconn.errmsg);

        /* 0=拒绝新客户连接 */
        return 0;
    } else {
        LOGGER_DEBUG("RedisHashMultiSet(%s): {host=%s, port=%s}", server->msgbuf, epmsg->hbuf, epmsg->sbuf);

        /* 1=接受新客户连接 */
        return 1;
    }
}


int epcb_event_peer_close(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_PEER_CLOSE(%d): %s", epmsg->clientfd, epmsg->buf);
    return 0;
}


int epcb_event_accept(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_ACCEPT(%d)", epmsg->clientfd);
    return 0;
}


int epcb_event_reject(epollet_msg epmsg, void *arg)
{
    LOGGER_TRACE("EPEVT_REJECT(%d)", epmsg->clientfd);
    return 0;
}


int epcb_event_pollin(epollet_msg epmsg, void *arg)
{
    // 返回值:
    //   -1 没有实现, 使用默认实现
    //   0  暂时无法提供服务
    //   1  接受请求
    LOGGER_TRACE("EPEVT_POLLIN(%d): %s", epmsg->clientfd, epmsg->buf);

    int rc;
    XS_server server = (XS_server) arg;

    /**
     * 判断用户是否已经连接
     *
     * hmget xs:1:xcon:10 host port clientid
     */
    const char * fields[] = {
        "host",
        "port",
        "clientid",
        0
    };

    redisReply *reply = 0;

    XCON_redis_table_key(server->serverid, epmsg->clientfd, server->msgbuf, sizeof server->msgbuf);

    if (RedisHashMultiGet(&server->redisconn, server->msgbuf, fields, &reply) != REDISAPI_SUCCESS) {
        return 0;
    }

    LOGGER_TRACE("[%s] => {%s='%s' %s='%s' %s='%s'}", server->msgbuf,
        fields[0],
            (reply->element[0]->str? reply->element[0]->str : "(nil)"),
        fields[1],
            (reply->element[1]->str? reply->element[1]->str : "(nil)"),
        fields[2],
            (reply->element[2]->str? reply->element[2]->str : "(nil)")
    );

    rc = threadpool_unused_queues(server->pool);
    LOGGER_TRACE("threadpool_unused_queues=%d", rc);

    if (rc > 0) {
        int flags = 0;

        PollinData_t * pollin = (PollinData_t *) mem_alloc(1, sizeof(PollinData_t));

        if (reply->element[2]->type == REDIS_REPLY_STRING) {
            // 用户连接已经存在
            LOGGER_TRACE("peer connected.");

            // 复制 clientid 到 buf
            memcpy(pollin->buf, reply->element[2]->str, reply->element[2]->len);

            pollin->buf[ reply->element[2]->len ] = 0;

            flags = 1;
        } else {
            // 用户要求建立连接
            LOGGER_TRACE("peer connecting ...");

            assert(reply->element[2]->type == REDIS_REPLY_NIL);

            pollin->buf[0] = 0;

            flags = 100;
        }

        RedisFreeReplyObject(&reply);

        pollin->epollfd = epmsg->epollfd;
        pollin->epevent = epmsg->pollin_event;
        pollin->arg = arg;

        if (threadpool_add(server->pool, event_task, (void*) pollin, flags) == 0) {
            // 增加到线程池成功
            LOGGER_TRACE("threadpool_add task success");
            return 1;
        }

        // 增加到线程池失败
        LOGGER_ERROR("threadpool_add task failed");
        free(pollin);
        return 0;
    }

    RedisFreeReplyObject(&reply);

    /* queue is full */
    return 0;
}
