/***********************************************************************
* Copyright (c) 2018 pepstack, pepstack.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*   claim that you wrote the original software. If you use this software
*   in a product, an acknowledgment in the product documentation would be
*   appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
*   misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/**
 * client_api.c
 *
 * client_init_from_watch
 * 从 watch 目录初始化 client. watch 目录遵循下面的规则:
 *
 *  xsync-client-home/                           (客户端主目录)
 *            |
 *            +---- CLIENTID  (客户端唯一标识 ID, 确保名称符合文件目录名规则, 长度不大于 40 字符)
 *            |
 *            +---- LICENCE                      (客户端版权文件)
 *            |
 *            +---- bin/
 *            |       |
 *            |       +---- xsync-client-1.0.0   (客户端程序)
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
 *            |       +---- 91wifi-logserver/           (服务器名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |
 *            |       +---- ...
 *            |
 *            |
 *            |
 *            +---- watch/             (可选, 客户端 watch 目录)
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


__attribute__((used))
static void watch_event_task (thread_context_t * thread_ctx)
{
    threadpool_task_t * task = thread_ctx->task;

    get_multimer_singleton();

    if (task->flags == XS_watch_event_type_inotify) {
        XS_watch_event event = (XS_watch_event) task->argument;

        task->flags = XS_watch_event_type_none;

        LOGGER_WARN("TODO: sendfile to server");

        // MUST release event after using
        XS_watch_event_release(&event);
    } else {
        LOGGER_ERROR("unknown event type(=%d)", task->flags);
    }
}


__attribute__((used))
static XS_RESULT client_on_inotify_event (XS_client client, struct inotify_event * inevent)
{
    int num;

    xs_watch_path_t * wp = client_wd_table_lookup(client, inevent->wd);
    assert(wp && wp->watch_wd == inevent->wd);

    LOGGER_DEBUG("TODO: wd=%d mask=%d cookie=%d len=%d dir=%s. (%s/%s)",
        inevent->wd, inevent->mask, inevent->cookie, inevent->len, ((inevent->mask & IN_ISDIR) ? "yes" : "no"),
        wp->fullpath, (inevent->len > 0 ? inevent->name : "None"));

    /**
     * 如果队列空闲才能加入事件
     */
    num = XS_client_threadpool_unused_queues(client);
    LOGGER_TRACE("threadpool_unused_queues=%d", num);

    if (num > 0) {
        int sid, err;

        err = XS_client_lock(client);
        if (! err) {

            XS_watch_event events[XSYNC_SERVER_MAXID + 1] = {0};

            // client_info_makeup_events
            int sidmax = client_populate_events_inlock(client, inevent, events);

            // unlock immediately
            XS_client_unlock(client);

            for (sid = 1; sid <= sidmax; sid++) {
                XS_watch_event event = events[sid];

                err = threadpool_add(client->pool, watch_event_task, (void*) event, XS_watch_event_type_inotify);

                if (! err) {
                    // success
                    LOGGER_TRACE("threadpool_add event success");
                } else {
                    // error
                    XS_watch_event_release(&event);

                    LOGGER_WARN("threadpool_add event error(%d): %s", err, threadpool_error_messages[-err]);
                }
            }
        }
    }

    return 0;
}


/***********************************************************************
 *
 * XS_client application api
 *
 **********************************************************************/
extern XS_RESULT XS_client_create (clientapp_opts *opts, XS_client *outClient)
{
    int i, sid, err;

    int SERVERS;
    int THREADS;
    int QUEUES;

    XS_client client;

    *outClient = 0;

    client = (XS_client) mem_alloc(1, sizeof(xs_client_t));
    assert(client->thread_args == 0);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&client->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));
        free((void*) client);
        return XS_ERROR;
    };

    /* init dhlist for watch path */
    LOGGER_TRACE("dhlist_init");
    INIT_LIST_HEAD(&client->wp_dlist);
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

    if (opts->force_watch) {
        err = XS_client_conf_from_watch(client, opts->config);
    } else {
        if ( ! strcmp(strrchr(opts->config, '.'), ".xml") ) {
            err = XS_client_conf_load_xml(client, opts->config);
        } else if ( ! strcmp(strrchr(opts->config, '.'), ".ini") ) {
            err = XS_client_conf_load_ini(client, opts->config);
        } else {
            LOGGER_ERROR("unknown config: %s", opts->config);
            err = XS_E_FILE;
        }
    }

    if (err) {
        xs_client_delete((void*) client);
        return XS_ERROR;
    }

    client->threads = opts->threads;
    client->queues = opts->queues;

    SERVERS = XS_client_get_server_maxid(client);
    THREADS = client->threads;
    QUEUES = client->queues;

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        perdata->sockfds[0] = SERVERS;
        perdata->threadid = i + 1;

        for (sid = 1; sid <= XS_client_get_server_maxid(client); sid++) {
            perdata->sockfds[sid] = SOCKAPI_ERROR_SOCKET;
        }

        client->thread_args[i] = (void*) perdata;
    }

    LOGGER_DEBUG("threadpool_create: (threads=%d, queues=%d)", THREADS, QUEUES);
    client->pool = threadpool_create(THREADS, QUEUES, client->thread_args, 0);
    if (! client->pool) {
        LOGGER_FATAL("threadpool_create error: Out of memory");
        xs_client_delete((void*) client);
        return XS_ERROR;
    }

    get_multimer_singleton();

    /**
     * output XS_client
     */
    if ((err = RefObjectInit(client)) == 0) {
        *outClient = client;
        LOGGER_TRACE("xclient=%p", client);
        return XS_SUCCESS;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        xs_client_delete((void*) client);
        return XS_ERROR;
    }
}


