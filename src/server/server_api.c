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
 * @author: master@pepstack.epcb_event_peer_open
 *
 * @version: 0.4.0
 *
 * @create: 2018-01-29
 *
 * @update: 2018-11-01 16:25:38
 */

#include "server_api.h"
#include "server_conf.h"


__attribute__((used))
static void handle_peer_connect2 (perthread_data *perdata)
{
    /*
    XS_server server = (XS_server) perdata->pollin.arg;

    int epollfd = perdata->pollin.epollfd;
    int clientfd = perdata->pollin.epevent.data.fd;

    int connected = 0;
    int next = 1;

    off_t total = 0;
    off_t offset = 0;

    ssize_t count;

    // read-up all bytes
    while (next && (count = readlen_next(clientfd, perdata->buffer + offset, sizeof(perdata->buffer) - offset, &next)) >= 0) {
        offset += count;
        total += count;

        if (offset == sizeof perdata->buffer) {
            //  缓冲区已经用完, 处理数据...
            LOGGER_WARN("(thread-%d) TODO: read %ju bytes (total %ju bytes)", perdata->threadid, offset, total);

            // 重头开始再次读
            offset = 0;
        }
    }

    if (! next) {
        // 成功读光了全部数据
        if (total == XS_CONNECT_REQ_SIZE) {
            // 数据正好为连接请求尺寸
            LOGGER_INFO("(thread-%d) sock(%d): read connect request ok", perdata->threadid, clientfd);

            // xs:1:xcon:10 host port clientid
            XCON_redis_table_key(server->serverid, clientfd, perdata->buffer, sizeof perdata->buffer);

            const char * flds[] = {
                "clientid",
                0
            };

            const char * vals[] = {
                "todo_clientid",
                0
            };

            if (RedisHashMultiSet(&perdata->redconn, perdata->buffer, flds, vals, 0, -1) != 0) {
                LOGGER_ERROR("RedisHashMultiSet(%s): %s", perdata->buffer, perdata->redconn.errmsg);
            } else {
                connected = 1;
                LOGGER_INFO("RedisHashMultiSet(%s) success connected", perdata->buffer);
            }
        } else {
            // 无效的连接请求
            LOGGER_WARN("(thread-%d) sock(%d): invalid connect request(=%d)", perdata->threadid, clientfd, total);
        }
    } else if (count == -1) {
        // 读出错
        LOGGER_ERROR("(thread-%d) sock(%d): read error (%s)", perdata->threadid, clientfd, strerror(errno));
    } else if (count == -2) {
        // 客户端关闭了连接
        LOGGER_WARN("(thread-%d) sock(%d): remote closed connection", perdata->threadid, clientfd);
    }

    if (! connected) {
        // Closing the descriptor will make epoll remove it from the set of
        //  descriptors which are monitored.
        close(clientfd);
        return;
    }


    // Re-arm the socket 添加到 epollet 中
    if (epollet_ctl_mod(epollfd, &perdata->pollin.epevent, perdata->buffer, XSYNC_BUFSIZE - 1) == -1) {
        LOGGER_ERROR("(thread-%d) sock(%d): epollet_ctl_mod error: %s", perdata->threadid, clientfd, perdata->buffer);

        close(clientfd);

        connected = 0;
    }

    // TODO: 通知客户端连接已经成功建立:
    //   如果本连接是客户端接收 socket 用于消息通知, 则不要添加到 epollet 中
    //   可以将 clientfd 缓存起来 (Redis?), 当有消息需要通知客户时, 再写入消息

    count = snprintf(perdata->buffer, XSYNC_BUFSIZE, "SUCCESS");
    perdata->buffer[count] = 0;

    if (write(clientfd, perdata->buffer, count + 1) == -1) {
        LOGGER_ERROR("(thread-%d) sock(%d): write error(%d): %s", perdata->threadid, clientfd, errno, strerror(errno));

        close(clientfd);

        connected = 0;
    } else {
        LOGGER_INFO("(thread-%d) sock(%d): write SUCCESS.", perdata->threadid, clientfd);

        // Re-arm the socket 添加到 epollet 中
        if (epollet_ctl_mod(epollfd, &perdata->pollin.epevent, perdata->buffer, XSYNC_BUFSIZE - 1) == -1) {
            LOGGER_ERROR("(thread-%d) sock(%d): epollet_ctl_mod error: %s", perdata->threadid, clientfd, perdata->buffer);

            close(clientfd);

            connected = 0;
        }
    }
    */
}



