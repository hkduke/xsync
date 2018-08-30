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
 * @file: server_api.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.1
 *
 * @create: 2018-01-29
 *
 * @update: 2018-08-30 12:36:28
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"
#include "../redisapi/redis_api.h"

/**
 * redis-cluster 集群存储架构
 *
 * [table-1: 客户端连接表] {key=xs:serverid:connfd}
 *
 *   key         host:port           clientid
 * ---------------------------------------------
 * xs:1:10   127.0.0.1:38690        xsync-test
 * xs:1:12   127.0.0.1:38691        xsync-test
 * xs:1:14   127.0.0.1:38694        xsync-test
 * xs:1:15   127.0.0.1:38696        xsync-test
 *
 *
 *
 *
 */


__attribute__((used))
static void zdbPoolErrorHandler (const char *error)
{
    LOGGER_FATAL("%s", error);

    exit(-101);
}


__attribute__((used))
static void handleNewConnection (perthread_data *perdata)
{
    size_t cbsize, offset;

    int epollfd = perdata->pollin_data.epollfd;
    int connfd = perdata->pollin_data.connfd;

    XS_server server = (XS_server) perdata->pollin_data.arg;
    assert(server);

    offset = 0;

    while (1) {
        cbsize = read(connfd, perdata->buffer + offset, XSYNC_BUFSIZE);

        if (cbsize == 0) {
            /**
             * End of file. The remote has closed the connection.
             * 对方关闭了连接: close(fd)
             * close the descriptor will make epoll remove it
             *   from the set of descriptors which are monitored.
             **/
            LOGGER_WARN("(thread-%d) peer(%d) closed: %s", perdata->threadid, connfd, strerror(errno));
            close(connfd);
            break;
        } else if (cbsize == -1) {
            /* If errno == EAGAIN, that means we have read all data. So go back to the main loop. */
            if (errno == EAGAIN) {
                // socket 是非阻塞时, EAGAIN 表示缓冲队列已满, 并非网络出错，而是可以再次注册事件
                if (epapi_modify_epoll(epollfd, connfd, EPOLLONESHOT, perdata->pollin_data.msg, sizeof(perdata->pollin_data.msg)) == -1) {
                    LOGGER_ERROR("(thread-%d) epapi_modify_epoll error: %s", perdata->threadid, perdata->pollin_data.msg);
                    close(connfd);
                } else {
                    int accepted = 0;

                    LOGGER_DEBUG("(thread-%d) epapi_modify_epoll ok", perdata->threadid);
                    // TODO: 保存 connfd ?

                    // 此处不应该关闭，只是模拟定期清除 sessioin
                    //close(connfd);

                    if (offset == XSConnectRequestSize) {
                        LOGGER_INFO("(thread-%d) client(%d) request connecting", perdata->threadid, connfd);

                        // 校验
                        XSConnectRequest xconReq = {0};

                        if (XSConnectRequestParse((unsigned char*) perdata->buffer, &xconReq) == XS_TRUE) {
                            memcpy(perdata->buffer, xconReq.clientid, sizeof(xconReq.clientid));

                            perdata->buffer[sizeof(xconReq.clientid)] = 0;

                            LOGGER_INFO("(thread-%d): clientid(%s) msgid(%d='%c%c%c%c') magic(%d) version(%d) utctime(%d) randnum(%d)",
                                perdata->threadid,
                                perdata->buffer,
                                xconReq.msgid,
                                xconReq._request[0], xconReq._request[1], xconReq._request[2], xconReq._request[3],
                                xconReq.magic,
                                xconReq.client_version,
                                xconReq.client_utctime,
                                xconReq.randnum);
                                
                            if (xconReq.msgid == 1313817432) {
                                if (xconReq.magic == server->magic) {
                                    LOGGER_INFO("(thread-%d): clientid(%s) accepted.", perdata->threadid, perdata->buffer);

                                    accepted = 1;
                                }
                            }
                        } 
                    }

                    if (! accepted) {
                        close(connfd);

                        LOGGER_WARN("(thread-%d) sock(%d) declined.", perdata->threadid, connfd);
                    } else {
                        // TODO: 发送确认消息

                        LOGGER_WARN("(thread-%d) TODO: reply to sock(%d).", perdata->threadid, connfd);
                    }
                }

                break;
            }

            LOGGER_ERROR("(thread-%d) recv error(%d): %s", perdata->threadid, errno, strerror(errno));
            close(connfd);
            break;
        } else {
            offset += cbsize;

            // success recv data cbsize
            LOGGER_DEBUG("(thread-%d): client(%d): recv %ld bytes. total %ld bytes",
                perdata->threadid, connfd, cbsize, offset);
        }
    }
}


