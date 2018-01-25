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

#ifndef CLIENT_API_H_INCLUDED
#define CLIENT_API_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "server_opts.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"


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

    char clientid[XSYNC_CLIENTID_MAXLEN + 1];

    /**
     * servers_opts[0].servers
     * total connections in server_conns
     */
    xs_server_opts_t  servers_opts[XSYNC_SERVER_MAXID + 1];

    /**
     * number of threads
     */
    int threads;

    /**
     * queue size per thread
     */
    int queues;

    /**
     * if using linux sendfile() to send data
     */
    int sendfile;

    /**
     * inotify fd
     */
    int infd;

    /**
     * thread pool for handlers */
    threadpool_t * pool;
    void        ** thread_args;

    /**
     * hash table for wd (watch descriptor) -> watch_path
     */
    XS_watch_path wd_table[XSYNC_PATH_HASH_MAXID + 1];

    /**
     * hash map for watch_entry -> watch_entry
     */
    XS_watch_entry * entry_map[XSYNC_ENTRY_HASH_MAXID + 1];

    /**
     * dhlist for watch path:
     *    watch_path list and hashmap
     */
    struct list_head list1;
    struct hlist_head hlist[XSYNC_PATH_HASH_MAXID + 1];
} * XS_client, xs_client_t;


#define XSYNC_PATH_HASH_ID_GET(wd)   ((int)((wd) & XSYNC_PATH_HASH_MAXID))


/**
 * insert wd into wd_table of client
 */
__attribute__((used))
static inline void client_wd_table_insert (XS_client client, XS_watch_path wp)
{
    int hash = XSYNC_PATH_HASH_ID_GET(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_PATH_HASH_MAXID);

    wp->next = client->wd_table[hash];
    client->wd_table[hash] = wp;
}


__attribute__((used))
static inline xs_watch_path_t * client_wd_table_lookup (XS_client client, int wd)
{
    xs_watch_path_t * wp;

    int hash = XSYNC_PATH_HASH_ID_GET(wd);
    assert(wd != -1 && hash >= 0 && hash <= XSYNC_PATH_HASH_MAXID);

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

    int hash = XSYNC_PATH_HASH_ID_GET(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_PATH_HASH_MAXID);

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
static inline int client_get_servers (XS_client client)
{
    return client->servers_opts->servers;
}


__attribute__((used))
static inline int client_threadpool_unused_queues (XS_client client)
{
    return threadpool_unused_queues(client->pool);
}


__attribute__((used))
static inline xs_server_opts_t * client_get_server_by_id (XS_client client, int id /* 1 based */)
{
    assert(id > 0 && id <= client->servers_opts->servers);
    return client->servers_opts + id;
}


extern int XS_client_create (const char * xmlconf, XS_client * outClient);

extern void XS_client_release (XS_client * pclient);

extern void XS_client_clear_all_paths (XS_client client);

extern int XS_client_find_path (XS_client client, char * path, XS_watch_path * outPath);

extern int XS_client_add_path (XS_client client, XS_watch_path wp);

extern int XS_client_remove_path (XS_client client, char * path);

extern int XS_client_listening_events (XS_client client);

extern int XS_client_lock (XS_client client);

extern int XS_client_unlock (XS_client client);

extern int XS_client_on_inotify_event (XS_client client, struct inotify_event * inevent);

#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_API_H_INCLUDED */
