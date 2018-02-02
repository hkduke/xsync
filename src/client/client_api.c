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

#include "../common/readconf.h"

#include "../xmlconf.h"


#define XS_watch_path_hash_get(path)   ((int)(BKDRHash(path) & XSYNC_WATCH_PATH_HASHMAX))

#define XS_watch_id_hash_get(wd)       ((int)((wd) & XSYNC_WATCH_PATH_HASHMAX))

#define XS_entry_map_hash_get(path)    ((int)(BKDRHash(path) & XSYNC_WATCH_ENTRY_HASHMAX))


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


/**
 * insert wd into wd_table of client
 */
__attribute__((used))
static inline void client_wd_table_insert (XS_client client, XS_watch_path wp)
{
    int hash = XS_watch_id_hash_get(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

    wp->next = client->wd_table[hash];
    client->wd_table[hash] = wp;
}


__attribute__((used))
static inline xs_watch_path_t * client_wd_table_lookup (XS_client client, int wd)
{
    xs_watch_path_t * wp;

    int hash = XS_watch_id_hash_get(wd);
    assert(wd != -1 && hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

    wp = client->wd_table[hash];
    while (wp) {
        if (wp->watch_wd == wd) {
            return wp;
        }
        wp = wp->next;
    }
    return wp;
}


__attribute__((used))
static inline XS_watch_path client_wd_table_remove (XS_client client, XS_watch_path wp)
{
    xs_watch_path_t * lead;
    xs_watch_path_t * node;

    int hash = XS_watch_id_hash_get(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

    // 特殊处理头节点
    lead = client->wd_table[hash];
    if (! lead || lead == wp) {
        if (lead) {
            client->wd_table[hash] = lead->next;
            lead->next = 0;
        }
        return lead;
    }

    // lead->node->...
    node = lead->next;
    while (node && node != wp) {
        lead = node;
        node = node->next;
    }

    if (node == wp) {
        // found wd
        lead->next = node->next;
        node->next = 0;
    }

    return node;
}


__attribute__((used))
static inline int client_get_sid_max (XS_client client)
{
    return client->servers_opts->sidmax;
}


__attribute__((used))
static inline int client_threadpool_unused_queues (XS_client client)
{
    return threadpool_unused_queues(client->pool);
}


__attribute__((used))
static inline xs_server_opts_t * client_get_server_by_id (XS_client client, int sid /* 1 based */)
{
    assert(sid > 0 && sid <= client->servers_opts->sidmax);
    return client->servers_opts + sid;
}


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
static int client_init_from_watch (XS_client client, const char * watchdir)
{
    int err;

    char inbuf[XSYNC_PATH_MAX_SIZE];

    err = listdir(watchdir, inbuf, sizeof(inbuf), (listdir_callback_t) lscb_add_watch_path, (void*) client);
    if (! err) {
        err = listdir(watchdir, inbuf, sizeof(inbuf), (listdir_callback_t) lscb_init_watch_path, (void*) client);

        if (! err) {
            XS_client_traverse_watch_paths(client, watch_path_set_sid_masks_cb, client);

            return XS_SUCCESS;
        }
    }

    return err;
}


__attribute__((used))
static int client_init_from_config (XS_client client, const char * config_file)
{
    int err = XS_ERROR;

    if ( ! strcmp(strrchr(config_file, '.'), ".xml") ) {
        err = XS_client_load_conf_xml(client, config_file);
    } else if ( ! strcmp(strrchr(config_file, '.'), ".ini") ) {
        err = XS_client_load_conf_ini(client, config_file);
    } else {
        LOGGER_ERROR("unknown config file: %s", config_file);
        err = XS_E_FILE;
    }

    return err;
}


__attribute__((used))
static XS_RESULT client_add_watch_entry (XS_client client, XS_watch_entry entry)
{
    //XS_entry_map_hash_get


    return 0;
}


__attribute__((used))
static int client_create_events (XS_client client, struct inotify_event * inevent, XS_watch_event events[], int maxsid)
{

    return 0;
}

__attribute__((used))
static void xs_client_free (void *pv)
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

/**
 * public api for XS_client
 *

extern XS_RESULT XS_client_create (char *config, int config_type, XS_client *outclient)
{

extern XS_RESULT XS_client_bootstrap (XS_client client);
 */
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
    INIT_LIST_HEAD(&client->list1);
    for (i = 0; i <= XSYNC_WATCH_PATH_HASHMAX; i++) {
        INIT_HLIST_HEAD(&client->hlist[i]);
    }

    client->servers_opts->sidmax = 0;
    client->infd = -1;

    /* http://www.cnblogs.com/jimmychange/p/3498862.html */
    LOGGER_DEBUG("inotify_init");
    client->infd = inotify_init();
    if (client->infd == -1) {
        LOGGER_ERROR("inotify_init() error(%d): %s", errno, strerror(errno));
        xs_client_free((void*) client);
        return XS_ERROR;
    }

    if (opts->force_watch) {
        err = client_init_from_watch(client, opts->config);
    } else {
        err = client_init_from_config(client, opts->config);
    }

    if (err) {
        xs_client_free((void*) client);
        return XS_ERROR;
    }

    client->threads = opts->threads;
    client->queues = opts->queues;

    SERVERS = client_get_sid_max(client);
    THREADS = client->threads;
    QUEUES = client->queues;

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
        xs_client_free((void*) client);
        return XS_ERROR;
    }

    /**
     * output xs_client_t object
     */
    if ((err = RefObjectInit(client)) == 0) {
        *outClient = client;
        LOGGER_TRACE("xclient=%p", client);

        XS_client_save_conf_xml(client, "/tmp/xsync-client-debug.conf");

        return XS_SUCCESS;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        xs_client_free((void*) client);
        return XS_ERROR;
    }
}


XS_VOID XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, xs_client_free);
}


