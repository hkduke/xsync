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
 * @version: 0.0.6
 *
 * @create: 2018-01-29
 *
 * @update: 2018-08-13 19:03:32
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"


__attribute__((used))
static void zdbPoolErrorHandler (const char *error)
{
    LOGGER_FATAL("%s", error);

    exit(-101);
}


/**
 * doRecvChunkTask
 *   处理接收文件数据请求
 */
__attribute__((used))
static void doRecvChunkTask (perthread_data *perdata)
{
    int size;

    int epollfd = perdata->pollin_data.epollfd;
    int connfd = perdata->pollin_data.connfd;

    char *iobuf = perdata->buffer;

    XS_server server = (XS_server) perdata->pollin_data.arg;
    assert(server);

    while (1) {
        // 接收数据
        size = recv(connfd, iobuf, XSYNC_BUFSIZE, 0);

        if (size == 0) {
            /**
             * 对方关闭了连接: close(fd)
             * close the descriptor will make epoll remove it
             *   from the set of descriptors which are monitored.
             **/
            LOGGER_WARN("close(%d): %s", connfd, strerror(errno));

            close(connfd);

            break;
        } else if (size < 0) {
            if (errno == EAGAIN) {
                // socket是非阻塞时, EAGAIN 表示缓冲队列已满, 并非网络出错，而是可以再次注册事件
                if (epapi_modify_epoll(epollfd, connfd, EPOLLONESHOT, iobuf, XSYNC_BUFSIZE) == -1) {
                    LOGGER_ERROR("epapi_modify_epoll error: %s", iobuf);
                    close(connfd);
                } else {
                    LOGGER_DEBUG("epapi_modify_epoll ok");
                    // TODO: 保存 connfd

                    // 此处不应该关闭，只是模拟定期清除 sessioin
                    //close(connfd);
                }
                break;
            }

            LOGGER_ERROR("recv error(%d): %s", errno, strerror(errno));
            break;
        } else {
            // success
            iobuf[size] = 0;
            LOGGER_DEBUG("client(%d): {%s}", connfd, iobuf);
        }
    }
}


/**
 * doNewConnectTask
 *    处理用户连接请求
 */
__attribute__((used))
static void doNewConnectTask (perthread_data *perdata)
{
    int size = 0;
    int offset = 0;

    int epollfd = perdata->pollin_data.epollfd;
    int connfd = perdata->pollin_data.connfd;

    char *iobuf = perdata->buffer;

    // XS_server server = (XS_server) perdata->pollin_data.arg;
    // assert(server);

    while (offset < XSConnectRequestSize) {
        // 接收连接请求头
        size = recv(connfd, iobuf + offset, XSYNC_BUFSIZE, 0);

        if (size == 0) {
            /**
             * 对方关闭了连接: close(fd)
             * close the descriptor will make epoll remove it
             *   from the set of descriptors which are monitored.
             **/
            LOGGER_WARN("client(%d) close: %s", connfd, strerror(errno));
            close(connfd);
            offset = 0;
            break;
        } else if (size < 0) {
            if (errno == EAGAIN) {
                // socket是非阻塞时, EAGAIN 表示缓冲队列已满, 并非网络出错，而是可以再次注册事件
                if (epapi_modify_epoll(epollfd, connfd, EPOLLONESHOT, iobuf, XSYNC_BUFSIZE) == -1) {
                    LOGGER_ERROR("client(%d) epapi_modify_epoll error: %s", connfd, iobuf);
                    close(connfd);
                } else {
                    LOGGER_DEBUG("epapi_modify_epoll ok");
                    // TODO: 保存 connfd
                    // 此处不应该关闭，只是模拟定期清除 sessioin
                    //close(connfd);
                }
                offset = 0;
                break;
            }

            LOGGER_ERROR("client(%d) recv error(%d): %s", connfd, errno, strerror(errno));
            offset = 0;
            break;
        } else {
            // success
            offset += size;
        }
    }

    if (offset == XSConnectRequestSize) {
        XSConnectRequest request = {0};

        XS_BOOL xb = XSConnectRequestParse((ub1 *) perdata->buffer, &request);

        if (xb) {
            LOGGER_DEBUG("client(%d) connect request: clientid={%s} magic={%d}",
                connfd, request.clientid, request.magic);

            snprintf(perdata->buffer, sizeof(perdata->buffer), "xs:clientid:%s", request.clientid);

            const char * argv[] = {
                "HGET",
                perdata->buffer,
                "magic"
            };

            redisReply * reply = RedisConnExecCommand(&perdata->redconn, sizeof(argv)/sizeof(argv[0]), argv, 0);

            if (! reply || reply->type != REDIS_REPLY_STRING) {
                LOGGER_ERROR("RedisConnExecCommand - %s", perdata->redconn.errmsg);

                RedisFreeReplyObject(&reply);

                close(connfd);
            } else {
                LOGGER_DEBUG("clientid={%s}, magic={%s}", request.clientid, reply->str);

                snprintf(perdata->buffer, sizeof(perdata->buffer), "%d", request.magic);

                if (! strcmp(perdata->buffer, reply->str)) {
                    LOGGER_INFO("client(%d): accepted client={%s}", connfd, request.clientid);

                    RedisFreeReplyObject(&reply);

                    snprintf(perdata->buffer, sizeof(perdata->buffer), "xs:sock:%d", connfd);
                    
                    // 用户通过校验, 设置状态为已连接=1
                    //
                    const char * argv[] = {
                        "HSET",
                        perdata->buffer,
                        "state",
                        "1"
                    };

                    LOGGER_DEBUG("HSET %s state 1", perdata->buffer);

                    redisReply * reply = RedisConnExecCommand(&perdata->redconn, sizeof(argv)/sizeof(argv[0]), argv, 0);

                    if (! reply) {
                        LOGGER_ERROR("no reply");
                        RedisFreeReplyObject(&reply);
                    } else {
                        LOGGER_INFO("reply type:%d", reply->type);
                        RedisFreeReplyObject(&reply);

                        const char * argv2[] = {
                            "EXPIRE",
                            perdata->buffer,
                            "600"
                        };

                        redisReply * reply2 = RedisConnExecCommand(&perdata->redconn, sizeof(argv2)/sizeof(argv2[0]), argv2, 0);
                        RedisFreeReplyObject(&reply2);
                    }                    
                } else {
                    LOGGER_WARN("client(%d): declined client={%s}", connfd, request.clientid);

                    RedisFreeReplyObject(&reply);

                    close(connfd);
                }
            }
        } else {
            LOGGER_WARN("bad connect request");
            close(connfd);
        }
    } else {
        LOGGER_ERROR("bad connect request size(%d)", offset);
        close(connfd);
    }
}