__no_warning_unused(static)
void event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    epollin_arg_t *pollin = (epollin_arg_t *) task->argument;

    perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;

    memcpy(&perdata->pollin_data, pollin, sizeof(perdata->pollin_data));

    task->argument = 0;
    free(pollin);

    LOGGER_TRACE("(thread-%d) task start...", perdata->threadid);

    if (task->flags == 100) {
        handleNewConnection(perdata);
    } else {
        // handleOldClient(perdata);
        LOGGER_ERROR("(thread-%d) unknown task flags(=%d)", perdata->threadid, task->flags);
    }

    task->flags = 0;

    LOGGER_TRACE("(thread-%d) task end.", perdata->threadid);
}


static inline int epmsg_cb_error_epoll_wait(epevent_msg epmsg, void *svrarg)
{
    LOGGER_ERROR("EPEVT_ERROR_EPOLL_WAIT: %s", epmsg->msgbuf);
    return 0;
}


static inline int epmsg_cb_error_epoll_event(epevent_msg epmsg, void *svrarg)
{
    LOGGER_ERROR("EPEVT_ERROR_EPOLL_EVENT: %s", epmsg->msgbuf);
    return 0;
}


static inline int epmsg_cb_accept_eagain(epevent_msg epmsg, void *svrarg)
{
    LOGGER_TRACE("EPEVT_ACCEPT_EAGAIN");
    return 0;
}


static inline int epmsg_cb_accept_ewouldblock(epevent_msg epmsg, void *svrarg)
{
    LOGGER_WARN("EPEVT_ACCEPT_EWOULDBLOCK");
    return 0;
}


static inline int epmsg_cb_error_accept(epevent_msg epmsg, void *svrarg)
{
    LOGGER_ERROR("EPEVT_ERROR_ACCEPT: %s", epmsg->msgbuf);
    return 0;
}


/**
 * 当用户发送数据
 *
 */
