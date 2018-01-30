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

#include "../xmlconf.h"


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

static int lscb_add_watch_path (const char * path, int pathlen, struct dirent *ent, XS_client client)
{
    int  lnk, err;

    char resolved_path[XSYNC_PATH_MAX_SIZE];

    lnk = fileislink(path, 0, 0);

    if (isdir(path)) {
        if (lnk) {
            char * abspath = realpath(path, resolved_path);

            if (abspath) {
                XS_watch_path  wp;

                char * pathid = strrchr(path, '/') + 1;

                LOGGER_DEBUG("watch pathid=[%s] -> (%s)", pathid, abspath);

                err = XS_watch_path_create(pathid, abspath, IN_ACCESS | IN_MODIFY, &wp);

                if (err) {
                    // error
                    return 0;
                }

                if (! XS_client_add_path(client, wp)) {
                    XS_watch_path_release(&wp);
                    // error
                    return 0;
                }
            } else {
                LOGGER_ERROR("realpath error(%d): %s", errno, strerror(errno));
                return (-4);
            }
        } else {
            LOGGER_WARN("ignored path due to not a symbol link: %s", path);
        }
    }

    // continue to next
    return 1;
}


static int lscb_init_watch_path (const char * path, int pathlen, struct dirent *ent, XS_client client)
{
    int  lnk, sid, err;

    char resolved_path[XSYNC_PATH_MAX_SIZE];

    lnk = fileislink(path, 0, 0);

    if (! isdir(path)) {
        char sid_table[10];
        char sid_included[20];
        char sid_excluded[20];

        char * sidfile = realpath(path, resolved_path);

        if (! sidfile) {
            LOGGER_ERROR("realpath error(%d): %s", errno, strerror(errno));
            return (-4);
        }

        for (sid = 1; sid < XSYNC_SERVER_MAXID; sid++) {
            snprintf(sid_table, sizeof(sid_table), "%d", sid);
            snprintf(sid_included, sizeof(sid_included), "%d.included", sid);
            snprintf(sid_excluded, sizeof(sid_excluded), "%d.excluded", sid);

            if (! strcmp(sid_table, ent->d_name)) {
                // 设置 servers_opts
                assert(client->servers_opts[sid].magic == 0);

                if (lnk) {
                    LOGGER_INFO("init sid=%d from: %s -> %s", sid, path, sidfile);
                } else {
                    LOGGER_INFO("init sid=%d from: %s", sid, path);
                }

                err = server_opt_init(&client->servers_opts[sid], sid, sidfile);
                if (err) {
                    // 有错误, 中止运行
                    return 0;
                }

                if (client->servers_opts[0].sidmax < sid) {
                    client->servers_opts[0].sidmax = sid;
                }
            }

            if (! strcmp(sid_included, ent->d_name)) {
                // 读 sid.included 文件, 设置 sid 包含哪些目录文件
                err = XS_client_read_filter_file(client, path, sid, XS_path_filter_type_included);

                if (! err) {
                    LOGGER_INFO("read included filter: %s", path);
                } else {
                    LOGGER_ERROR("read included filter: %s", path);

                    // 有错误, 中止运行
                    return 0;
                }
            } else if (! strcmp(sid_excluded, ent->d_name)) {
                // 读 sid.excluded 文件, 设置 sid 排除哪些目录文件
                err = XS_client_read_filter_file(client, path, sid, XS_path_filter_type_excluded);

                if (! err) {
                    LOGGER_INFO("read excluded filter: %s", path);
                } else {
                    LOGGER_ERROR("read excluded filter: %s", path);

                    // 有错误, 中止运行
                    return 0;
                }
            }
        }
    }

    // continue to next
    return 1;
}


static int watch_path_set_sid_masks_cb (XS_watch_path wp, void * data)
{
    int sid;

    XS_client client = (XS_client) data;

    wp->sid_masks[0] = client->servers_opts[0].sidmax;

    // TODO: 设置 watch_path 的 sid_masks
    for (sid = 1; sid <= wp->sid_masks[0]; sid++) {
        // wp->included_filters[sid]

        // wp->excluded_filters[sid]

        // TODO:
        wp->sid_masks[sid] = 0xfffff;

        LOGGER_WARN("pathid: %s (sid=%d, masks=%d)", wp->pathid, sid, 0xfffff);
    }

    return 1;
}


