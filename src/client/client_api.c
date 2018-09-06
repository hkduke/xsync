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
 * @file: client_api.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.4
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-13 15:53:46
 */

/**
 *
 * client_init_from_watch
 * 从 watch 目录初始化 client. watch 目录遵循下面的规则:
 *
 *  xsync-client-home/                           (客户端主目录)
 *            |
 *            +---- CLIENTID  (客户端唯一标识 ID, 确保名称符合文件目录名规则, 长度不大于 36 字符)
 *            |
 *            +---- LICENCE                      (客户端版权文件)
 *            |
 *            +---- bin/
 *            |       |
 *            |       +---- xsync-client-$version (客户端程序)
 *            |
 *            |
 *            +---- conf/                        (客户端配置文件目录)
 *            |       |
 *            |       +---- xsync-client.conf    (客户端配置文件)
 *            |       |
 *            |       +---- log4crc              (客户端日志配置文件)
 *            |
 *            |
 *            +---- sid/                         (服务器配置目录)
 *            |       |
 *            |       +---- pepstack-server/           (服务器名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |          ...
 *            |       |
 *            |       +---- giant-logserver/           (服务器名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |
 *            |       +---- ...
 *            |
 *            |
 *            |
 *            +---- watch/             (可选, 客户端 watch 目录 - 符号链接)
 *                    |
 *                    +---- pathlinkA -> /path/to/A/
 *                    |
 *                    +---- pathlinkB -> /path/to/B/
 *                    |
 *                    +---- pathLinkC -> /path/to/C/
 *                    |        ...
 *                    +---- pathLinkN -> /path/to/N/
 *                    |
 *                    +---- 1   (服务器 sid=1, 内容仅1行 = "host:port#magic";
 *                    |          或者为链接文件 -> ../sid/nameserver/SERVERID)
 *                    |
 *                    +---- 2   (服务器 sid=2, 内容仅1行 = "host:port#magic";
 *                    |          或者为链接文件 -> ../sid/?/SERVERID)
 *                    |    ...
 *                    |
 *                    +---- i   (服务器 sid=i. 服务器 sid顺序编制, 不可以跳跃.
 *                    |          最大服务号不可以超过255. 参考手册)
 *                    |
 *                    |     (以下文件为可选)
 *                    |
 *                    +---- 0.included  (可选, 适用于全部服务: 包含文件的正则表达式)
 *                    +---- 0.excluded  (可选, 适用于全部服务: 排除的文件的正则表达式)
 *                    |                      [全部服务  = included - excluded]
 *                    |
 *                    +---- 1.included  (可选, 服务1: 包含文件的正则表达式)
 *                    +---- 1.excluded  (可选, 服务1: 排除的文件的正则表达式)
 *                    |                      [服务1  = included - excluded]
 *                    |
 *                    +---- 2.excluded  (可选, 服务2: 排除的文件的正则表达式
 *                    |                      [服务2  = all - excluded]
 *                           ...
 *
 */
#include "client_api.h"

#include "client_conf.h"

#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/common_util.h"

// 定时器启动延时秒
#define MUL_TIMER_START_DELAY  10


__no_warning_unused(static)
int OnEventSweepWatchPath (mul_event_hdl eventhdl, int eventargid, void *eventarg, void *timerarg)
{
    mul_event_t *event = mul_handle_cast_event(eventhdl);
    printf("    => event_%lld: on_counter=%lld hash=%d\n",
        (long long) event->eventid, (long long) event->on_counter, event->hash);

    XS_client client = (XS_client) timerarg;
    XS_watch_path wp = (XS_watch_path) eventarg;

    if (XS_watch_path_sweep(wp, client) != XS_SUCCESS) {
        // 如果刷新目录失败

        // 删除事件
        mul_timer_remove_event(eventhdl);

        // 释放 watch_path
        XS_watch_path_release(&wp);

        return (-1);
    }

    return 1;
}


__no_warning_unused(static)
void event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    if (task->flags == 100) {
        XS_watch_event event = (XS_watch_event) task->argument;

        task->argument = 0;
        task->flags = 0;

        LOGGER_TRACE("thread-%d: start task=%lld (entryid=%llu, offset=%llu)",
            thread_ctx->id, (long long) event->taskid,
            (long long) event->entry->entryid,
            (long long) event->entry->offset);

        ssize_t sendbytes = XS_watch_event_sync_file(event, (perthread_data *) thread_ctx->thread_arg);

        LOGGER_TRACE("thread-%d: end task=%lld (entryid=%llu,  offset=%llu, send=%lu)",
            thread_ctx->id, (long long) event->taskid,
            (long long) event->entry->entryid,
            (long long) event->entry->offset,
            (long unsigned int) sendbytes);

        /* MUST release event after use */
        XS_watch_event_release(&event);
    } else {
        LOGGER_ERROR("unknown event task flags(=%d)", task->flags);
    }
}


