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

#include "../common.h"

#include "server_opts.h"
#include "watch_path.h"


typedef struct perthread_data
{
    int    threadid;

    int    sessions[XSYNC_SERVER_MAXID + 1];
    int    sockfds[XSYNC_SERVER_MAXID + 1];

    byte_t buffer[XSYNC_IO_BUFSIZE];
} perthread_data;


/**
 * xsync_client type
 */
typedef struct xsync_client
{
    EXTENDS_REFOBJECT_TYPE();

    pthread_cond_t  condition;

    char clientid[XSYNC_CLIENTID_MAXLEN + 1];

    /**
     * servers_opts[0].servers
     * total connections in server_conns */
    xsync_server_opts  servers_opts[XSYNC_SERVER_MAXID + 1];

    /**
     * number of threads */
    int threads;

    /**
     * queue size per thread */
    int queues;

    /**
     * if using linux sendfile() to send data */
    int sendfile;

    /**
     * inotify fd */
    int infd;

    /**
     * thread pool for handlers */
    threadpool_t * pool;
    void        ** thread_args;

    /**
     * dhlist for watch path */
    struct list_head list1;
    struct hlist_head hlist[XSYNC_WPATH_HASH_MAXID + 1];

    /**
     * hash table for wd (watch descriptor) -> watch_path */
    xsync_watch_path * wd_table[XSYNC_WPATH_HASH_MAXID + 1];

} xsync_client;


#define XSYNC_GET_WPATH_HASHID(wd)   ((int)((wd) & XSYNC_WPATH_HASH_MAXID))


/**
 * insert wd into wd_table of client
 */
static inline void client_wd_table_insert (xsync_client * client, xsync_watch_path * wp)
{
    int hash = XSYNC_GET_WPATH_HASHID(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_WPATH_HASH_MAXID);

    wp->next = client->wd_table[hash];
    client->wd_table[hash] = wp;
}


static inline xsync_watch_path * client_wd_table_lookup (xsync_client * client, int wd)
{
    xsync_watch_path * wp;

    int hash = XSYNC_GET_WPATH_HASHID(wd);
    assert(wd != -1 && hash >= 0 && hash <= XSYNC_WPATH_HASH_MAXID);

    wp = client->wd_table[hash];
    while (wp) {
        if (wp->watch_wd == wd) {
            return wp;
        }
        wp = wp->next;
    }
    return wp;
}


static inline xsync_watch_path * client_wd_table_remove (xsync_client * client, xsync_watch_path * wp)
{
    xsync_watch_path * lead;
    xsync_watch_path * node;

    int hash = XSYNC_GET_WPATH_HASHID(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_WPATH_HASH_MAXID);

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


__attribute__((unused))
static inline int xsync_client_get_servers (xsync_client * client)
{
    return client->servers_opts->servers;
}


__attribute__((unused))
static inline xsync_server_opts * xsync_client_get_server_by_id (xsync_client * client, int id /* 1 based */)
{
    assert(id > 0 && id <= client->servers_opts->servers);

    return client->servers_opts + id;
}


extern int xsync_client_create (const char * xmlconf, xsync_client ** outClient);

extern void xsync_client_release (xsync_client ** pclient);

extern void xsync_client_clear_all_paths (xsync_client * client);

extern int xsync_client_find_path (xsync_client * client, char * path, xsync_watch_path **outPath);

extern int xsync_client_add_path (xsync_client * client, xsync_watch_path * wp);

extern int xsync_client_remove_path (xsync_client * client, char * path);

extern int xsync_client_waiting_events (xsync_client * client);

extern int xsync_client_lock (xsync_client * client);

extern int xsync_client_unlock (xsync_client * client);

#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