__attribute__((used))
static int client_init_from_watch (XS_client client, const char * watchdir, char * inbuf, size_t inbufsize)
{
    int err;

    err = listdir(watchdir, inbuf, inbufsize, (listdir_callback_t) lscb_add_watch_path, (void*) client);
    if (! err) {
        err = listdir(watchdir, inbuf, inbufsize, (listdir_callback_t) lscb_init_watch_path, (void*) client);

        if (! err) {
            // 设置 watch_path 的 sid_masks
            LOGGER_TRACE("max sid=%d", client->servers_opts[0].sidmax);

            XS_client_traverse_watch_paths(client, watch_path_set_sid_masks_cb, client);
            
            // success
            return 0;
        }
    }

    return err;
}


__attribute__((used))
static int client_init_from_config (XS_client client, const char * config, char * inbuf, size_t inbufsize)
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


XS_RESULT XS_client_create (char * config, int force_watch, char * inbuf, size_t inbufsize, XS_client * outClient)
{
    int i, sid, err;

    XS_client client;

    int SERVERS = 2;
    int THREADS = 4;
    int QUEUES = 256;

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
    INIT_LIST_HEAD(&client->list1);
    for (i = 0; i <= XSYNC_PATH_HASH_MAXID; i++) {
        INIT_HLIST_HEAD(&client->hlist[i]);
    }

    client->servers_opts->sidmax = 0;
    client->infd = -1;

    /* http://www.cnblogs.com/jimmychange/p/3498862.html */
    LOGGER_DEBUG("inotify_init");
    client->infd = inotify_init();
    if (client->infd == -1) {
        LOGGER_ERROR("inotify_init() error(%d): %s", errno, strerror(errno));
        free_xsync_client((void*) client);
        return XS_ERROR;
    }

    if (force_watch) {
        err = client_init_from_watch(client, config, inbuf, inbufsize);
    } else {
        err = client_init_from_config(client, config, inbuf, inbufsize);
    }

    if (err) {
        free_xsync_client((void*) client);
        return XS_ERROR;
    }

    client->threads = THREADS;
    client->queues = QUEUES;

    validate_client_config(client);

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data * perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        perdata->sockfds[0] = SERVERS;
        perdata->threadid = i + 1;

        // TODO: socket
        for (sid = 1; sid <= client_get_sid_max(client); sid++) {
            XS_server_opts server = client_get_server_by_id(client, sid);

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
        return XS_ERROR;
    }

    /**
     * output xs_client_t object
     */
    if ((err = RefObjectInit(client)) == 0) {
        *outClient = client;
        LOGGER_TRACE("xclient=%p", client);

        XS_client_save_config_file(client, "/tmp/xsync-client-conf-debug.xml");

        return XS_SUCCESS;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        free_xsync_client((void*) client);
        return XS_ERROR;
    }
}


XS_VOID XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, free_xsync_client);
}


XS_RESULT XS_client_lock (XS_client client)
{
    return 0;
}


XS_RESULT XS_client_unlock (XS_client client)
{
    return 0;
}


void XS_client_traverse_watch_paths (XS_client client, traverse_watch_path_callback_t traverse_path_cb, void * data)
{
    struct list_head *list;

    list_for_each(list, &client->list1) {
        struct xs_watch_path_t *wp = list_entry(list, struct xs_watch_path_t, i_list);
        if (traverse_path_cb(wp, data) != 1) {
            return;
        }
    }
}


XS_VOID XS_client_clear_all_paths (XS_client client)
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


XS_BOOL XS_client_find_path (XS_client client, char * path, XS_watch_path * outwp)
{
    struct hlist_node * hp;

    // 计算 hash
    int hash = XSYNC_GET_HLIST_HASHID(path);

    LOGGER_TRACE("fullpath=%s", path);

    hlist_for_each(hp, &client->hlist[hash]) {
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


XS_BOOL XS_client_add_path (XS_client client, XS_watch_path wp)
{
    if (! XS_client_find_path(client, wp->fullpath, 0)) {
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

        return XS_TRUE;
    }

    return XS_FALSE;
}


XS_BOOL XS_client_remove_path (XS_client client, char * path)
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

                return XS_TRUE;
            } else {
                LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
                return XS_FALSE;
            }
        }
    }

    return XS_FALSE;
}


XS_VOID XS_client_listening_events (XS_client client)
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

        // MUST release event after using
        XS_watch_event_release(&event);
    }
}


XS_RESULT XS_client_on_inotify_event (XS_client client, struct inotify_event * inevent)
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


