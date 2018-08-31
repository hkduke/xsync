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
 * @version: 0.2.1
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-10 18:11:59
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
    struct hlist_head wp_hlist[XSYNC_WATCH_PATH_HASHMAX + 1];

    /* buffer must be in lock */
    char inlock_buffer[XSYNC_BUFSIZE];
} xs_client_t;


/**
 * private functions
 */
extern void xs_client_delete (void *pv);


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


/**
 * insert wd into wd_table of client
 */
__no_warning_unused(static)
inline void client_wd_table_insert (XS_client client, XS_watch_path wp)
{
    int hash = XS_watch_id_hash_get(wp->watch_wd);
    assert(wp->watch_wd != -1 && hash >= 0 && hash <= XSYNC_WATCH_PATH_HASHMAX);

    LOGGER_TRACE0();

    wp->next = client->wd_table[hash];
    client->wd_table[hash] = wp;
}



__no_warning_unused(static)
inline xs_watch_path_t * client_wd_table_lookup (XS_client client, int wd)
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


__no_warning_unused(static)
inline XS_watch_path client_wd_table_remove (XS_client client, XS_watch_path wp)
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


extern int XS_client_prepare_events (XS_client client, const XS_watch_path wp, struct inotify_event *inevent, XS_watch_event events[]);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
