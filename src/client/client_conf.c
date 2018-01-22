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
#include "client.h"

#include "watch_path.h"


#define XSYNC_HASH_PATH(path)   ((int)(BKDRHash(path) & XSYNC_DHLIST_HASHSIZE))


static inline void free_xsync_client (void *pv)
{
    int i, sid;
    xsync_client * client = (xsync_client *) pv;

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
    xsync_client_clear_all_paths(client);

    LOGGER_TRACE("~xclient=%p", client);

    free(client);
}


int xsync_client_create (const char * xmlconf, xsync_client ** outClient)
{
    int i, sid, err;

    xsync_client * client;

    int SERVERS = 2;
    int THREADS = 4;
    int QUEUES = 256;
    uint16_t BUFSIZE = 8192;

    *outClient = 0;

    client = (xsync_client *) mem_alloc(1, sizeof(xsync_client));

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
    for (i = 0; i <= XSYNC_DHLIST_HASHSIZE; i++) {
        INIT_HLIST_HEAD(&client->hlist[i]);
    }

    client->servers_opts->servers = SERVERS;
    client->bufsize = BUFSIZE;
    client->threads = THREADS;
    client->queues = QUEUES;
    client->sendfile = 1;
    client->infd = -1;

    /* populate server_opts from xmlconf */
    for (sid = 1; sid <= SERVERS; sid++) {
        xsync_server_opts * server_opts = xsync_client_get_server_by_id(client, sid);

        server_opts_init(server_opts);

        // TODO:
    }

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data * perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data) + sizeof(char) * BUFSIZE);

        perdata->sockfds[0] = SERVERS;
        perdata->threadid = i + 1;
        perdata->bufsize = BUFSIZE;

        // TODO: socket
        for (sid = 1; sid <= SERVERS; sid++) {
            xsync_server_opts * server = xsync_client_get_server_by_id(client, sid);

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
    for (i = 0; i < 10; i++) {
        xsync_watch_path * wp = 0;

        char pathfile[20];

        snprintf(pathfile, sizeof(pathfile), "/tmp/%d", i);

        if (watch_path_create(pathfile, &wp) == 0) {
            if (! xsync_client_add_path(client, wp)) {
                watch_path_release(&wp);
            }
        } else {
            free_xsync_client((void*) client);
            return (-1);
        }
    }

    /**
     * output xsync_client object */
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


void xsync_client_release (xsync_client ** inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, free_xsync_client);
}


void xsync_client_clear_all_paths (xsync_client * client)
{
    struct list_head *list, *node;

    LOGGER_TRACE0();

    list_for_each_safe(list, node, &client->list1) {
        struct xsync_watch_path * wp = list_entry(list, struct xsync_watch_path, i_list);

        hlist_del(&wp->i_hash);
        list_del(list);

        watch_path_release(&wp);
    }
}


int xsync_client_find_path (xsync_client * client, char * path, xsync_watch_path **outwp)
{
    struct hlist_node * hp;

    // 计算 hash
    int hash = XSYNC_HASH_PATH(path);

    hlist_for_each(hp, &client->hlist[hash]) {
        struct xsync_watch_path * wp = hlist_entry(hp, struct xsync_watch_path, i_hash);

        if (! strcmp(wp->fullpath, path)) {
            LOGGER_TRACE("xpath=%p (%s)", wp, wp->fullpath);

            if (outwp) {
                RefObjectRetain((void**) &wp);

                *outwp = wp;
            }

            return 1;
        }
    }

    return 0;
}


int xsync_client_add_path (xsync_client * client, xsync_watch_path * wp)
{
    if (! xsync_client_find_path(client, wp->fullpath, 0)) {
        int hash = XSYNC_HASH_PATH(wp->fullpath);

        // 串入长串
        list_add(&wp->i_list, &client->list1);

        // 串入HASH短串
        hlist_add_head(&wp->i_hash, &client->hlist[hash]);

        // 成功
        LOGGER_TRACE("xpath=%p (%s)", wp, wp->fullpath);
        return 1;
    }

    return 0;
}


int xsync_client_remove_path (xsync_client * client, char * path)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int hash = XSYNC_HASH_PATH(path);

    hlist_for_each_safe(hp, hn, &client->hlist[hash]) {
        struct xsync_watch_path * wp = hlist_entry(hp, struct xsync_watch_path, i_hash);

        if (! strcmp(wp->fullpath, path)) {
            hlist_del(hp);
            list_del(&wp->i_list);

            LOGGER_TRACE("xpath=%p (%s)", wp, wp->fullpath);
            watch_path_release(&wp);
            return 1;
        }
    }

    return 0;
}

