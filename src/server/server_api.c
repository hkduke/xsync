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
 * @version: 0.0.2
 *
 * @create: 2018-01-29
 *
 * @update: 2018-09-05 15:36:50
 */

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"
#include "../redisapi/redis_api.h"


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

    //epollin_arg_t *pollin = (epollin_arg_t *) task->argument;

    perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;

    //memcpy(&perdata->pollin_data, pollin, sizeof(perdata->pollin_data));

    task->argument = 0;

    //free(pollin);

    LOGGER_TRACE("(thread-%d) task start...", perdata->threadid);

    /*
    if (task->flags == 100) {
        handleNewConnection(perdata);
    } else {
        // handleOldClient(perdata);
        LOGGER_ERROR("(thread-%d) unknown task flags(=%d)", perdata->threadid, task->flags);
    }
    */

    task->flags = 0;

    LOGGER_TRACE("(thread-%d) task end.", perdata->threadid);
}


extern XS_RESULT XS_server_create (xs_appopts_t *opts, XS_server *outServer)
{
    int i;

    XS_server server;

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

    LOGGER_INFO("serverid=%s threads=%d queues=%d timeout_ms=%d", opts->serverid, THREADS, QUEUES, TIMEOUTMS);

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

    LOGGER_DEBUG("epollet_server_initiate(%s:%s): somaxconn=%d maxevents=%d", opts->host, opts->port, BACKLOGS, MAXEVENTS);
    do {
        if (epollet_server_initiate(&server->epserver, opts->host, opts->port, BACKLOGS, MAXEVENTS) == -1) {
            LOGGER_FATAL("epollet_server_initiate failed: %s", server->epserver.msg);
            xs_server_delete((void*) server);
            exit(XS_ERROR);
        }

        LOGGER_INFO("epollet_server_initiate: %s", server->epserver.msg);
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

    epollet_msg_t epmsg;

    bzero(&epmsg, sizeof(epmsg));

    // 设置回调函数
    epmsg.msg_cbs[EPEVT_TRACE] = epcb_event_trace;
    epmsg.msg_cbs[EPEVT_WARN] = epcb_event_warn;
    epmsg.msg_cbs[EPEVT_FATAL] = epcb_event_fatal;
    epmsg.msg_cbs[EPEVT_ACCEPT_NEW] = epcb_event_accept_new;
    epmsg.msg_cbs[EPEVT_ACCEPTED] = epcb_event_accepted;
    epmsg.msg_cbs[EPEVT_POLLIN] = epcb_event_pollin;
    epmsg.msg_cbs[EPEVT_PEER_CLOSE] = epcb_event_peer_close;

    LOGGER_INFO("epollet_server_loop_events ...");

    epollet_server_loop_events(&server->epserver, &epmsg, server);

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