__no_warning_unused(static)
XS_RESULT client_on_inotify_event (XS_client client, struct inotify_event * inevent)
{
    int err, sid, num;

    XS_watch_path wp;

    if (inevent->mask & IN_ISDIR) {
        // 目录事件
        if (! inevent->len) {
            LOGGER_FATAL("inotify error");
            return XS_E_INOTIFY;
        }

        wp = client_wd_table_lookup(client, inevent->wd);
        assert(wp && wp->watch_wd == inevent->wd);

        LOGGER_WARN("TODO: inevent dir: '%s/%s'", wp->fullpath, inevent->name);

        return XS_E_NOTIMP;
    } else {
        // 文件事件
        if (! inevent->len) {
            LOGGER_FATAL("inotify error");
            return XS_E_INOTIFY;
        }

        wp = client_wd_table_lookup(client, inevent->wd);
        assert(wp && wp->watch_wd == inevent->wd);

        LOGGER_DEBUG("inevent file: '%s/%s'", wp->fullpath, inevent->name);

        /**
         * 如果队列空闲才能加入事件
         */
        num = XS_client_threadpool_unused_queues(client);
        LOGGER_TRACE("threadpool_unused_queues=%d", num);

        if (num > 0) {
            XS_watch_event events[XSYNC_SERVER_MAXID + 1] = {0};

            num = XS_client_prepare_events(client, wp, inevent, events);

            for (sid = 1; sid <= num; sid++) {
                XS_watch_event event = events[sid];

                if (event) {
                    err = threadpool_add(client->pool, event_task, (void*) event, 100);

                    if (err) {
                        LOGGER_ERROR("threadpool_add task(=%lld) error(%d): %s",
                            (long long) event->taskid, err, threadpool_error_messages[-err]);

                        // 增加到线程池失败
                        XS_watch_event_release(&event);

                        return XS_E_POOL;
                    }

                    // 增加到线程池成功
                    LOGGER_TRACE("threadpool_add task(=%lld) success", (long long) event->taskid);
                }
            }

            return XS_SUCCESS;
        }

        LOGGER_WARN("threadpool is full");
        return XS_E_POOL;
    }
}


/***********************************************************************
 *
 * XS_client application api
 *
 **********************************************************************/
extern XS_RESULT XS_client_create (xs_appopts_t *opts, XS_client *outClient)
{
    XS_client client;

    int i, err;

    int SERVERS;
    int THREADS = opts->threads;
    int QUEUES = opts->queues;

    *outClient = 0;

    client = (XS_client) mem_alloc(1, sizeof(xs_client_t));
    assert(client->thread_args == 0);

    __interlock_release(&client->task_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&client->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));
        free((void*) client);
        return XS_ERROR;
    }

    /* init dhlist for watch path */
    LOGGER_TRACE("hlist_init");
    for (i = 0; i <= XSYNC_WATCH_PATH_HASHMAX; i++) {
        INIT_HLIST_HEAD(&client->wp_hlist[i]);
    }

    client->servers_opts->sidmax = 0;
    client->infd = -1;

    /* http://www.cnblogs.com/jimmychange/p/3498862.html */
    LOGGER_DEBUG("inotify init");
    client->infd = inotify_init();
    if (client->infd == -1) {
        LOGGER_ERROR("inotify_init() error(%d): %s", errno, strerror(errno));

        xs_client_delete((void*) client);
        return XS_ERROR;
    }

    // 创建定时器, MUL_TIMER_START_DELAY 秒之后启动. 失败退出程序
    LOGGER_DEBUG("mul_timer_init: delay %d seconds", MUL_TIMER_START_DELAY);
    if (mul_timer_init(MUL_TIMEUNIT_SEC, 1, MUL_TIMER_START_DELAY, MULTIMER_DEFAULT_HANDLER, client, 0) != 0) {
        LOGGER_FATAL("mul_timer_init error");

        xs_client_delete((void*) client);
        exit(XS_ERROR);
    }

    // 初始化客户端
    if (opts->from_watch) {
        // 从监控目录初始化客户端: 是否递归?
        err = XS_client_conf_from_watch(client, opts->config);
    } else {
        // 从 XML 配置文件初始化客户端
        if ( ! strcmp(strrchr(opts->config, '.'), ".xml") ) {
            err = XS_client_conf_load_xml(client, opts->config);
        } else {
            LOGGER_ERROR("bad config xml file: %s", opts->config);
            err = XS_E_FILE;
        }
    }

    if (err) {
        xs_client_delete((void*) client);
        return XS_ERROR;
    }

    SERVERS = XS_client_get_server_maxid(client);

    client->threads = THREADS;
    client->queues = QUEUES;

    LOGGER_INFO("threads=%d queues=%d servers=%d", THREADS, QUEUES, SERVERS);

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        // 服务器数量
        perdata->server_conns[0] = (XS_server_conn) (long) SERVERS;

        perdata->threadid = i + 1;

        client->thread_args[i] = (void*) perdata;
    }

    LOGGER_DEBUG("threadpool_create: (threads=%d, queues=%d)", THREADS, QUEUES);
    client->pool = threadpool_create(THREADS, QUEUES, client->thread_args, 0);
    if (! client->pool) {
        xs_client_delete((void*) client);

        LOGGER_FATAL("threadpool_create error: Out of memory");

        // 失败退出程序
        exit(XS_ERROR);
    }

    /**
     * output XS_client
     */
    *outClient = (XS_client) RefObjectInit(client);

    LOGGER_TRACE("client=%p", client);

    mul_timer_start();

    return XS_SUCCESS;
}


