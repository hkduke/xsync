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
 * @version: 2018-05-20 22:07:00
 *
 * @create: 2018-01-29
 *
 * @update: 2018-05-20 22:07:00
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
 * doEventTask
 *
 */
__attribute__((used))
static void doThreadTask (perthread_data *perdata)
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
                // socket是非阻塞时, EAGAIN 表示缓冲队列已满
                // 并非网络出错，而是可以再次注册事件
                //
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
            iobuf[size] = 0;

            LOGGER_DEBUG("client(%d): {%s}", connfd, iobuf);
        }
    }
}


__no_warning_unused(static)
void event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    if (task->flags == 100) {
        epollin_data_t *pdata = (epollin_data_t *) task->argument;

        task->argument = 0;
        task->flags = 0;

        perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;

        memcpy(&perdata->pollin_data, pdata, sizeof(perdata->pollin_data));

        free(pdata);

        LOGGER_DEBUG("task start");

        doThreadTask(perdata);

        LOGGER_DEBUG("task end");
    } else {
        LOGGER_ERROR("unknown task flags(=%d)", task->flags);
    }
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
static int xsyncOnEpollEvent (int msgid, int expect, epevent_data_t *epdata, void *arg)
{
    switch (msgid) {
    case EPEVENT_MSG_ERROR:
        do {
            switch (epdata->status) {
            case EPOLL_WAIT_TIMEOUT:
                LOGGER_TRACE("EPOLL_WAIT_TIMEOUT");
                break;

            case EPOLL_WAIT_EINTR:
                LOGGER_TRACE("EPOLL_WAIT_TIMEOUT");
                break;

            case EPOLL_WAIT_ERROR:
                LOGGER_WARN("EPOLL_WAIT_ERROR: %s", epdata->msg);
                break;

            case EPOLL_WAIT_OK:
                LOGGER_TRACE("EPOLL_WAIT_OK");
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
        LOGGER_TRACE("EPEVENT_PEER_ADDRIN");
        break;

    case EPEVENT_PEER_INFO:
        LOGGER_TRACE("EPEVENT_PEER_INFO: client=%d (%s:%s)", epdata->connfd, epdata->hbuf, epdata->sbuf);
        break;

    case EPEVENT_PEER_ACPTNEW:
        LOGGER_DEBUG("EPEVENT_PEER_ACPTNEW client=%d (%s:%s)", epdata->connfd, epdata->hbuf, epdata->sbuf);
        break;

    case EPEVENT_PEER_POLLIN:
        LOGGER_TRACE("EPEVENT_PEER_POLLIN client=%d", epdata->connfd);

        // 客户端有数据发送的事件发生
        expect = handleClientPollin(epdata, arg);
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

    int MAXCLIENTS = opts->maxclients;
    int THREADS = opts->threads;
    int QUEUES = opts->queues;
    int MAXEVENTS = opts->maxclients;
    int BACKLOGS = opts->somaxconn;
    int TIMEOUTMS = opts->timeout_ms;

    // TODO:
    const char dbpool_url[] = "mysql://localhost/xsyncdb?user=xsync&password=pAssW0rd";

    LOGGER_DEBUG("zdbpool_init: (url=%s, maxsize=%d)", dbpool_url, ZDBPOOL_MAX_SIZE);

    zdbpool_init(&db_pool, zdbPoolErrorHandler, dbpool_url, 0, 0, 0, 0);

    LOGGER_DEBUG("%s", db_pool.errmsg);

    *outServer = 0;

    server = (XS_server) mem_alloc(1, sizeof(xs_server_t));
    assert(server->thread_args == 0);

    server->db_pool.pool = db_pool.pool;

    __interlock_release(&server->session_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&server->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));

        free((void*) server);

        zdbpool_end(&db_pool);

        return XS_ERROR;
    }

    /* init dhlist for client_session */
    LOGGER_TRACE("hlist_init client_session");
    for (i = 0; i <= XSYNC_CLIENT_SESSION_HASHMAX; i++) {
        INIT_HLIST_HEAD(&server->client_hlist[i]);
    }

    LOGGER_INFO("threads=%d queues=%d maxclients=%d", THREADS, QUEUES, MAXCLIENTS);

    /* create per thread data */
    server->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

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
    server->maxclients = MAXCLIENTS;

    server->events = events;
    server->epollfd = epollfd;
    server->listenfd = listenfd;
    server->listenfd_oldopt = old_sockopt;

    server->numevents = MAXEVENTS;
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
        server->numevents,
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