__no_warning_unused(static)
void event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    int flags = task->flags;

    epollin_data_t *epdata = (epollin_data_t *) task->argument;

    perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;

    memcpy(&perdata->pollin_data, epdata, sizeof(perdata->pollin_data));

    task->argument = 0;
    task->flags = 0;

    free(epdata);

    LOGGER_TRACE("task start");

    if (flags == 100) {
        doRecvChunkTask(perdata);
    } else if (flags == 1) {
        doNewConnectTask(perdata);
    } else {
        LOGGER_ERROR("unknown task flags(=%d)", flags);
    }

    LOGGER_TRACE("task end");
}


/**
 * handleClientConnect
 *
 *   有新的客户端请求连接
 */
__attribute__((used))
static int handleClientConnect (epevent_data_t *epdata, void *arg)
{
    XS_server server = (XS_server) arg;

    int num = threadpool_unused_queues(server->pool);
    LOGGER_TRACE("threadpool_unused_queues=%d", num);

    if (num > 0) {
        epollin_data_t *pollin_data = (epollin_data_t *) mem_alloc(1, sizeof(epollin_data_t));

        pollin_data->arg = arg;
        pollin_data->epollfd = epdata->epollfd;
        pollin_data->connfd = epdata->connfd;

        if (threadpool_add(server->pool, event_task, (void*) pollin_data, 1) == 0) {
            // 增加到线程池成功
            LOGGER_TRACE("threadpool_add task success");

            return EPCB_EXPECT_NEXT;
        }

        // 增加到线程池失败
        LOGGER_ERROR("threadpool_add task failed");

        free(pollin_data);
    }

    return EPCB_EXPECT_BREAK;
}


/**
 * handleClientPollin
 *   客户端有数据发送的事件发生
 */
__attribute__((used))
static int handleClientPollin (epevent_data_t *epdata, void *arg)
{
    XS_server server = (XS_server) arg;

    int num = threadpool_unused_queues(server->pool);
    LOGGER_TRACE("threadpool_unused_queues=%d", num);

    if (num > 0) {
        epollin_data_t *pollin_data = (epollin_data_t *) mem_alloc(1, sizeof(epollin_data_t));

        pollin_data->arg = arg;
        pollin_data->epollfd = epdata->epollfd;
        pollin_data->connfd = epdata->connfd;

        if (threadpool_add(server->pool, event_task, (void*) pollin_data, 100) == 0) {
            // 增加到线程池成功
            LOGGER_TRACE("threadpool_add task success");

            return EPCB_EXPECT_NEXT;
        }

        // 增加到线程池失败
        LOGGER_ERROR("threadpool_add task failed");

        free(pollin_data);
    }

    return EPCB_EXPECT_BREAK;
}


