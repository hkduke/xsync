/***********************************************************************
* COPYRIGHT (C) 2018 PEPSTACK, PEPSTACK.COM
*
* THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES
* ARISING FROM THE USE OF THIS SOFTWARE.
*
* PERMISSION IS GRANTED TO ANYONE TO USE THIS SOFTWARE FOR ANY PURPOSE,
* INCLUDING COMMERCIAL APPLICATIONS, AND TO ALTER IT AND REDISTRIBUTE IT
* FREELY, SUBJECT TO THE FOLLOWING RESTRICTIONS:
*
*  THE ORIGIN OF THIS SOFTWARE MUST NOT BE MISREPRESENTED; YOU MUST NOT
*  CLAIM THAT YOU WROTE THE ORIGINAL SOFTWARE. IF YOU USE THIS SOFTWARE
*  IN A PRODUCT, AN ACKNOWLEDGMENT IN THE PRODUCT DOCUMENTATION WOULD
*  BE APPRECIATED BUT IS NOT REQUIRED.
*
*  ALTERED SOURCE VERSIONS MUST BE PLAINLY MARKED AS SUCH, AND MUST NOT
*  BE MISREPRESENTED AS BEING THE ORIGINAL SOFTWARE.
*
*  THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
***********************************************************************/

/**
 * @file: client_conf.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.8
 *
 * @create: 2018-01-25
 *
 * @update: 2018-09-29 16:56:29
 */

#ifndef CLIENT_CONF_H_INCLUDED
#define CLIENT_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "server_conn.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/red_black_tree.h"

#include "../xsync-error.h"
#include "../xsync-config.h"


#define XS_client_threadpool_unused_queues(client)    \
    threadpool_unused_queues(client->pool)

#define XS_client_get_server_maxid(client)            \
    (client->servers_opts->sidmax)


/**
 * xs_client_t singleton
 */
typedef struct xs_client_t
{
    EXTENDS_REFOBJECT_TYPE();

    pthread_cond_t  condition;

    /* 任务数量 */
    ref_counter_t task_counter;

    /* 客户端唯一 ID */
    char clientid[XSYNC_CLIENTID_MAXLEN + 1];

    /**
     * servers_opts[0].servers
     * total connections in server_conns
     */
    xs_server_opts  servers_opts[XSYNC_SERVER_MAXID + 1];

    /**
     * interval in seconds
     */
    int interval_seconds;

    /** queue size per thread */
    int queues;

    /** number of threads */
    int threads;

    /** thread pool for handlers */
    threadpool_t *pool;
    void        **thread_args;

    /* 是(1)否(0)重启监控(当配置更改时有必要重启监控) */
    volatile int inotify_reload;

    /* 路径组合配置 */
    int offs_watch_root;
    int offs_path_filter;
    int offs_event_task;
    volatile int size_paths;
    char *paths;

    char file_filter_buf[XSYNC_BUFSIZE];
    char path_filter_buf[XSYNC_BUFSIZE];

    /**
     * tree for caching all inotify events
     */
    red_black_tree_t  event_rbtree;
    thread_lock_t rbtree_lock;

    /**
     * wpath_hmap: a hash map for XS_watch_path
     * wpath_lock: 访问 wpath_hmap 的锁
     */
    struct hlist_head wpath_hmap[XSYNC_HASHMAP_MAX_LEN + 1];
    thread_lock_t wpath_lock;






    /** DEL??
     * hash map for watch_entry -> watch_entry
     */
    XS_watch_entry entry_map[XSYNC_WATCH_ENTRY_HASHMAX + 1];

    /** hash table for wd (watch descriptor) -> watch_path */
    XS_watch_path wd_table[XSYNC_HASHMAP_MAX_LEN + 1];



    /* buffer must be in lock */
    char inlock_buffer[XSYNC_BUFSIZE];
} xs_client_t;


#define get_watch_root_len(client)  (client->offs_path_filter - client->offs_watch_root - 1)

#define get_path_filter_len(client)  (client->offs_event_task - client->offs_path_filter - 1)