__attribute__((used))
static void handle_peer_connect (perthread_data *perdata)
{
    //// XS_server server = (XS_server) perdata->pollin.arg;

    /*
    int epollfd = perdata->pollin.epollfd;
    int clientfd = perdata->pollin.epevent.data.fd;

    int next = 1;
    int cb = 0;

    // read-up all bytes
    while (next && (cb = readlen_next(clientfd, perdata->buffer, XSYNC_BUFSIZE, &next)) >= 0) {
        if (cb > 0) {
            LOGGER_DEBUG("(thread-%d) sock(%d): read %d bytes.", perdata->threadid, clientfd, cb);
        }
    }

    if (cb < 0) {
        LOGGER_ERROR("(thread-%d) sock(%d): read error(%d).", perdata->threadid, clientfd, cb);

        // Closing the descriptor will make epoll remove it from the set of
        // descriptors which are monitored.

        close(clientfd);

        return;
    }

    // 如果本连接是客户端接收 socket 用于消息通知, 则不要添加到 epollet 中
    // 可以将 clientfd 缓存起来 (Redis?), 当有消息需要通知客户时, 再写入消息

    cb = snprintf(perdata->buffer, XSYNC_BUFSIZE, "SUCCESS");
    perdata->buffer[cb] = 0;

    if (write(clientfd, perdata->buffer, cb + 1) == -1) {
        LOGGER_ERROR("(thread-%d) sock(%d): write error(%d): %s", perdata->threadid, clientfd, errno, strerror(errno));

        close(clientfd);
    } else {
        // Re-arm the socket 添加到 epollet 中
        if (epollet_ctl_mod(epollfd, &perdata->pollin.epevent, perdata->buffer, XSYNC_BUFSIZE - 1) == -1) {
            LOGGER_ERROR("(thread-%d) sock(%d): epollet_ctl_mod error: %s", perdata->threadid, clientfd, perdata->buffer);

            close(clientfd);
        }
    }
    */
}


__attribute__((unused))
void event_task (thread_context_t *thread_ctx)
{
    perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;

    threadpool_task_t *task = thread_ctx->task;

    memcpy(&perdata->pollin, (PollinData_t *) task->argument, sizeof(PollinData_t));

    mem_free_s(&task->argument);

    LOGGER_TRACE("(thread-%d) task start...", perdata->threadid);

    if (task->flags == 100) {
        handle_peer_connect(perdata);
    } else {
        // handleOldClient(perdata);
        LOGGER_ERROR("(thread-%d) unhandled task (flags=%d)", perdata->threadid, task->flags);

        // read_all_peer(perdata);
    }

    task->flags = 0;

    LOGGER_TRACE("(thread-%d) task end.", perdata->threadid);
}