static inline int epmsg_cb_epoll_edge_trig(epevent_msg epmsg, void *svrarg)
{
    int num;

    XS_server server = (XS_server) svrarg;

    /**
     * 判断用户是否已经连接
     *
     * hmget xs:1:10 host port clientid
     */
    const char * fields[] = {
        "host",
        "port",
        "clientid",
        0
    };

    redisReply *reply = 0;

    snprintf(server->msgbuf, sizeof(server->msgbuf), "xs:%s:%d", server->serverid, epmsg->connfd);

    LOGGER_TRACE("EPEVT_EPOLLIN_EDGE_TRIG: sock(%d)", epmsg->connfd);

    if (RedisHashMultiGet(& server->redisconn, server->msgbuf, fields, &reply) != REDISAPI_SUCCESS) {
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

    num = threadpool_unused_queues(server->pool);
    LOGGER_TRACE("threadpool_unused_queues=%d", num);

    if (num > 0) {
        int flags = 0;
        epollin_arg_t * pollin = 0;

        if (reply->element[2]->type == REDIS_REPLY_STRING) {
            // 用户连接已经存在
            LOGGER_TRACE("peer connected.");

            pollin = (epollin_arg_t *) mem_alloc(1, sizeof(epollin_arg_t));
            
            // 复制 clientid 到 msg
            memcpy(pollin->msg, reply->element[2]->str, reply->element[2]->len);

            pollin->msg[ reply->element[2]->len ] = 0;

            flags = 1;
        } else {
            // 用户要求建立连接
            LOGGER_TRACE("peer connecting ...");

            assert(reply->element[2]->type == REDIS_REPLY_NIL);

            pollin = (epollin_arg_t *) mem_alloc(1, sizeof(epollin_arg_t));

            pollin->msg[0] = 0;

            flags = 100;
        }

        RedisFreeReplyObject(&reply);

        pollin->epollfd = epmsg->epollfd;
        pollin->connfd = epmsg->connfd;
        pollin->arg = svrarg;

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


/**
 * 当有新用户连接
 *
 */
static inline int epmsg_cb_peer_nameinfo(epevent_msg epmsg, void *svrarg)
{
    LOGGER_DEBUG("EPEVT_PEER_NAMEINFO: sock(%d)=%s:%s", epmsg->connfd, epmsg->hbuf, epmsg->sbuf);

    XS_server server = (XS_server) svrarg;

    // xs:1:10 host port clientid
    snprintf(server->msgbuf, sizeof(server->msgbuf), "xs:%s:%d", server->serverid, epmsg->connfd);

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
    } else {
        LOGGER_DEBUG("RedisHashMultiSet(%s): {host=%s, port=%s}", server->msgbuf, epmsg->hbuf, epmsg->sbuf);
    }

    return 0;
}


static inline int epmsg_cb_error_peername(epevent_msg epmsg, void *svrarg)
{
    LOGGER_ERROR("EPEVT_ERROR_PEERNAME: %s", epmsg->msgbuf);
    return 0;
}


static inline int epmsg_cb_error_setnonblock(epevent_msg epmsg, void *svrarg)
{
    LOGGER_ERROR("EPEVT_ERROR_SETNONBLOCK: %s", epmsg->msgbuf);
    return 0;
}


static inline int epmsg_cb_error_epoll_add_oneshot(epevent_msg epmsg, void *svrarg)
{
    LOGGER_ERROR("EPEVT_ERROR_EPOLL_ADD_ONESHOT: %s", epmsg->msgbuf);
    return 0;
}


static inline int epmsg_cb_peer_closed(epevent_msg epmsg, void *svrarg)
{
    LOGGER_WARN("EPEVT_PEER_CLOSED: sock(%d)", epmsg->connfd);
    return 0;
}


extern XS_RESULT XS_server_create (xs_appopts_t *opts, XS_server *outServer)
{
    int i;

    int listenfd, epollfd, old_sockopt;

    XS_server server;

    struct epoll_event *events;

    struct zdbpool_t db_pool;

    int redis_timeo_ms = 100;

    int THREADS = opts->threads;
    int QUEUES = opts->queues;
    int MAXEVENTS = opts->maxevents;
    int BACKLOGS = opts->somaxconn;
    int TIMEOUTMS = opts->timeout_ms;

    *outServer = 0;

    // TODO: hbase C 客户端
    //   https://github.com/mapr/libhbase
    //   https://github.com/mapr

    // TODO: mysql
    const char dbpool_url[] = "mysql://localhost/xsyncdb?user=xsync&password=pAssW0rd";
    LOGGER_DEBUG("zdbpool_init: (url=%s, maxsize=%d)", dbpool_url, ZDBPOOL_MAX_SIZE);
    zdbpool_init(&db_pool, zdbPoolErrorHandler, dbpool_url, 0, 0, 0, 0);
    LOGGER_DEBUG("%s", db_pool.errmsg);

    // create XS_server
    server = (XS_server) mem_alloc(1, sizeof(xs_server_t));
    assert(server->thread_args == 0);

    // redis connection
    LOGGER_DEBUG("RedisConnInit2: cluster='%s'", opts->redis_cluster);
    if (RedisConnInit2(&server->redisconn, opts->redis_cluster, opts->redis_auth, redis_timeo_ms, redis_timeo_ms) != 0) {
        LOGGER_ERROR("RedisConnInit2 failed - %s", server->redisconn.errmsg);
        free((void*) server);
        return XS_ERROR;
    }

    server->db_pool.pool = db_pool.pool;

    __interlock_release(&server->session_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&server->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));

        zdbpool_end(&db_pool);
        RedisConnFree(&server->redisconn);

        free((void*) server);
        return XS_ERROR;
    }

    /* init dhlist for client_session */
    LOGGER_TRACE("hlist_init client_session");
    for (i = 0; i <= XSYNC_CLIENT_SESSION_HASHMAX; i++) {
        INIT_HLIST_HEAD(&server->client_hlist[i]);
    }

    LOGGER_INFO("serverid=%s threads=%d queues=%d maxevents=%d somaxconn=%d timeout_ms=%d",
        opts->serverid, THREADS, QUEUES, MAXEVENTS, BACKLOGS, TIMEOUTMS);

    /* create per thread data */
    server->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        // perthread_data was defined in file_entry.h
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        RedisConnInit2(&perdata->redconn, opts->redis_cluster, opts->redis_auth, redis_timeo_ms, redis_timeo_ms);

        perdata->threadid = i + 1;

        server->thread_args[i] = (void*) perdata;
    }

    server->threads = THREADS;
    server->queues = QUEUES;

    LOGGER_DEBUG("threadpool_create: threads=%d queues=%d", THREADS, QUEUES);
    server->pool = threadpool_create(THREADS, QUEUES, server->thread_args, 0);
    if (! server->pool) {
        LOGGER_FATAL("threadpool_create error: out of memory");

        xs_server_delete((void*) server);

        // 失败退出程序
        exit(XS_ERROR);
    }

    LOGGER_INFO("epapi_create_and_bind: %s:%s", opts->host, opts->port);
    do {
        listenfd = epapi_create_and_bind(opts->host, opts->port, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (listenfd == -1) {
            LOGGER_FATAL("epapi_create_and_bind error: %s", server->msgbuf);

            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }

        old_sockopt = epapi_set_nonblock(listenfd, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (old_sockopt == -1) {
            LOGGER_FATAL("epapi_set_nonblock error: %s", server->msgbuf);

            close(listenfd);

            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }

        LOGGER_INFO("epapi_init_epoll (backlog=%d)", BACKLOGS);
        epollfd = epapi_init_epoll(listenfd, BACKLOGS, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (epollfd == -1) {
            LOGGER_FATAL("epapi_init_epoll error: %s", server->msgbuf);

            fcntl(listenfd, F_SETFL, old_sockopt);
            close(listenfd);

            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }

        LOGGER_INFO("create epoll events (maxevents=%d)", MAXEVENTS);
        events = (struct epoll_event *) calloc(MAXEVENTS, sizeof(struct epoll_event));
        if (! events) {
            LOGGER_FATAL("out of memory");

            fcntl(listenfd, F_SETFL, old_sockopt);
            close(listenfd);
            close(epollfd);

            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }
    } while(0);

    /**
     * here we got success
     *   output XS_server
     */
    server->events = events;
    server->epollfd = epollfd;
    server->listenfd = listenfd;
    server->listenfd_oldopt = old_sockopt;

    memcpy(server->host, opts->host, sizeof(opts->host));
    server->port = atoi(opts->port);

    server->maxevents = MAXEVENTS;
    server->timeout_ms = TIMEOUTMS;

    /* 服务ID: 整数表示, 整个 redis-cluster 中必须唯一 */
    snprintf(server->serverid, sizeof(server->serverid), "%s", opts->serverid);
    server->serverid[sizeof(server->serverid) - 1] = 0;

    /* 服务魔数: 用于验证用户 */
    server->magic = opts->magic;

    /**
     * output XS_server
     */
    *outServer = (XS_server) RefObjectInit(server);

    LOGGER_TRACE("%p", server);

    return XS_SUCCESS;
}


extern XS_VOID XS_server_release (XS_server *pServer)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) pServer, xs_server_delete);
}


