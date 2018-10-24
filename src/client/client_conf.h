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
 * @version: 0.2.7
 *
 * @create: 2018-01-25
 *
 * @update: 2018-10-23 12:17:24
 */

#ifndef CLIENT_CONF_H_INCLUDED
#define CLIENT_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "server_conn.h"
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

    /* watch 全路径或 config.xml 全路径: 最长 FILENAME_MAXLEN 字符 */
    int from_watch;
    char watch_config[FILENAME_MAXLEN + 1];

    /**
     * servers_opts[0].servers
     * total connections in server_conns
     */
    xs_server_opts  servers_opts[XSYNC_SERVER_MAXID + 1];

    /* 是(1)否(0)使用 kafka */
    int kafka;

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

    /* 是(1)否(0)重启监视(当配置更改时有必要重启监视) */
    volatile int inotify_reload;

    /* 刷新时间: 默认 0 */
    volatile time_t ready_time;

    /* 总的刷新计数器 */
    volatile int64_t sweep_count;

    /* 每次刷新的文件数 */
    int64_t sweep_files;

    /* lua context */
    lua_context luactx;

    /* 存放监视 wd 对应的 pathid. 最多监视 XSYNC_WATCH_PATHID_MAX=256 个 pathid 目录 */
#ifdef XSYNC_USE_STATIC_PATHID_TABLE
    char *wd_pathid_table[XSYNC_WATCH_PATHID_MAX];
#else
    red_black_tree_t  wd_pathid_rbtree;
#endif

    /* 当前的监视事件树: 用于缓存正在处理的事件, 防止事件被重复处理 */
    red_black_tree_t  event_rbtree;
    pthread_mutex_t rbtree_lock;

    /* application home dir, for instance: '/opt/xclient/sbin/' */
    int apphome_len;
    char apphome[FILENAME_MAXLEN + FILENAME_MAXLEN + 2];

    /* buffer with size >= 8192 and >= (PATH_MAX x 2) */
    char buffer[XSYNC_BUFSIZE];
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


#ifndef XSYNC_USE_STATIC_PATHID_TABLE

struct wd_pathid_t
{
    int wd;
    char len;
    char pathid[0];
};


__no_warning_unused(static)
int wd_pathid_rbtree_cmp (void *newObject, void *nodeObject)
{
    int wdNew = ((struct wd_pathid_t *) newObject)->wd;
    int wdOld = ((struct wd_pathid_t *) nodeObject)->wd;

    return (wdNew == wdOld ? 0 : (wdNew > wdOld ? 1 : -1));
}


__no_warning_unused(static)
void wd_pathid_rbtree_op (void *object, void *param)
{
}

#endif


/**
 * private functions
 */
extern void xs_client_delete (void *pv);

extern int xs_client_find_wpath_inlock (XS_client client, const char *wpath, char *pathroute, ssize_t pathsize,
    char *clientid_buf, int clientid_cb, char *pathid_buf, int pathid_cb, char *route_buf, int route_cb);

#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_CONF_H_INCLUDED */