extern XS_RESULT XS_server_create (xs_appopts_t *opts, XS_server *outServer)
{
    int i;

    XS_server server;

    char *hosts;
    int nodes;

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

    // create XS_server
    server = (XS_server) mem_alloc_zero(1, sizeof(xs_server_t));
    assert(server->thread_args == 0);

    // redis connection
    LOGGER_DEBUG("RedisConnInit2: cluster='%s'", opts->redis_cluster);

    nodes = RedisParseHosts(opts->redis_cluster, -1, &hosts);
    if ( ! nodes ) {
        LOGGER_ERROR("RedisConnInit2 failed. bad argument: --redis-cluster='%s'", opts->redis_cluster);
        mem_free((void*) server);
        return XS_ERROR;
    }

    if (RedisConnInit2(&server->redisconn, hosts, nodes, opts->redis_auth, redis_timeo_ms, redis_timeo_ms) != 0) {
        LOGGER_ERROR("RedisConnInit2 failed: '%s'", hosts);

        RedisMemFree(hosts);
        mem_free((void*) server);

        return XS_ERROR;
    } else {
        LOGGER_NOTICE("RedisConnInit2 success: '%s'", hosts);
    }

    __interlock_release(&server->session_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&server->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));

        RedisConnFree(&server->redisconn);

        RedisMemFree(hosts);
        mem_free((void*) server);

        return XS_ERROR;
    }

    /* TODO: init dhlist for client_session */
    LOGGER_TRACE("hlist_init client_session");
    for (i = 0; i <= XSYNC_CLIENT_SESSION_HASHMAX; i++) {
        INIT_HLIST_HEAD(&server->client_hlist[i]);
    }

    LOGGER_INFO("serverid=%s threads=%d queues=%d timeout_ms=%d", opts->serverid, THREADS, QUEUES, TIMEOUTMS);

    /* create per thread data */
    server->thread_args = (void **) mem_alloc_zero(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        // perthread_data was defined in file_entry.h
        perthread_data *perdata = (perthread_data *) mem_alloc_zero(1, sizeof(perthread_data));

        RedisConnInit2(&perdata->redconn, hosts, nodes, opts->redis_auth, redis_timeo_ms, redis_timeo_ms);

        perdata->threadid = i + 1;

        server->thread_args[i] = (void*) perdata;
    }

    RedisMemFree(hosts);

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

    LOGGER_DEBUG("epollet_conf_init(%s:%s): somaxconn=%d maxevents=%d", opts->host, opts->port, BACKLOGS, MAXEVENTS);
    do {
        if (epollet_conf_init(&server->epconf, opts->host, opts->port, BACKLOGS, MAXEVENTS, server->msgbuf, sizeof server->msgbuf) == -1) {
            LOGGER_FATAL("epollet_conf_init fail: %s", server->msgbuf);
            xs_server_delete((void*) server);
            exit(XS_ERROR);
        }

        // 设置回调函数
        /*
        server->epconf.epcb_trace = epcb_event_trace;
        server->epconf.epcb_warn = epcb_event_warn;
        server->epconf.epcb_error = epcb_event_error;
        server->epconf.epcb_new_peer = epcb_event_new_peer;
        server->epconf.epcb_accept = epcb_event_accept;
        server->epconf.epcb_reject = epcb_event_reject;
        */
        server->epconf.epcb_pollin = epcb_event_pollin;
        //server->epconf.epcb_pollout = epcb_event_pollout;

        LOGGER_INFO("epollet_conf_init: %s", server->msgbuf);
    } while (0);

    memcpy(server->host, opts->host, sizeof(opts->host));
    server->port = atoi(opts->port);

    //DEL:server->maxevents = MAXEVENTS;
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
    on_exit(exit_cleanup_server, (void *) server);

    mul_timer_start();

    /*
    epollet_msg_t epmsg;
    bzero(&epmsg, sizeof(epmsg));

    // 设置回调参数
    epmsg.userarg = server;

    // 设置回调函数: 必须全部设置
    epmsg.msg_cbs[EPEVT_TRACE] = epcb_event_trace;
    epmsg.msg_cbs[EPEVT_WARN] = epcb_event_warn;
    epmsg.msg_cbs[EPEVT_FATAL] = epcb_event_fatal;
    epmsg.msg_cbs[EPEVT_PEER_OPEN] = epcb_event_peer_open;
    epmsg.msg_cbs[EPEVT_PEER_CLOSE] = epcb_event_peer_close;
    epmsg.msg_cbs[EPEVT_ACCEPT] = epcb_event_accept;
    epmsg.msg_cbs[EPEVT_REJECT] = epcb_event_reject;
    epmsg.msg_cbs[EPEVT_POLLIN] = epcb_event_pollin;
    */

    LOGGER_INFO("epollet_loop_events starting...");

    do {
        struct epollet_event_t event;
        bzero(&event, sizeof(event));

        event.arg = (void *) server;

        // see: common/epollet.h
        epollet_loop_events(&server->epconf, &event);
    } while (0);

    mul_timer_pause();

    LOGGER_FATAL("epollet_loop_events stopped.");
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
