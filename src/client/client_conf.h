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

#ifndef CLIENT_CONF_H_INCLUDED
#define CLIENT_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common/common_util.h"

#include "server_opts.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"


#include "../xsync-error.h"
#include "../xsync-config.h"


#define XS_watch_path_hash_get(path)  \
    ((int)(BKDRHash(path) & XSYNC_WATCH_PATH_HASHMAX))

#define XS_watch_id_hash_get(wd)      \
    ((int)((wd) & XSYNC_WATCH_PATH_HASHMAX))

#define XS_client_threadpool_unused_queues(client)    \
    threadpool_unused_queues(client->pool)

#define XS_client_get_server_maxid(client)            \
    (client->servers_opts->sidmax)

#define XS_client_get_server_opts(client, sid)        \
    (client->servers_opts + sid)


typedef struct perthread_data
{
    int    threadid;

    int    sessions[XSYNC_SERVER_MAXID + 1];
    int    sockfds[XSYNC_SERVER_MAXID + 1];

    byte_t buffer[XSYNC_IO_BUFSIZE];
} perthread_data;


/**
 * xs_client_t type
 */
typedef struct xs_client_t
{
    EXTENDS_REFOBJECT_TYPE();

    pthread_cond_t  condition;

    /* 任务数量 */
    int64_t task_counter;

    /* 重置次数 */
    int64_t reset_times;

    /* 客户端唯一 ID */
    char clientid[XSYNC_CLIENTID_MAXLEN + 1];

    /**
     * servers_opts[0].servers
     * total connections in server_conns
     */
    xs_server_opts_t  servers_opts[XSYNC_SERVER_MAXID + 1];

    /** inotify fd */
    int infd;

    /** queue size per thread */
    int queues;

    /** number of threads */
    int threads;

    /** thread pool for handlers */
    threadpool_t *pool;
    void        **thread_args;

    /** hash table for wd (watch descriptor) -> watch_path */
    XS_watch_path wd_table[XSYNC_WATCH_PATH_HASHMAX + 1];

    /**
     * hash map for watch_entry -> watch_entry
     */
    XS_watch_entry entry_map[XSYNC_WATCH_ENTRY_HASHMAX + 1];

    /**
     * dhlist for watch path:
     *    watch_path list and hashmap
     */
    struct list_head wp_dlist;
    struct hlist_head wp_hlist[XSYNC_WATCH_PATH_HASHMAX + 1];

    /* buffer must be in lock */
    char inlock_buffer[XSYNC_IO_BUFSIZE];
} xs_client_t;


__attribute__((used))
static int64_t client_fetch_task_counter (XS_client client)
{
    int64_t old = __sync_add_and_fetch(&client->task_counter, (int64_t) 0);

    if (old == INT64_MAX) {
        int64_t rts = __sync_add_and_fetch(&client->reset_times, (int64_t) 0);
        if (rts == INT64_MAX) {
            rts = 0;
        } else {
            rts++;
        }
        __sync_lock_test_and_set(&client->reset_times, (int64_t) rts);

        __sync_lock_test_and_set(&client->task_counter, (int64_t) 0);
    }

    return __sync_add_and_fetch(&client->task_counter, (int64_t) 1);
}


/**
 * private functions
 */

__attribute__((used))
static XS_VOID client_clear_entry_map (XS_client client)
{
    int i;
    XS_watch_entry first, next;

    LOGGER_TRACE0();

    for (i = 0; i < sizeof(client->entry_map)/sizeof(client->entry_map[0]); i++) {
        first = client->entry_map[i];
        client->entry_map[i] = 0;

        while (first) {
            next = first->next;
            first->next = 0;

            XS_watch_entry_release(&first);

            first = next;
        }
    }
}


__attribute__((used))
static void xs_client_delete (void *pv)
{
    int i, sid, infd;

    XS_client client = (XS_client) pv;

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&client->condition);

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

    client_clear_entry_map(client);

    XS_client_clear_watch_paths(client);

    infd = client->infd;
    if (infd != -1) {
        client->infd = -1;

        LOGGER_DEBUG("inotify closed");
        close(infd);
    }

    LOGGER_TRACE("~xclient=%p", client);

    free(client);
}


/**
 * insert wd into wd_table of client
 */
__attribute__((used))
static inline void client_wd_table_insert (XS_client client, XS_watch_path wp)
{
    int hash = XS_watch_id_hash_get(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

    LOGGER_TRACE0();

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

    LOGGER_TRACE0();

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
static XS_BOOL client_find_watch_entry_inlock (XS_client client, int sid, int wd, const char *filename, XS_watch_entry *outEntry)
{
    XS_watch_entry entry;

    snprintf(client->inlock_buffer, XSYNC_IO_BUFSIZE, "%d:%d/%s", sid, wd, filename);
    client->inlock_buffer[XSYNC_IO_BUFSIZE - 1] = 0;

    int hash = XS_watch_entry_hash_get(client->inlock_buffer);

    entry = client->entry_map[hash];
    while (entry) {
        if (! strcmp(entry->name, client->inlock_buffer)) {
            if (outEntry) {
                *outEntry = entry;
            }

            return XS_TRUE;
        }

        entry = entry->next;
    }

    return XS_FALSE;
}


__attribute__((used))
static XS_BOOL client_add_watch_entry_inlock (XS_client client, XS_watch_entry entry)
{
    XS_watch_entry first;

    assert(entry->next == 0);

    first = client->entry_map[entry->hash];
    while (first) {
        if (first == entry || !strcmp(entry->name, first->name)) {
            return XS_FALSE;
        }
        first = first->next;
    }

    first = client->entry_map[entry->hash];
    entry->next = first;
    client->entry_map[entry->hash] = entry;

    return XS_TRUE;
}


__attribute__((used))
static int client_populate_events_inlock (XS_client client, struct inotify_event * inevent, XS_watch_event events[])
{
    XS_RESULT err;

    int sid;
    int events_maxsid = 0;
    int maxsid = XS_client_get_server_maxid(client);

    for (sid = 1; sid <= maxsid; sid++) {
        events[sid] = 0;

        if (! client_find_watch_entry_inlock(client, sid, inevent->wd, inevent->name, 0)) {
            XS_watch_entry entry;

            err = XS_watch_entry_create(inevent->wd, sid, inevent->name, inevent->len, &entry);

            if (! err) {
                if (client_add_watch_entry_inlock(client, entry)) {
                    XS_watch_event wevent;

                    err = XS_watch_event_create(inevent->mask, client, entry, &wevent);
                    if (! err) {
                        events[sid] = wevent;
                        events_maxsid = sid;
                    } else {
                        XS_watch_entry_release(&entry);
                    }
                } else {
                    XS_watch_entry_release(&entry);
                }
            }
        }
    }

    return events_maxsid;
}


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
