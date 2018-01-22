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

    int    bufsize;
    byte_t buffer[0];
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
     * buffer size to send data: 8192 (default) */
    uint16_t bufsize;

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
    struct hlist_head hlist[XSYNC_DHLIST_HASHSIZE + 1];
} xsync_client;


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

#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