XS_RESULT XS_client_lock (XS_client client)
{
    int err = RefObjectLock((void*) client, 1);

    if (err) {
        LOGGER_WARN("pthread_mutex_trylock error(%d): %s", err, strerror(err));
    }

    return err;
}


XS_VOID XS_client_unlock (XS_client client)
{
    RefObjectUnlock((void*) client);
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
    int hash = XS_watch_path_hash_get(path);

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
        hash = XS_watch_path_hash_get(wp->fullpath);
        assert(hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

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

    int hash = XS_watch_path_hash_get(path);

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


XS_VOID XS_client_bootstrap (XS_client client)
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

            // client_info_makeup_events
            num = client_create_events(client, inevent, events, XSYNC_SERVER_MAXID);

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


__attribute__((used))
static int list_server_cb (mxml_node_t *server_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(server_node, "sid", 0, attr);
    xmlconf_element_attr_read(server_node, "host", 0, attr);
    xmlconf_element_attr_read(server_node, "port", 0, attr);
    xmlconf_element_attr_read(server_node, "magic", 0, attr);

    return 1;
}


__attribute__((used))
static int list_sub_pattern_cb (mxml_node_t *subpattern_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(subpattern_node, "regex", 0, attr);
    xmlconf_element_attr_read(subpattern_node, "script", 0, attr);

    return 1;
}


__attribute__((used))
static int list_pattern_cb (mxml_node_t *pattern_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(pattern_node, "regex", 0, attr);
    xmlconf_element_attr_read(pattern_node, "script", 0, attr);

    xmlconf_list_mxml_nodes(pattern_node, "xs:sub-pattern", (xmlconf_list_node_cb_t) list_sub_pattern_cb, (void *) client);

    return 1;
}


__attribute__((used))
static int list_included_filter_cb (mxml_node_t *filter_node, XS_client client)
{
    const char * attr;
    xmlconf_element_attr_read(filter_node, "sid", 0, attr);
    xmlconf_element_attr_read(filter_node, "size", 0, attr);

    return 1;
}


__attribute__((used))
static int list_excluded_filter_cb (mxml_node_t *filter_node, XS_client client)
{
    const char * attr;
    xmlconf_element_attr_read(filter_node, "sid", 0, attr);
    xmlconf_element_attr_read(filter_node, "size", 0, attr);

    xmlconf_list_mxml_nodes(filter_node, "xs:pattern", (xmlconf_list_node_cb_t) list_pattern_cb, (void *) client);

    return 1;
}


__attribute__((used))
static int list_watch_path_cb (mxml_node_t *wp_node, XS_client client)
{
    const char * attr;
    mxml_node_t * node;

    xmlconf_element_attr_read(wp_node, "pathid", 0, attr);
    xmlconf_element_attr_read(wp_node, "fullpath", 0, attr);

    xmlconf_find_element_node(wp_node, "xs:included-filters", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:filter-patterns", (xmlconf_list_node_cb_t) list_included_filter_cb, (void *) client);

    }

    xmlconf_find_element_node(wp_node, "xs:excluded-filters", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:filter-patterns", (xmlconf_list_node_cb_t) list_excluded_filter_cb, (void *) client);
    }

    return 1;

error_exit:
    return 0;
}


XS_RESULT XS_client_load_conf_xml (XS_client client, const char * config_file)
{
    FILE *fp;

    const char * attr;

    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *node;

    if (strcmp(strrchr(config_file, '.'), ".xml")) {
        LOGGER_ERROR("config file end with not '.xml': %s", config_file);
        return XS_E_PARAM;
    }

    if (strstr(strrchr(config_file, '/'), XSYNC_CLIENT_APPNAME) !=  strrchr(config_file, '/') + 1) {
        LOGGER_ERROR("config file start with not '%s': %s", XSYNC_CLIENT_APPNAME, config_file);
        return XS_E_PARAM;
    }

    fp = fopen(config_file, "r");
    if (! fp) {
        LOGGER_ERROR("fopen error(%d): %s. (%s)", errno, strerror(errno), config_file);
        return XS_ERROR;
    }

    xml = mxmlLoadFile(0, fp, MXML_TEXT_CALLBACK);
    if (! xml) {
        LOGGER_ERROR("mxmlLoadFile error. (%s)", config_file);
        fclose(fp);
        return XS_ERROR;
    }

    xmlconf_find_element_node(xml, "xs:"XSYNC_CLIENT_APPNAME"-conf", root);
    {
        xmlconf_element_attr_check(root, "xmlns:xs", XSYNC_CLIENT_XMLNS, attr);
        xmlconf_element_attr_check(root, "copyright", XSYNC_COPYRIGHT, attr);
        xmlconf_element_attr_check(root, "version", XSYNC_CLIENT_VERSION, attr);
    }

    xmlconf_find_element_node(root, "xs:application", node);
    {
        xmlconf_element_attr_read(node, "clientid", 0, attr);
        xmlconf_element_attr_read(node, "threads", 0, attr);
        xmlconf_element_attr_read(node, "queues", 0, attr);
    }

    xmlconf_find_element_node(root, "xs:server-list", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:server", (xmlconf_list_node_cb_t) list_server_cb, (void *) client);
    }

    xmlconf_find_element_node(root, "xs:watch-path-list", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:watch-path", (xmlconf_list_node_cb_t) list_watch_path_cb, (void *) client);
    }

    mxmlDelete(xml);
    fclose(fp);

    return XS_SUCCESS;

error_exit:

    mxmlDelete(xml);
    fclose(fp);

    return XS_ERROR;
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
XS_RESULT XS_client_save_conf_xml (XS_client client, const char * config_file)
{
    int sid;

    mxml_node_t *xml;
    mxml_node_t *node;
    mxml_node_t *root;
    mxml_node_t *configNode;
    mxml_node_t *serversNode;
    mxml_node_t *watchpathsNode;

    if (strcmp(strrchr(config_file, '.'), ".xml")) {
        LOGGER_ERROR("config file end with not '.xml': %s", config_file);
        return XS_E_PARAM;
    }

    if (strstr(strrchr(config_file, '/'), XSYNC_CLIENT_APPNAME) !=  strrchr(config_file, '/') + 1) {
        LOGGER_ERROR("config file start with not '%s': %s", XSYNC_CLIENT_APPNAME, config_file);
        return XS_E_PARAM;
    }

    xml = mxmlNewXML("1.0");
    if (! xml) {
        LOGGER_ERROR("mxmlNewXML error: out of memory");
        return XS_ERROR;
    }

    mxmlSetWrapMargin(0);

    root = mxmlNewElement(xml, "xs:"XSYNC_CLIENT_APPNAME"-conf");

    // 添加名称空间, 在Firefox中看不到, 在Chrome中可以
    mxmlElementSetAttr(root, "xmlns:xs", XSYNC_CLIENT_XMLNS);
    mxmlElementSetAttr(root, "copyright", XSYNC_COPYRIGHT);
    mxmlElementSetAttr(root, "version", XSYNC_CLIENT_VERSION);
    mxmlElementSetAttr(root, "author", XSYNC_AUTHOR);

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

            mxmlDelete(xml);

            fclose(fp);

            return XS_SUCCESS;
        }

        LOGGER_ERROR("fopen error(%d): %s. (%s)", errno, strerror(errno), config_file);
    } while(0);

    // 必须删除整个 xml
    mxmlDelete(xml);

    return XS_ERROR;
}



XS_RESULT XS_client_load_conf_ini (XS_client client, const char * config_ini)
{
    int i, numsecs, sidmax;
    void *seclist;

    char *key, *val, *sec;

    char *family;
    char *qualifier;

    char sid_filter[20];

    char secname[CONF_MAX_SECNAME];

    LOGGER_DEBUG("[xsync-client-conf]");
    if (ConfReadValueRef(config_ini, "xsync-client-conf", "namespace", &val)) {
        LOGGER_DEBUG("  namespace=%s", val);
    } else {
        LOGGER_WARN("not found key: 'namespace'");
    }
    if (ConfReadValueRef(config_ini, "xsync-client-conf", "copyright", &val)) {
        LOGGER_DEBUG("  copyright=%s", val);
    } else {
        LOGGER_WARN("not found key: 'copyright'");
    }
    if (ConfReadValueRef(config_ini, "xsync-client-conf", "version", &val)) {
        LOGGER_DEBUG("  version=%s", val);
    } else {
        LOGGER_WARN("not found key: 'version'");
    }
    if (ConfReadValueRef(config_ini, "xsync-client-conf", "author", &val)) {
        LOGGER_DEBUG("  author=%s", val);
    } else {
        LOGGER_WARN("not found key: 'author'");
    }

    LOGGER_DEBUG("[application]");
    if (ConfReadValueRef(config_ini, "application", "threads", &val)) {
        LOGGER_DEBUG("  threads=%s", val);
    } else {
        LOGGER_WARN("not found key: 'threads'");
    }
    if (ConfReadValueRef(config_ini, "application", "clientid", &val)) {
        LOGGER_DEBUG("  clientid=%s", val);
    } else {
        LOGGER_WARN("not found key: 'clientid'");
    }
    if (ConfReadValueRef(config_ini, "application", "queues", &val)) {
        LOGGER_DEBUG("  queues=%s", val);
    } else {
        LOGGER_WARN("not found key: 'queues'");
    }

    // [server:id]
    sidmax = 0;
    numsecs = ConfGetSectionList(config_ini, &seclist);
    for (i = 0; i < numsecs; i++) {
        sec = ConfSectionListGetAt(seclist, i);

        strcpy(secname, sec);

        if (ConfSectionParse(secname, &family, &qualifier) == 2) {
            if (! strcmp(family, "server")) {
                LOGGER_DEBUG("[%s:%s]", family, qualifier);

                ConfReadValueRef(config_ini, sec, "host", &val);
                LOGGER_DEBUG("  host=%s", val);

                ConfReadValueRef(config_ini, sec, "port", &val);
                LOGGER_DEBUG("  port=%s", val);

                ConfReadValueRef(config_ini, sec, "magic", &val);
                LOGGER_DEBUG("  magic=%s", val);

                sidmax++;
            }
        }
    }
    ConfSectionListFree(seclist);

    // [watch-path:id]
    numsecs = ConfGetSectionList(config_ini, &seclist);
    for (i = 0; i < numsecs; i++) {
        sec = ConfSectionListGetAt(seclist, i);

        strcpy(secname, sec);

        if (ConfSectionParse(secname, &family, &qualifier) == 2) {
            if (! strcmp(family, "watch-path")) {
                LOGGER_DEBUG("[%s:%s]", family, qualifier);

                ConfReadValueRef(config_ini, sec, "fullpath", &val);
                LOGGER_DEBUG("  fullpath=%s", val);

                CONF_position cpos = ConfOpenFile(config_ini);
                char * str = ConfGetFirstPair(cpos, &key, &val);
                while (str) {
                    if (! strcmp(ConfGetSection(cpos), sec) && strcmp(key, "fullpath")) {
                        int sid;
                        char *filter;

                        snprintf(sid_filter, sizeof(sid_filter), "%s", key);

                        filter = strchr(sid_filter, '.');
                        *filter++ = 0;

                        if (! strcmp(filter, "included")) {
                            sid = atoi(sid_filter);

                            LOGGER_DEBUG("  %d.included=%s", sid, val);
                        } else if (! strcmp(filter, "excluded")) {
                            sid = atoi(sid_filter);

                            LOGGER_DEBUG("  %d.excluded=%s", sid, val);
                        } else {
                            LOGGER_WARN("  %s=%s", key, val);
                        }
                    }

                    str = ConfGetNextPair(cpos, &key, &val);
                }
                ConfCloseFile(cpos);
            }
        }
    }
    ConfSectionListFree(seclist);

    return XS_ERROR;
}


XS_RESULT XS_client_save_conf_ini (XS_client client, const char * config_ini)
{
    return XS_ERROR;
}
