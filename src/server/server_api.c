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
 * @version: 0.0.7
 *
 * @create: 2018-01-29
 *
 * @update: 2018-08-16 20:07:51
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


__no_warning_unused(static)
void event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    int flags = task->flags;

    epollin_arg_t *epdata = (epollin_arg_t *) task->argument;

    perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;

    memcpy(&perdata->pollin_data, epdata, sizeof(perdata->pollin_data));

    task->argument = 0;
    task->flags = 0;

    free(epdata);

    LOGGER_TRACE("task start");

    if (flags == 100) {
        //doRecvChunkTask(perdata);
    } else if (flags == 1) {
        //doNewConnectTask(perdata);
    } else {
        LOGGER_ERROR("unknown task flags(=%d)", flags);
    }

    LOGGER_TRACE("task end");
}


/**
 * epevent_cb
 *   回调函数
 */
__attribute__((used))
static int epevent_cb(int status, epevent_msg epmsg, void *argserver)
{
    switch (status) {
    case EPOLL_WAIT_NOREADY:
        LOGGER_TRACE("EPOLL_WAIT_NOREADY");
        break;

    case EPOLL_POLLIN_READY:
        LOGGER_DEBUG("EPOLL_POLLIN_READY: sock(%d)", epmsg->connfd);
        break;

    case EPOLL_WAIT_ERROR:
        LOGGER_DEBUG("EPOLL_WAIT_ERROR");
        break;
        
    case EPOLL_WAIT_READY:
        LOGGER_TRACE("EPOLL_WAIT_READY");
        break;
        
    case EPOLL_EVENT_ERROR:
        LOGGER_DEBUG("EPOLL_EVENT_ERROR");
        break;
        
    case EPOLL_ACCEPT_EAGAIN:
        LOGGER_TRACE("EPOLL_ACCEPT_EAGAIN");
        break;
        
    case EPOLL_ACCEPT_EWOULDBLOCK:
        LOGGER_TRACE("EPOLL_ACCEPT_EWOULDBLOCK");
        break;
        
    case EPOLL_ACCEPT_ERROR:
        LOGGER_ERROR("EPOLL_ACCEPT_ERROR");
        break;
        
    case EPOLL_GET_PEERNAME_OK:
        LOGGER_INFO("EPOLL_GET_PEERNAME_OK: sock(%d) => %s:%s", epmsg->connfd, epmsg->hbuf, epmsg->sbuf);
        break;
        
    case EPOLL_GET_PEERNAME_ERR:
        LOGGER_ERROR("EPOLL_GET_PEERNAME_ERR: sock(%d) - %s", epmsg->connfd, epmsg->msgbuf);
        break;
        
    case EPOLL_NSET_ONBLOCK_ERR:
        LOGGER_ERROR("EPOLL_NSET_ONBLOCK_ERR: sock(%d) - %s", epmsg->connfd, epmsg->msgbuf);
        break;
        
    case EPOLL_ADD_ONESHOT_ERR:
        LOGGER_ERROR("EPOLL_ADD_ONESHOT_ERR");
        break;
    }

    return 1;
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

    epapi_loop_events(server->epollfd,
        server->listenfd,
        server->events,
        server->maxevents,
        server->timeout_ms,
        epevent_cb, server);

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
