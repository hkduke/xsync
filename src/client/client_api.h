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

#include "../xsync-error.h"

#include "server_opts.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"


typedef struct clientapp_opts
{
    // singleton

    char *startcmd;

    int isdaemon;

    int threads;
    int queues;

    int force_watch;

    char config[XSYNC_PATHFILE_MAXLEN + 1];
} clientapp_opts;


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
    threadpool_t * pool;
    void        ** thread_args;

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

/**
 * public api

extern XS_RESULT XS_client_create (char *config, int config_type, XS_client *outclient);

extern XS_RESULT XS_client_bootstrap (XS_client client);
 */
extern XS_RESULT XS_client_create (clientapp_opts *opts, XS_client *outClient);

extern XS_VOID XS_client_release (XS_client *pClient);

extern int XS_client_lock (XS_client client);

extern void XS_client_unlock (XS_client client);




extern XS_VOID XS_client_clear_all_paths (XS_client client);

typedef XS_RESULT (*traverse_watch_path_callback_t)(XS_watch_path wp, void * data);

extern XS_VOID XS_client_traverse_watch_paths (XS_client client, traverse_watch_path_callback_t traverse_path_cb, void * data);

extern XS_BOOL XS_client_find_path (XS_client client, char * path, XS_watch_path * outPath);

extern XS_BOOL XS_client_add_path (XS_client client, XS_watch_path wp);

extern XS_BOOL XS_client_remove_path (XS_client client, char * path);

extern XS_VOID XS_client_bootstrap (XS_client client);

extern XS_RESULT XS_client_on_inotify_event (XS_client client, struct inotify_event * inevent);

extern XS_RESULT XS_client_read_filter_file (XS_client client, const char * filter_file, int sid, int filter_type);

extern XS_RESULT XS_client_load_conf_xml (XS_client client, const char * config_xml);

extern XS_RESULT XS_client_save_conf_xml (XS_client client, const char * config_xml);

extern XS_RESULT XS_client_load_conf_ini (XS_client client, const char * config_ini);

extern XS_RESULT XS_client_save_conf_ini (XS_client client, const char * config_ini);

#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_API_H_INCLUDED */