XS_RESULT XS_client_read_filter_file (XS_client client, const char * filter_file, int sid, int filter_type)
{
    int ret, lineno, err;
    char * endpath;

    char pathid_file[XSYNC_IO_BUFSIZE];
    char resolved_path[XSYNC_IO_BUFSIZE];

    FILE * fp = 0;

    ret = snprintf(pathid_file, sizeof(pathid_file), "%s", filter_file);
    if (ret < 0) {
        LOGGER_ERROR("snprintf error(%d): %s", errno, strerror(errno));
        return (-1);
    }

    if (ret >= sizeof(pathid_file)) {
        LOGGER_ERROR("insufficent buffer for file: %s", filter_file);
        return (-2);
    }

    endpath = strrchr(pathid_file, '/');
    if (! endpath) {
        LOGGER_ERROR("invalid filter file: %s", filter_file);
        return (-3);
    }

    endpath++;
    *endpath = 0;

    assert(filter_type == XS_path_filter_type_excluded || filter_type == XS_path_filter_type_included);

    fp = fopen(filter_file, "r");
    if (! fp) {
        LOGGER_ERROR("fopen error(%d): %s", errno, strerror(errno));
        return (-4);
    }

    err = -5;

    do {
        char line[XSYNC_IO_BUFSIZE];
        char * p, * filter;

        char * sid_filter = strrchr(filter_file, '/');
        sid_filter++;

        lineno = 0;

        while (fgets(line, sizeof(line), fp)) {
            lineno++;

            p = strrchr(line, '\n');
            if (p) {
                *p = 0;
            }
            p = strrchr(line, '\r');
            if (p) {
                *p = 0;
            }

            if (line[0] == '#') {
                // 忽略注释行
                LOGGER_TRACE("[%s - line: %d] %s", sid_filter, lineno, line);
                continue;
            }

            filter = strchr(line, '/');
            if (! filter) {
                LOGGER_ERROR("[%s - line: %d] %s", sid_filter, lineno, line);
                goto error_exit;
            }

            LOGGER_DEBUG("[%s - line: %d] %s", sid_filter, lineno, line);

            *filter++ = 0;

            *endpath = 0;
            strcat(pathid_file, line);

            char *fullpath = realpath(pathid_file, resolved_path);

            if (fullpath) {
                XS_watch_path wp = 0;

                if (XS_client_find_path(client, fullpath, &wp)) {
                    XS_path_filter pf = 0;

                    if (filter_type == XS_path_filter_type_excluded) {
                        pf = XS_watch_path_get_excluded_filter(wp, sid);
                    } else {
                        assert(filter_type == XS_path_filter_type_included);
                        pf = XS_watch_path_get_included_filter(wp, sid);
                    }

                    LOGGER_DEBUG("pathid=%s, pathid-file=%s", line, pathid_file);

                    ret = XS_path_filter_add_patterns(pf, filter);
                    if (ret != 0) {
                        err = -6;
                        goto error_exit;
                    }
                } else {
                    err = -7;
                    LOGGER_WARN("not found path: (pathid=%s, pathid-file=%s)", line, pathid_file);
                    goto error_exit;
                }
            } else {
                err = -8;
                LOGGER_ERROR("realpath error(%d): %s. (%s)", errno, strerror(errno), pathid_file);
                goto error_exit;
            }
        }

        // success return here
        fclose(fp);
        return 0;
    } while(0);

error_exit:
    fclose(fp);
    return err;
}


static int watch_path_set_xmlnode_cb (XS_watch_path wp, void *data)
{
    int sid, sid_max;

    mxml_node_t *watchpathsNode = (mxml_node_t *) data;

    mxml_node_t *watch_path_node;
    mxml_node_t *included_filters_node;
    mxml_node_t *excluded_filters_node;
    mxml_node_t *filter_node;

    watch_path_node = mxmlNewElement(watchpathsNode, "xs:watch-path");

    mxmlElementSetAttrf(watch_path_node, "pathid", "%s", wp->pathid);

    mxmlElementSetAttrf(watch_path_node, "fullpath", "%s", wp->fullpath);

    included_filters_node = mxmlNewElement(watch_path_node, "xs:included-filters");
    excluded_filters_node = mxmlNewElement(watch_path_node, "xs:excluded-filters");

    sid_max = wp->sid_masks[0];

    for (sid = 1; sid <= sid_max; sid++) {
        XS_path_filter filter = wp->included_filters[sid];
        if (filter) {
            filter_node = mxmlNewElement(included_filters_node, "xs:filter-patterns");

            xs_filter_set_xmlnode(filter, filter_node);
        }
    }

    for (sid = 1; sid <= sid_max; sid++) {
        XS_path_filter filter = wp->excluded_filters[sid];
        if (filter) {
            filter_node = mxmlNewElement(excluded_filters_node, "xs:filter-patterns");

            xs_filter_set_xmlnode(filter, filter_node);
        }
    }

    return 1;
}