extern XS_VOID XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, xs_client_delete);
}


extern int XS_client_lock (XS_client client)
{
    int err = RefObjectTryLock(client);

    if (err) {
        LOGGER_WARN("RefObjectTryLock error(%d): %s", err, strerror(err));
    }

    return err;
}


extern XS_VOID XS_client_unlock (XS_client client)
{
    RefObjectUnlock(client);
}


extern XS_VOID XS_client_bootstrap (XS_client client)
{
    fd_set set;

    int i, sid;

    int wait_seconds = 6;

    struct timeval timeout;
    struct inotify_event * inevent;

    char inevent_buf[XSYNC_INEVENT_BUFSIZE]__attribute__((aligned(4)));

    ssize_t len, at = 0;

    LOGGER_INFO("inevent_size=%d, wait_seconds=%d", XSYNC_INEVENT_BUFSIZE, wait_seconds);

    for (i = 0; i < client->threads; ++i) {
        perthread_data * perdata = (perthread_data *) client->thread_args[i];

        if (XS_client_get_server_maxid(client) == 0) {
            LOGGER_WARN("no server specified: see reference for how to add server!");
        } else {
            for (sid = 1; sid <= XS_client_get_server_maxid(client); sid++) {
                xs_server_opts * srv = XS_client_get_server_opts(client, sid);

                XS_server_conn_create(srv, client->clientid, &perdata->server_conns[sid]);

                if (perdata->server_conns[sid]->sockfd == -1) {
                    LOGGER_ERROR("[thread_%d] connect server-%d (%s:%d)",
                        perdata->threadid,
                        sid,
                        srv->host,
                        srv->port);
                } else {
                    LOGGER_INFO("[thread_%d] connected server-%d (%s:%d)",
                        perdata->threadid,
                        sid,
                        srv->host,
                        srv->port);
                }
            }
        }
    }

    // 启动定时器
    mul_timer_start();

    // 开始 inotify
    LOGGER_INFO("inotify start running...");

    for ( ; ; ) {
        FD_ZERO(&set);

        // TODO: https://stackoverflow.com/questions/7976388/increasing-limit-of-fd-setsize-and-select
        assert(client->infd < FD_SETSIZE);

        FD_SET(client->infd, &set);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        if (select(client->infd + 1, &set, 0, 0, &timeout) > 0) {

            if (FD_ISSET(client->infd, &set)) {
                /* read XSYNC_EVENT_BUFSIZE bytes’ worth of events */
                while ((len = read(client->infd, &inevent_buf, XSYNC_INEVENT_BUFSIZE)) > 0) {

                    /* loop over every read inevent until none remain */
                    at = 0;

                    while (at < len) {
                        /* here we get an inevent */
                        inevent = (struct inotify_event *) (inevent_buf + at);

                        if (inevent->mask & IN_Q_OVERFLOW) {
                            /* inotify is overflow */
                            LOGGER_WARN("IN_Q_OVERFLOW");

                            break;
                        } else if (inevent->len) {
                            //TODO: why '/tmp/#25'
                            if (inevent->name[0] != '#') {
                                LOGGER_DEBUG("inevent: wd=%d mask=%d cookie=%d len=%d dir=%c name='%s'",
                                    inevent->wd, inevent->mask, inevent->cookie, inevent->len,
                                    ((inevent->mask & IN_ISDIR)? 'Y' : 'N'),
                                    (inevent->len? inevent->name : ""));

                                /* handle the inevent */
                                client_on_inotify_event(client, inevent);
                            }
                        }

                        /* update the index to the start of the next event */
                        at += sizeof(struct inotify_event) + inevent->len;
                    }
                }
            }
        }
    }
}


extern XS_VOID XS_client_list_watch_paths (XS_client client, list_watch_path_cb_t list_wp_cb, void * data)
{
    int hash;
    struct hlist_node *hp, *hn;

    for (hash = 0; hash <= XSYNC_WATCH_PATH_HASHMAX; hash++) {
        hlist_for_each_safe(hp, hn, &client->wp_hlist[hash]) {
            struct xs_watch_path_t *wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);

            if (list_wp_cb(wp, data) != 1) {
                return;
            }
        }
    }
}


