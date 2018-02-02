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

#include "../common/common_incl.h"

#include "server_opts.h"

#include "../xsync-error.h"
#include "../xsync-config.h"


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
     * inotify fd
     */
    int infd;

    /**
     * thread pool for handlers */
    threadpool_t *pool;
    void        **thread_args;

    /**
     * hash table for wd (watch descriptor) -> watch_path
     */
    XS_watch_path wd_table[XSYNC_WATCH_PATH_HASHMAX + 1];

    /**
     * hash map for watch_entry -> watch_entry
     */
    XS_watch_entry * entry_map[XSYNC_WATCH_ENTRY_HASHMAX + 1];

    /**
     * dhlist for watch path:
     *    watch_path list and hashmap
     */
    struct list_head list1;
    struct hlist_head hlist[XSYNC_WATCH_PATH_HASHMAX + 1];
} * XS_client, xs_client_t;


#define XS_watch_path_hash_get(path)   ((int)(BKDRHash(path) & XSYNC_WATCH_PATH_HASHMAX))

#define XS_watch_id_hash_get(wd)       ((int)((wd) & XSYNC_WATCH_PATH_HASHMAX))

#define XS_entry_map_hash_get(path)    ((int)(BKDRHash(path) & XSYNC_WATCH_ENTRY_HASHMAX))

#define XS_client_threadpool_unused_queues(client)    threadpool_unused_queues(client->pool)


#define XS_client_get_server_maxid(client)       (client->servers_opts->sidmax)

#define XS_client_get_server_opts(client, sid)   (client->servers_opts + sid)


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