/**
 * xsyncOnEpollEvent
 *   回调函数
 */
__attribute__((used))
static int xsyncOnEpollEvent (int msgid, int expect, epevent_data_t *epdata, void *argserver)
{
    switch (msgid) {
    case EPEVENT_MSG_ERROR:
        do {
            switch (epdata->status) {
            case EPOLL_WAIT_TIMEOUT:
                LOGGER_TRACE("EPOLL_WAIT_TIMEOUT");
                break;

            case EPOLL_WAIT_EINTR:
                LOGGER_DEBUG("EPOLL_WAIT_TIMEOUT");
                break;

            case EPOLL_WAIT_ERROR:
                LOGGER_WARN("EPOLL_WAIT_ERROR: %s", epdata->msg);
                break;

            case EPOLL_WAIT_OK:
                LOGGER_DEBUG("EPOLL_WAIT_OK");
                break;

            case EPOLL_EVENT_NOTREADY:
                LOGGER_WARN("EPOLL_EVENT_NOTREADY");
                break;

            case EPOLL_ACPT_EAGAIN:
                LOGGER_WARN("EPOLL_ACPT_EAGAIN");
                break;

            case EPOLL_ACPT_EWOULDBLOCK:
                LOGGER_WARN("EPOLL_ACPT_EWOULDBLOCK");
                break;

            case EPOLL_ACPT_ERROR:
                LOGGER_ERROR("EPOLL_ACPT_ERROR: %s", epdata->msg);
                break;

            case EPOLL_NAMEINFO_ERROR:
                LOGGER_ERROR("EPOLL_NAMEINFO_ERROR: %s", epdata->msg);
                break;

            case EPOLL_SETNONBLK_ERROR:
                LOGGER_ERROR("EPOLL_SETNONBLK_ERROR: %s", epdata->msg);
                break;

            case EPOLL_ADDONESHOT_ERROR:
                LOGGER_ERROR("EPOLL_ADDONESHOT_ERROR: %s", epdata->msg);
                break;

            case EPOLL_ERROR_UNEXPECT:
                LOGGER_WARN("EPOLL_ERROR_UNEXPECT");
                break;
            }
        } while(0);

        return expect;

    case EPEVENT_PEER_ADDRIN:
        LOGGER_DEBUG("EPEVENT_PEER_ADDRIN: client(%d) (%s:%s)", epdata->connfd, epdata->hbuf, epdata->sbuf);
        break;

    case EPEVENT_PEER_INFO:
        LOGGER_DEBUG("EPEVENT_PEER_INFO: client(%d) (%s:%s)", epdata->connfd, epdata->hbuf, epdata->sbuf);

        do {// redis
            XS_server server = (XS_server) argserver;

            snprintf(server->msgbuf, sizeof(server->msgbuf), "xs:sock:%d", epdata->connfd);

            const char * argv[] = {
                "HMSET",
                server->msgbuf,
                "ip",
                epdata->hbuf,
                "port",
                epdata->sbuf,
                "state",
                "0"
            };

            LOGGER_DEBUG("HMSET %s ip %s port %s", server->msgbuf, epdata->hbuf, epdata->sbuf);

            redisReply * reply = RedisConnExecCommand(&server->redisconn, sizeof(argv)/sizeof(argv[0]), argv, 0);

            if (! reply) {
                LOGGER_ERROR("no reply");
            } else {
                LOGGER_INFO("reply type:%d", reply->type);
            }

            RedisFreeReplyObject(&reply);

            const char * argv2[] = {
                "EXPIRE",
                server->msgbuf,
                "10"
            };

            redisReply * reply2 = RedisConnExecCommand(&server->redisconn, sizeof(argv2)/sizeof(argv2[0]), argv2, 0);

            if (! reply2) {
                LOGGER_ERROR("no reply");
            } else {
                LOGGER_INFO("reply type:%d", reply2->type);
            }

            RedisFreeReplyObject(&reply2);
        } while(0);
        
        break;

    case EPEVENT_PEER_ACPTNEW:
        LOGGER_DEBUG("EPEVENT_PEER_ACPTNEW: client(%d) (%s:%s)", epdata->connfd, epdata->hbuf, epdata->sbuf);

        // 有新的客户端请求连接
        // expect = handleClientConnect(epdata, argserver);

        do {// redis
            XS_server server = (XS_server) argserver;

            snprintf(server->msgbuf, sizeof(server->msgbuf), "xs:sock:%d", epdata->connfd);

            const char * argv[] = {
                "HMGET",
                server->msgbuf,
                "ip",
                "port",
                "state"
            };

            LOGGER_DEBUG("HMGET %s ip port", server->msgbuf);

            redisReply * reply = RedisConnExecCommand(&server->redisconn, sizeof(argv)/sizeof(argv[0]), argv, 0);

            if (! reply) {
                LOGGER_ERROR("no reply");
                RedisFreeReplyObject(&reply);
                return EPCB_EXPECT_BREAK;
            } else {
                LOGGER_INFO("reply type:%d", reply->type);
            }

            RedisFreeReplyObject(&reply);
        } while(0);

        break;

    case EPEVENT_PEER_POLLIN:
        LOGGER_DEBUG("EPEVENT_PEER_POLLIN: client(%d)", epdata->connfd);

        do {// redis
            // 判断 client 是否连接
            XS_server server = (XS_server) argserver;

            snprintf(server->msgbuf, sizeof(server->msgbuf), "xs:sock:%d", epdata->connfd);

            const char * argv[] = {
                "HGET",
                server->msgbuf,
                "state"
            };

            LOGGER_DEBUG("HGET %s state", server->msgbuf);

            redisReply * reply = RedisConnExecCommand(&server->redisconn, sizeof(argv)/sizeof(argv[0]), argv, 0);

            if (! reply || reply->type != REDIS_REPLY_STRING || reply->len != 1) {
                LOGGER_ERROR("no reply");
                RedisFreeReplyObject(&reply);
                return EPCB_EXPECT_BREAK;
            } else {
                LOGGER_INFO("reply type=%d, str=%s", reply->type, reply->str);
                
                if (reply->str[0] == '0') {
                    // 未连接: 处理连接请求
                    RedisFreeReplyObject(&reply);
                    expect = handleClientConnect(epdata, argserver);
                    break;
                }

                if (reply->str[0] == '1') {
                    // 已经连接: 处理数据传输
                    RedisFreeReplyObject(&reply);
                    expect = handleClientPollin(epdata, argserver);
                    break;
                }
            }

            RedisFreeReplyObject(&reply);
            return EPCB_EXPECT_BREAK;
        } while(0);
        break;
    }

    return expect;
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

    // TODO: mysql
    const char dbpool_url[] = "mysql://localhost/xsyncdb?user=xsync&password=pAssW0rd";
    LOGGER_DEBUG("zdbpool_init: (url=%s, maxsize=%d)", dbpool_url, ZDBPOOL_MAX_SIZE);
    zdbpool_init(&db_pool, zdbPoolErrorHandler, dbpool_url, 0, 0, 0, 0);
    LOGGER_DEBUG("%s", db_pool.errmsg);

    // create XS_server
    server = (XS_server) mem_alloc(1, sizeof(xs_server_t));
    assert(server->thread_args == 0);

    // redis connection
    LOGGER_DEBUG("RedisConnInitiate2: cluster='%s'", opts->redis_cluster);
    if (RedisConnInitiate2(&server->redisconn, opts->redis_cluster, opts->redis_auth, redis_timeo_ms, redis_timeo_ms) != 0) {
        LOGGER_ERROR("RedisConnInitiate2 failed - %s", server->redisconn.errmsg);
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
        RedisConnRelease(&server->redisconn);

        free((void*) server);
        return XS_ERROR;
    }

    /* init dhlist for client_session */
    LOGGER_TRACE("hlist_init client_session");
    for (i = 0; i <= XSYNC_CLIENT_SESSION_HASHMAX; i++) {
        INIT_HLIST_HEAD(&server->client_hlist[i]);
    }

    LOGGER_INFO("threads=%d queues=%d maxevents=%d somaxconn=%d timeout_ms=%d", THREADS, QUEUES, MAXEVENTS, BACKLOGS, TIMEOUTMS);

    /* create per thread data */
    server->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        // perthread_data was defined in file_entry.h
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        RedisConnInitiate2(&perdata->redconn, opts->redis_cluster, opts->redis_auth, redis_timeo_ms, redis_timeo_ms);

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

        LOGGER_INFO("epapi_init_epoll_event (backlog=%d)", BACKLOGS);
        epollfd = epapi_init_epoll_event(listenfd, BACKLOGS, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (epollfd == -1) {
            LOGGER_FATAL("epapi_init_epoll_event error: %s", server->msgbuf);

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
    LOGGER_INFO("server starting ...");

    on_exit(exit_cleanup_server, (void *) server);

    mul_timer_start();

    epapi_loop_epoll_events(server->epollfd,
        server->listenfd,
        server->events,
        server->maxevents,
        server->timeout_ms,
        xsyncOnEpollEvent, server);

    mul_timer_pause();

    LOGGER_FATAL("server stopped.");
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