// MXML_WS_BEFORE_OPEN, MXML_WS_AFTER_OPEN, MXML_WS_BEFORE_CLOSE, MXML_WS_AFTER_CLOSE
//
static const char * whitespace_cb(mxml_node_t *node, int where)
{
    const char *name;

    static char wrapline[] = "\n";

    if (node->type != MXML_ELEMENT) {
        return 0;
    }

    if (node->value.element.name == 0) {
        return 0;
    }

    name = node->value.element.name;

    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) {
        if (name[0] == 'x' && name[1] == 's' && name[2] == ':') {
            if (! strcmp(name, "xs:application")) {
                return "\n\t";
            }
            if (! strcmp(name, "xs:server-list")) {
                return "\n\t";
            }
            if (! strcmp(name, "xs:watch-path-list")) {
                return "\n\t";
            }

            if (! strcmp(name, "xs:server")) {
                return "\n\t\t";
            }
            if (! strcmp(name, "xs:watch-path")) {
                return "\n\t\t";
            }

            if (! strcmp(name, "xs:included-filters")) {
                return "\n\t\t\t";
            }
            if (! strcmp(name, "xs:excluded-filters")) {
                return "\n\t\t\t";
            }

            if (! strcmp(name, "xs:filter-patterns")) {
                return "\n\t\t\t\t";
            }

            if (! strcmp(name, "xs:pattern")) {
                return "\n\t\t\t\t\t";
            }

            if (! strcmp(name, "xs:sub-pattern")) {
                return "\n\t\t\t\t\t\t";
            }

            return wrapline;
        }
    }

    // 如果不需要添加空白字符则返回 0
    return 0;
}


/**
 * https://www.systutorials.com/docs/linux/man/3-mxml/
 */
XS_RESULT XS_client_save_config_file (XS_client client, const char * config_file)
{
    int sid;

    mxml_node_t *xml = 0;

    xml = mxmlNewXML("1.0");
    if (! xml) {
        LOGGER_ERROR("mxmlNewXML error: out of memory");
        return XS_ERROR;
    }

    mxml_node_t * root;
    mxml_node_t * configNode;
    mxml_node_t * serversNode;
    mxml_node_t * watchpathsNode;

    mxml_node_t * node;

    mxmlSetWrapMargin(0);

    root = mxmlNewElement(xml, "xs:xsync-client-conf");

    // 添加名称空间, 在Firefox中看不到, 在Chrome中可以
    mxmlElementSetAttr(root, "xmlns:xs", "http://github.com/pepstack/xsync");
    mxmlElementSetAttr(root, "copyright", "pepstack.com");

    configNode = mxmlNewElement(root, "xs:application");
    serversNode = mxmlNewElement(root, "xs:server-list");

    do {
        mxmlElementSetAttrf(configNode, "clientid", "%s", client->clientid);
        mxmlElementSetAttrf(configNode, "threads", "%d", client->threads);
        mxmlElementSetAttrf(configNode, "queues", "%d", client->queues);
    } while(0);

    for (sid = 1; sid <= client_get_sid_max(client); sid++) {
        XS_server_opts server = client_get_server_by_id(client, sid);

        node = mxmlNewElement(serversNode, "xs:server");

        mxmlElementSetAttrf(node, "sid", "%d", sid);
        mxmlElementSetAttrf(node, "host", "%s", server->host);
        mxmlElementSetAttrf(node, "port", "%d", server->port);
        mxmlElementSetAttrf(node, "magic", "%d", server->magic);
    } while(0);

    watchpathsNode = mxmlNewElement(root, "xs:watch-path-list");

    XS_client_traverse_watch_paths(client, watch_path_set_xmlnode_cb, watchpathsNode);

    do {
        FILE * fp;

        fp = fopen(config_file, "w");

        if (fp) {
            mxmlSaveFile(xml, fp, whitespace_cb);

            fclose(fp);
            mxmlDelete(xml);

            return XS_SUCCESS;
        }

        LOGGER_ERROR("fopen error(%d): %s. (%s)", errno, strerror(errno), config_file);
    } while(0);

    // 必须删除整个 xml
    mxmlDelete(xml);

    return XS_ERROR;
}
