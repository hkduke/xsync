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
 */

#include "client.h"

#include "watch_path.h"


#define XSYNC_GET_HLIST_HASHID(path)   ((int)(BKDRHash(path) & XSYNC_PATH_HASH_MAXID))


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
 *            +---- conf/                        (客户端配置文件目录)
 *            |       |
 *            |       +---- xsync-client.conf    (客户端配置文件)
 *            |       |
 *            |       +---- log4crc              (客户端日志配置文件)
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
 *                    +---- 1.INCLUDE  (可选, 服务1: 包含文件的正则表达式)
 *                    |
 *                    +---- 1.EXCLUDE  (可选, 服务1: 排除文件的正则表达式)
 *                    |
 *                    +---- 2.INCLUDE  (可选)
 *                           ...
 *
 */
__attribute__((used))
static int client_init_from_watch (XS_client client, const char * watchdir)
{

    return -1;
}


__attribute__((used))
static int client_init_from_conf (XS_client client, const char * xmlconf)
{
    return -1;
}


static void free_xsync_client (void *pv)
{
    int i, sid;
    xs_client_t * client = (xs_client_t *) pv;

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&client->condition);

    int infd = client->infd;
    if (infd != -1) {
        client->infd = -1;

        LOGGER_DEBUG("close inotify");
        close(infd);
    }

    if (client->pool) {
        LOGGER_DEBUG("threadpool_destroy");
        threadpool_destroy(client->pool, 0);
    }

    if (client->thread_args) {
        perthread_data * perdata;
        int sockfd, sid_max;

        for (i = 0; i < client->threads; ++i) {
            perdata = client->thread_args[i];
            client->thread_args[i] = 0;

            sid_max = perdata->sockfds[0] + 1;

            for (sid = 1; sid < sid_max; sid++) {
                sockfd = perdata->sockfds[sid];

                perdata->sockfds[sid] = SOCKAPI_ERROR_SOCKET;

                if (sockfd != SOCKAPI_ERROR_SOCKET) {
                    close(sockfd);
                }
            }

            free(perdata);
        }

        free(client->thread_args);
    }

    // 删除全部路径
    XS_client_clear_all_paths(client);

    LOGGER_TRACE("~xclient=%p", client);

    free(client);
}


int XS_client_create (const char * xmlconf, XS_client * outClient)
{
    int i, sid, err;

    xs_client_t * client;

    int SERVERS = 2;
    int THREADS = 4;
    int QUEUES = 256;

    *outClient = 0;

    client = (xs_client_t *) mem_alloc(1, sizeof(xs_client_t));

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&client->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));
        free((void*) client);
        return (-1);
    };

    /* init dhlist for watch path */
    LOGGER_TRACE("dhlist_init");
    INIT_LIST_HEAD(&client->list1);
    for (i = 0; i <= XSYNC_PATH_HASH_MAXID; i++) {
        INIT_HLIST_HEAD(&client->hlist[i]);
    }

    client->servers_opts->servers = SERVERS;
    client->threads = THREADS;
    client->queues = QUEUES;
    client->sendfile = 1;
    client->infd = -1;

    /* populate server_opts from xmlconf */
    for (sid = 1; sid <= SERVERS; sid++) {
        xs_server_opts_t * server_opts = client_get_server_by_id(client, sid);

        server_opt_init(server_opts);

        // TODO:
    }

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data * perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        perdata->sockfds[0] = SERVERS;
        perdata->threadid = i + 1;

        // TODO: socket
        for (sid = 1; sid <= SERVERS; sid++) {
            xs_server_opts_t * server = client_get_server_by_id(client, sid);

            int sockfd = opensocket(server->host, server->port, server->sockopts.timeosec, server->sockopts.nowait, &err);

            if (sockfd == SOCKAPI_ERROR_SOCKET) {
                LOGGER_ERROR("[thread_%d] connect server-%d (%s:%d) error(%d): %s",
                    perdata->threadid,
                    sid,
                    server->host,
                    server->port,
                    err,
                    strerror(errno));
            } else {
                LOGGER_INFO("[thread_%d] connected server-%d (%s:%d)",
                    perdata->threadid,
                    sid,
                    server->host,
                    server->port);
            }

            perdata->sockfds[sid] = sockfd;
        }

        client->thread_args[i] = (void*) perdata;
    }

    LOGGER_DEBUG("threadpool_create: (threads=%d, queues=%d)", THREADS, QUEUES);
    client->pool = threadpool_create(THREADS, QUEUES, client->thread_args, 0);
    if (! client->pool) {
        LOGGER_FATAL("threadpool_create error: Out of memory");
        free_xsync_client((void*) client);
        return (-1);
    }

    /* http://www.cnblogs.com/jimmychange/p/3498862.html */
    LOGGER_DEBUG("inotify_init");
    client->infd = inotify_init();
    if (client->infd == -1) {
        LOGGER_ERROR("inotify_init() error(%d): %s", errno, strerror(errno));
        free_xsync_client((void*) client);
        return (-1);
    }

    /**
     * TODO: add watch path from XMLCONF
     */
    do {
        char pathfile[20];

        int mask = IN_ACCESS | IN_MODIFY;

        xs_watch_path_t * wp = 0;

        snprintf(pathfile, sizeof(pathfile), "/tmp");

        if (XS_watch_path_create(pathfile, mask, &wp) == 0) {
            if (! XS_client_add_path(client, wp)) {
                XS_watch_path_release(&wp);
            }
        } else {
            free_xsync_client((void*) client);
            return (-1);
        }
    } while(0);

    /**
     * output xs_client_t object */
    if ((err = RefObjectInit(client)) == 0) {
        *outClient = client;
        LOGGER_TRACE("xclient=%p", client);
        return 0;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        free_xsync_client((void*) client);
        return (-1);
    }
}


void XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, free_xsync_client);
}


int XS_client_lock (XS_client client)
{
    return 0;
}


int XS_client_unlock (XS_client client)
{
    return 0;
}


void XS_client_clear_all_paths (XS_client client)
{
    struct list_head *list, *node;

    LOGGER_TRACE0();

    list_for_each_safe(list, node, &client->list1) {
        struct xs_watch_path_t * wp = list_entry(list, struct xs_watch_path_t, i_list);

        /* remove from inotify watch */
        int wd = wp->watch_wd;

        if (inotify_rm_watch(client->infd, wd) == 0) {
            LOGGER_TRACE("xpath=%p, inotify_rm_watch success(0). (%s)", wp, wp->fullpath);

            wp = client_wd_table_remove(client, wp);
            assert(wp->watch_wd == wd);
            wp->watch_wd = -1;

            hlist_del(&wp->i_hash);
            list_del(list);

            XS_watch_path_release(&wp);
        } else {
            LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
        }
    }
}


int XS_client_find_path (XS_client client, char * path, XS_watch_path * outwp)
{
    struct hlist_node * hp;

    // 计算 hash
    int hash = XSYNC_GET_HLIST_HASHID(path);

    hlist_for_each(hp, &client->hlist[hash]) {
        struct xs_watch_path_t * wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);

        LOGGER_TRACE("xpath=%p (%s)", wp, wp->fullpath);

        if (! strcmp(wp->fullpath, path)) {
            if (outwp) {
                RefObjectRetain((void**) &wp);

                *outwp = wp;
            }

            return 1;
        }
    }

    return 0;
}


int XS_client_add_path (XS_client client, XS_watch_path wp)
{
    if (! XS_client_find_path(client, wp->fullpath, 0)) {
        int hash;

        assert(wp->watch_wd == -1);

        /**
         * add inotify watch
         */
        wp->watch_wd = inotify_add_watch(client->infd, wp->fullpath, wp->watch_mask);
        if (wp->watch_wd == -1) {
            LOGGER_ERROR("inotify_add_watch error(%d): %s", errno, strerror(errno));
            return (0);
        }

        /**
         * add wd_table
         */
        client_wd_table_insert(client, wp);

        /**
         * add dlist and hlist
         */
        hash = XSYNC_GET_HLIST_HASHID(wp->fullpath);
        assert(hash >= 0 && hash <= XSYNC_PATH_HASH_MAXID);

        // 串入长串
        list_add(&wp->i_list, &client->list1);

        // 串入HASH短串
        hlist_add_head(&wp->i_hash, &client->hlist[hash]);

        // 成功
        LOGGER_TRACE("xpath=%p, wd=%d. (%s)", wp, wp->watch_wd, wp->fullpath);

        return 1;
    }

    return 0;
}


int XS_client_remove_path (XS_client client, char * path)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int hash = XSYNC_GET_HLIST_HASHID(path);

    hlist_for_each_safe(hp, hn, &client->hlist[hash]) {
        struct xs_watch_path_t * wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);
        if (! strcmp(wp->fullpath, path)) {
            /* remove from inotify watch */
            int wd = wp->watch_wd;

            if (inotify_rm_watch(client->infd, wd) == 0) {
                LOGGER_TRACE("xpath=%p, inotify_rm_watch success(0). (%s)", wp, wp->fullpath);

                wp = client_wd_table_remove(client, wp);
                assert(wp->watch_wd == wd);
                wp->watch_wd = -1;

                hlist_del(hp);
                list_del(&wp->i_list);

                XS_watch_path_release(&wp);

                return 1;
            } else {
                LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
                return 0;
            }
        }
    }

    return 0;
}


int XS_client_listening_events (XS_client client)
{
    fd_set set;

    int handled;

    int wait_seconds = 6;

    struct timeval timeout;
    struct inotify_event * inevent;

    char inevent_buf[XSYNC_INEVENT_BUFSIZE]__attribute__((aligned(4)));

    ssize_t len, at = 0;

    LOGGER_TRACE("event_size=%d, wait_seconds=%d", XSYNC_INEVENT_BUFSIZE, wait_seconds);

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
                        handled = XS_client_on_inotify_event(client, inevent);

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


static void event_task (thread_context_t * thread_ctx)
{
    threadpool_task_t * task = thread_ctx->task;

    if (task->flags == XS_watch_event_typeid) {
        XS_watch_event event = (XS_watch_event) task->argument;
        task->flags = 0;

        sleep(3000);

        // MUST release event after using
        XS_watch_event_release(&event);
    }
}


int XS_client_on_inotify_event (XS_client client, struct inotify_event * inevent)
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
    num = client_threadpool_unused_queues(client);
    LOGGER_TRACE("threadpool_unused_queues=%d", num);

    if (num > 0) {
        int i, err;

        err = XS_client_lock(client);
        if (! err) {

            XS_watch_event events[XSYNC_SERVER_MAXID + 1];

            //TODO: num = XS_client_create_events(client, inevent, events, XSYNC_SERVER_MAXID);
            num = XSYNC_SERVER_MAXID;

            // unlock immediately
            XS_client_unlock(client);

            for (i = 0; i < num; ++i) {
                XS_watch_event event = events[i];

                err = threadpool_add(client->pool, event_task, (void*) event, 100);

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