extern XS_VOID XS_client_clear_watch_paths (XS_client client)
{
    int hash;
    struct hlist_node *hp, *hn;

    LOGGER_TRACE("hlist clear");

    for (hash = 0; hash <= XSYNC_WATCH_PATH_HASHMAX; hash++) {
        hlist_for_each_safe(hp, hn, &client->wp_hlist[hash]) {
            struct xs_watch_path_t *wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);

            /* remove from inotify watch */
            int wd = wp->watch_wd;

            if (inotify_rm_watch(client->infd, wd) == 0) {
                LOGGER_TRACE("xpath=%p, inotify_rm_watch success(0). (%s)", wp, wp->fullpath);

                wp = client_wd_table_remove(client, wp);
                assert(wp->watch_wd == wd);
                wp->watch_wd = -1;

                hlist_del(&wp->i_hash);

                XS_watch_path_release(&wp);
            } else {
                LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
            }
        }
    }
}


extern XS_BOOL XS_client_add_watch_path (XS_client client, XS_watch_path wp)
{
    if (! XS_client_find_watch_path(client, wp->fullpath, 0)) {
        int hash;

        assert(wp->watch_wd == -1);

        /**
         * add inotify watch
         */
        assert(client->infd != -1);

        wp->watch_wd = inotify_add_watch(client->infd, wp->fullpath, wp->events_mask);

        if (wp->watch_wd == -1) {
            LOGGER_ERROR("inotify_add_watch error(%d): %s", errno, strerror(errno));
            return XS_FALSE;
        }

        // 设置多定时器: 刷新目录事件. 增加 watch_path 计数
        RefObjectRetain((void**) &wp);

        if (mul_timer_set_event(XSYNC_SWEEP_PATH_INTERVAL + (wp->watch_wd & XSYNC_WATCH_PATH_HASHMAX),
                XSYNC_SWEEP_PATH_INTERVAL,
                MULTIMER_EVENT_INFINITE,
                OnEventSweepWatchPath,
                wp->watch_wd,
                (void*) wp,
                MULTIMER_EVENT_CB_BLOCK) <= 0)
        {
            int wd = wp->watch_wd;
            wp->watch_wd = -1;

            inotify_rm_watch(client->infd, wd);

            XS_watch_path_release(&wp);

            LOGGER_FATAL("mul_timer_set_event failed");
            exit(XS_ERROR);
        }

        LOGGER_DEBUG("mul_timer_set_event ok. (%s)", wp->fullpath);

        client_wd_table_insert(client, wp);

        hash = XS_watch_path_hash_get(wp->fullpath);
        assert(hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

        hlist_add_head(&wp->i_hash, &client->wp_hlist[hash]);

        LOGGER_TRACE("%p, wd=%d. (%s)", wp, wp->watch_wd, wp->fullpath);
        return XS_TRUE;
    }

    return XS_FALSE;
}


extern XS_BOOL XS_client_find_watch_path (XS_client client, char * path, XS_watch_path * outwp)
{
    struct hlist_node * hp;

    // 计算 hash
    int hash = XS_watch_path_hash_get(path);

    LOGGER_TRACE("fullpath=%s", path);

    hlist_for_each(hp, &client->wp_hlist[hash]) {
        struct xs_watch_path_t * wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);

        LOGGER_TRACE("xpath=%p (%s)", wp, wp->fullpath);

        if (! strcmp(wp->fullpath, path)) {
            if (outwp) {
                *outwp = (XS_watch_path) RefObjectRetain((void**) &wp);
            }

            return XS_TRUE;
        }
    }

    return XS_FALSE;
}


extern XS_BOOL XS_client_remove_watch_path (XS_client client, char * path)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int hash = XS_watch_path_hash_get(path);

    hlist_for_each_safe(hp, hn, &client->wp_hlist[hash]) {
        struct xs_watch_path_t * wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);
        if (! strcmp(wp->fullpath, path)) {
            /* remove from inotify watch */
            int wd = wp->watch_wd;

            if (inotify_rm_watch(client->infd, wd) == 0) {
                LOGGER_TRACE("xpath=%p, inotify_rm_watch success(0). (%s)", wp, wp->fullpath);

                wp = client_wd_table_remove(client, wp);
                assert(wp->watch_wd == wd);
                wp->watch_wd = -1;

                //?? hlist_del(hp);
                hlist_del(&wp->i_hash);

                XS_watch_path_release(&wp);

                return XS_TRUE;
            } else {
                LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
                return XS_FALSE;
            }
        }
    }

    return XS_FALSE;
}