#define get_event_task_len(client)  (client->size_paths - client->offs_event_task - 1)

#define watch_root_path(client)  (client->paths + client->offs_watch_root)

#define path_filter_path(client)  (client->paths + client->offs_path_filter)

#define event_task_path(client)  (client->paths + client->offs_event_task)


__no_warning_unused(static)
inline xs_server_opts* XS_client_get_server_opts (XS_client client, int sid)
{
    if (sid > 0 && sid <= XSYNC_SERVER_MAXID) {
        return (client->servers_opts + sid);
    } else {
        return 0;
    }
}


__no_warning_unused(static)
inline int client_is_inotify_reload (struct xs_client_t *client)
{
    return __interlock_get(&client->inotify_reload);
}


__no_warning_unused(static)
inline void client_set_inotify_reload (struct xs_client_t *client, int reload)
{
    __interlock_set(&client->inotify_reload, reload);
}


__no_warning_unused(static)
int event_rbtree_cmp(void *newObject, void *nodeObject)
{
    return watch_event_compare((XS_watch_event) newObject, (XS_watch_event) nodeObject);
}


__no_warning_unused(static)
void event_rbtree_op(void *object, void *param)
{
}


/**
 * private functions
 */
extern void xs_client_delete (void *pv);


/**
 * level = 0 : 必须是目录符号链接
 * level = 1, 2, ... : 可以是目录符号链接, 也可以是物理目录
 */
extern int lscb_init_watch_path (const char * path, int pathlen, struct mydirent *myent, void *arg1, void *arg2);


__no_warning_unused(static)
XS_VOID client_clear_entry_map (XS_client client)
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


__no_warning_unused(static)
XS_BOOL client_find_watch_entry_inlock (XS_client client, int sid, int wd, const char *filename, XS_watch_entry *outEntry)
{
    XS_watch_entry entry;

    char nameid[XSYNC_BUFSIZE];
    snprintf(nameid, XSYNC_BUFSIZE, "%d:%d/%s", sid, wd, filename);
    nameid[XSYNC_BUFSIZE - 1] = 0;

    int hash = xs_watch_entry_hash(nameid);

    entry = client->entry_map[hash];
    while (entry) {
        if (! strcmp(entry->namebuf, nameid)) {
            *outEntry = entry;
            return XS_TRUE;
        }

        entry = entry->next;
    }

    return XS_FALSE;
}


__no_warning_unused(static)
XS_BOOL client_add_watch_entry_inlock (XS_client client, XS_watch_entry entry)
{
    XS_watch_entry first;

    assert(entry->next == 0);

    first = client->entry_map[entry->hash];
    while (first) {
        if ( first == entry || ! strcmp(xs_entry_nameid(entry), xs_entry_nameid(first)) ) {
            LOGGER_TRACE("entry(%s) exists in map", xs_entry_nameid(entry));
            return XS_FALSE;
        }

        first = first->next;
    }

    first = client->entry_map[entry->hash];
    entry->next = first;
    client->entry_map[entry->hash] = entry;

    return XS_TRUE;
}


__no_warning_unused(static)
XS_BOOL client_remove_watch_entry_inlock (XS_client client, XS_watch_entry entry)
{
    XS_watch_entry first;

    first = client->entry_map[entry->hash];
    if (! first) {
        LOGGER_ERROR("application error: entry not found. (%s)", xs_entry_nameid(entry));
        return XS_TRUE;
    }

    if (first == entry) {
        client->entry_map[entry->hash] = first->next;
        first->next = 0;

        XS_watch_entry_release(&entry);
        return XS_TRUE;
    }

    while (first->next && first->next != entry) {
        first = first->next;
    }

    if (first->next == entry) {
        first->next = entry->next;
        entry->next = 0;

        XS_watch_entry_release(&entry);
        return XS_TRUE;
    }

    LOGGER_ERROR("application error: entry not found. (%s)", xs_entry_nameid(entry));
    return XS_TRUE;
}


extern int XS_client_prepare_watch_events (XS_client client, struct inotify_event *inevent, XS_watch_event events[XSYNC_SERVER_MAXID + 1]);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