extern XS_VOID XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, xs_client_delete);
}


extern XS_RESULT XS_client_lock (XS_client client)
{
    int err = RefObjectLock((void*) client, 1);

    if (err) {
        LOGGER_WARN("pthread_mutex_trylock error(%d): %s", err, strerror(err));
    }

    return err;
}


extern XS_VOID XS_client_unlock (XS_client client)
{
    RefObjectUnlock((void*) client);
}


extern XS_VOID XS_client_bootstrap (XS_client client)
{
    fd_set set;

    int i, sid, err, handled;

    int wait_seconds = 6;

    struct timeval timeout;
    struct inotify_event * inevent;

    char inevent_buf[XSYNC_INEVENT_BUFSIZE]__attribute__((aligned(4)));

    ssize_t len, at = 0;

    LOGGER_TRACE("event_size=%d, wait_seconds=%d", XSYNC_INEVENT_BUFSIZE, wait_seconds);

    for (i = 0; i < client->threads; ++i) {
        perthread_data * perdata = (perthread_data *) client->thread_args[i];

        // TODO: socket
        for (sid = 1; sid <= XS_client_get_server_maxid(client); sid++) {
            XS_server_opts srv = XS_client_get_server_opts(client, sid);

            int sockfd = opensocket(srv->host, srv->port, srv->sockopts.timeosec, srv->sockopts.nowait, &err);

            if (sockfd == SOCKAPI_ERROR_SOCKET) {
                LOGGER_ERROR("[thread_%d] connect server-%d (%s:%d) error(%d): %s",
                    perdata->threadid,
                    sid,
                    srv->host,
                    srv->port,
                    err,
                    strerror(errno));
            } else {
                LOGGER_INFO("[thread_%d] connected server-%d (%s:%d)",
                    perdata->threadid,
                    sid,
                    srv->host,
                    srv->port);
            }

            perdata->sockfds[sid] = sockfd;
        }
    }


    for ( ; ; ) {

        FD_ZERO(&set);

        FD_SET(client->infd, &set);

        timeout.tv_sec =  wait_seconds;
        timeout.tv_usec = 0;

        if (select(client->infd + 1, &set, 0, 0, &timeout) > 0) {

            if (FD_ISSET(client->infd, &set)) {
                handled = 0;

                /* read XSYNC_EVENT_BUFSIZE bytes’ worth of events */
                while ((len = read(client->infd, &inevent_buf, XSYNC_INEVENT_BUFSIZE)) > 0) {

                    /* loop over every read inevent until none remain */
                    at = 0;

                    while (at < len) {
                        /* here we get an inevent */
                        inevent = (struct inotify_event *) (inevent_buf + at);

                        /* handle the inevent */
                        if (inevent->len > 0 && inevent->len < XSYNC_IO_BUFSIZE - 64) {
                            handled = client_on_inotify_event(client, inevent);
                        } else {
                            if (inevent->len) {
                                LOGGER_ERROR("bad filename: %s", inevent->name);
                            } else {
                                LOGGER_ERROR("empty filename");
                            }
                        }

                        handled += 0;

                        if (inevent->mask & IN_Q_OVERFLOW) {
                            /* inotify is overflow. do getting rid of overflow in queue. */
                            // select_sleep(0, 1);
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
    struct list_head *list;

    list_for_each(list, &client->wp_dlist) {
        struct xs_watch_path_t *wp = list_entry(list, struct xs_watch_path_t, i_list);
        if (list_wp_cb(wp, data) != 1) {
            return;
        }
    }
}


extern XS_VOID XS_client_clear_watch_paths (XS_client client)
{
    struct list_head *list, *node;

    LOGGER_TRACE0();

    list_for_each_safe(list, node, &client->wp_dlist) {
        struct xs_watch_path_t * wp = list_entry(list, struct xs_watch_path_t, i_list);

        /* remove from inotify watch */
        int wd = wp->watch_wd;

        if (inotify_rm_watch(client->infd, wd) == 0) {
            LOGGER_TRACE("xpath=%p, inotify_rm_watch success(0). (%s)", wp, wp->fullpath);

            wp = client_wd_table_remove(client, wp);
            assert(wp->watch_wd == wd);
            wp->watch_wd = -1;

            hlist_del(&wp->i_hash);

            list_del(&wp->i_list);
            //?? list_del(list);

            XS_watch_path_release(&wp);
        } else {
            LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
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

        /**
         * add wd_table
         */
        client_wd_table_insert(client, wp);

        /**
         * add dlist and wp_hlist
         */
        hash = XS_watch_path_hash_get(wp->fullpath);
        assert(hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

        // 串入长串
        list_add(&wp->i_list, &client->wp_dlist);

        // 串入HASH短串
        hlist_add_head(&wp->i_hash, &client->wp_hlist[hash]);

        // 成功
        LOGGER_TRACE("xpath=%p, wd=%d. (%s)", wp, wp->watch_wd, wp->fullpath);

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
                RefObjectRetain((void**) &wp);

                *outwp = wp;
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

                hlist_del(&wp->i_hash);  //?? hlist_del(hp);
                list_del(&wp->i_list);

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
