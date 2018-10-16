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
 * @version: 0.1.5
 *
 * @create: 2018-01-25
 *
 * @update: 2018-10-15 14:19:48
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

#include "perthread_data.h"

#include "../common/red_black_tree.h"


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
    volatile int size_paths;
    char *paths;

    /* lua context */
    lua_context luactx;

    /**
     * tree for caching all inotify events
     */
    red_black_tree_t  event_rbtree;
    pthread_mutex_t rbtree_lock;

    /** DEL??
     * wpath_hmap: a hash map for XS_watch_path
     * wpath_lock: 访问 wpath_hmap 的锁
     */
    struct hlist_head wpath_hmap[XSYNC_HASHMAP_MAX_LEN + 1];
    thread_lock_t wpath_lock;

    

    /* buffer */
    char buffer[XSYNC_BUFSIZE];

    /* application home dir, for instance: '/opt/xclient/sbin/' */
    int apphome_len;
    char apphome[0];
} xs_client_t;


#define event_rbtree_lock()  \
    do { \
        int __ptlock_errno = pthread_mutex_lock(&client->rbtree_lock); \
        if (__ptlock_errno != 0) { \
            if (__ptlock_errno == EBUSY) { \
                LOGGER_FATAL("pthread_mutex_lock fail: EBUSY"); \
            } else if (__ptlock_errno == EINVAL) { \
                LOGGER_FATAL("pthread_mutex_lock fail: EINVAL"); \
            } else if (__ptlock_errno == EAGAIN) { \
                LOGGER_FATAL("pthread_mutex_lock fail: EAGAIN"); \
            } else if (__ptlock_errno == EDEADLK) { \
                LOGGER_FATAL("pthread_mutex_lock fail: EDEADLK"); \
            } else if (__ptlock_errno == EPERM) { \
                LOGGER_FATAL("pthread_mutex_lock fail: EPERM"); \
            } else { \
                LOGGER_FATAL("pthread_mutex_lock fail(%d): unknown errno", __ptlock_errno); \
            } \
            exit(__ptlock_errno); \
        } \
    } while(0)


#define event_rbtree_unlock()  pthread_mutex_unlock(&client->rbtree_lock)

#define get_watch_root_len(client)  (client->offs_path_filter - client->offs_watch_root - 1)

#define watch_root_path(client)  (client->paths + client->offs_watch_root)


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

extern int XS_client_prepare_watch_events (XS_client client, struct inotify_event *inevent, XS_watch_event events[XSYNC_SERVER_MAXID + 1]);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