extern int XS_server_lock (XS_server server)
{
    int err = RefObjectTryLock(server);

    if (err) {
        LOGGER_WARN("RefObjectTryLock error(%d): %s", err, strerror(err));
    }

    return err;
}


extern void XS_server_unlock (XS_server server)
{
    RefObjectUnlock(server);
}


static void exit_cleanup_server (int code, void *server)
{
    XS_server p = (XS_server) server;
    XS_server_release(&p);
}


extern XS_VOID XS_server_bootstrap (XS_server server)
{
    LOGGER_INFO("server starting epapi_loop_events ...");

    on_exit(exit_cleanup_server, (void *) server);

    mul_timer_start();

    epevent_msg_t event_msg;
    bzero(&event_msg, sizeof(epevent_msg_t));

    event_msg.msg_cb_handlers[EPEVT_EPOLLIN_EDGE_TRIG]       = epmsg_cb_epoll_edge_trig;
    event_msg.msg_cb_handlers[EPEVT_ERROR_EPOLL_WAIT]        = epmsg_cb_error_epoll_wait;
    event_msg.msg_cb_handlers[EPEVT_ERROR_EPOLL_EVENT]       = epmsg_cb_error_epoll_event;
    event_msg.msg_cb_handlers[EPEVT_ACCEPT_EAGAIN]           = epmsg_cb_accept_eagain;
    event_msg.msg_cb_handlers[EPEVT_ACCEPT_EWOULDBLOCK]      = epmsg_cb_accept_ewouldblock;
    event_msg.msg_cb_handlers[EPEVT_ERROR_ACCEPT]            = epmsg_cb_error_accept;
    event_msg.msg_cb_handlers[EPEVT_PEER_NAMEINFO]           = epmsg_cb_peer_nameinfo;
    event_msg.msg_cb_handlers[EPEVT_ERROR_PEERNAME]          = epmsg_cb_error_peername;
    event_msg.msg_cb_handlers[EPEVT_ERROR_SETNONBLOCK]       = epmsg_cb_error_setnonblock;
    event_msg.msg_cb_handlers[EPEVT_ERROR_EPOLL_ADD_ONESHOT] = epmsg_cb_error_epoll_add_oneshot;
    event_msg.msg_cb_handlers[EPEVT_PEER_CLOSED]             = epmsg_cb_peer_closed;

    epapi_loop_events(server->epollfd,
        server->listenfd,
        server->timeout_ms,
        server->events,
        server->maxevents,
        &event_msg,
        server);

    mul_timer_pause();

    LOGGER_FATAL("server stopped epapi_loop_events.");
}


extern XS_VOID XS_server_clear_client_sessions (XS_server server)
{
    int hash;
    struct hlist_node *hp, *hn;

    LOGGER_TRACE("hlist clear");

    for (hash = 0; hash <= XSYNC_CLIENT_SESSION_HASHMAX; hash++) {
        hlist_for_each_safe(hp, hn, &server->client_hlist[hash]) {
            struct xs_client_session_t *client = hlist_entry(hp, struct xs_client_session_t, i_hash);

            hlist_del(&client->i_hash);

            XS_client_session_release(&client);
        }
    }
}
