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
 * @file: client_api.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.4
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-13 15:53:46
 */

/**
 *
 * client_init_from_watch
 * 从 watch 目录初始化 client. watch 目录遵循下面的规则:
 *
 *  xsync-client-home/                           (客户端主目录)
 *            |
 *            +---- CLIENTID  (客户端唯一标识 ID, 确保名称符合文件目录名规则, 长度不大于 36 字符)
 *            |
 *            +---- LICENCE                      (客户端版权文件)
 *            |
 *            +---- bin/
 *            |       |
 *            |       +---- xsync-client-$version (客户端程序)
 *            |
 *            |
 *            +---- conf/                        (客户端配置文件目录)
 *            |       |
 *            |       +---- xsync-client.conf    (客户端配置文件)
 *            |       |
 *            |       +---- log4crc              (客户端日志配置文件)
 *            |
 *            |
 *            +---- sid/                         (服务器配置目录)
 *            |       |
 *            |       +---- pepstack-server/           (服务器名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |          ...
 *            |       |
 *            |       +---- giant-logserver/           (服务器名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |
 *            |       +---- ...
 *            |
 *            |
 *            |
 *            +---- watch/             (可选, 客户端 watch 目录 - 符号链接)
 *                    |
 *                    +---- pathlinkA -> /path/to/A/
 *                    |
 *                    +---- pathlinkB -> /path/to/B/
 *                    |
 *                    +---- pathLinkC -> /path/to/C/
 *                    |        ...
 *                    +---- pathLinkN -> /path/to/N/
 *                    |
 *                    +---- 1   (服务器 sid=1, 内容仅1行 = "host:port#magic";
 *                    |          或者为链接文件 -> ../sid/nameserver/SERVERID)
 *                    |
 *                    +---- 2   (服务器 sid=2, 内容仅1行 = "host:port#magic";
 *                    |          或者为链接文件 -> ../sid/?/SERVERID)
 *                    |    ...
 *                    |
 *                    +---- i   (服务器 sid=i. 服务器 sid顺序编制, 不可以跳跃.
 *                    |          最大服务号不可以超过255. 参考手册)
 *                    |
 *                    |     (以下文件为可选)
 *                    |
 *                    +---- path-filter.sh  (目录过滤脚本: 用户实现)
 *                    |
 *                    +---- file-filter.sh  (文件过滤脚本: 用户实现)
 *
 *
 */
#include "client_api.h"

#include "client_conf.h"

#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/common_util.h"

// 刷新超时秒
#define SWEEP_INTERVAL_SECONDS  3


__no_warning_unused(static)
void event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    if (task->flags == 10) {
        XS_watch_event event = (XS_watch_event) task->argument;

        task->argument = 0;
        task->flags = 0;

        LOGGER_DEBUG("TODO:(thread-%d: task=%lld) sync file ...", thread_ctx->id, (long long) event->taskid);

        /*
        LOGGER_TRACE("thread-%d: start task=%lld (entryid=%llu, offset=%llu)",
            thread_ctx->id, (long long) event->taskid,
            (long long) event->entry->entryid,
            (long long) event->entry->offset);

        ssize_t sendbytes = XS_watch_event_sync_file(event, (perthread_data *) thread_ctx->thread_arg);

        LOGGER_TRACE("thread-%d: end task=%lld (entryid=%llu,  offset=%llu, send=%lu)",
            thread_ctx->id, (long long) event->taskid,
            (long long) event->entry->entryid,
            (long long) event->entry->offset,
            (long unsigned int) sendbytes);
        */

        /**
         * MUST release event after use
         */
        XS_client client = XS_client_retain(&event->client);
        {
            threadlock_lock(&client->event_map_lock);
            {
                XS_watch_event_release(&event);
            }
            threadlock_unlock(&client->event_map_lock);
        }
        XS_client_release(&client);
    } else if (task->flags == 90) {


    } else {
        LOGGER_ERROR("unknown event task flags(=%d)", task->flags);
    }
}


__no_warning_unused(static)
XS_RESULT client_add_inotify_event (XS_client client, struct inotify_event * inevent)
{
    int num = XS_client_threadpool_unused_queues(client);
    if (num < 1) {
        LOGGER_WARN("threadpool queues is full");
        return XS_E_POOL;
    }
    LOGGER_TRACE("threadpool_unused_queues=%d", num);

    if (threadlock_trylock(&client->event_map_lock) == 0) {
        int sid, err;

        XS_watch_event events[XSYNC_SERVER_MAXID + 1] = { 0 };

        num = XS_client_prepare_watch_events(client, inevent, events);

        for (sid = 0; sid < num; sid++) {
            XS_watch_event event = events[sid];

            if (event) {
                err = threadpool_add(client->pool, event_task, (void*) event, 10);

                if (err) {
                    LOGGER_ERROR("threadpool_add task(=%lld) error(%d): %s", (long long) event->taskid, err, threadpool_error_messages[-err]);

                    // 增加到线程池失败, 必须释放 (从 watch_map 中删除自身)
                    XS_watch_event_release(&event);
                } else {
                    // 增加到线程池成功
                    LOGGER_DEBUG("threadpool_add task(=%lld) success", (long long) event->taskid);
                }
            }
        }

        threadlock_unlock(&client->event_map_lock);

        return XS_SUCCESS;
    }

    return XS_ERROR;
}


/**
 * 调用外部脚本 path-filter.sh 过滤文件路径目录
 *
 *  (外部脚本返回值):
 *    100 - 接受
 *   -100 - 拒绝
 *     -1 - 重载
 *
 * 返回值:
 *  >  0 : wd 递归遍历目录 (listdir)
 *  =  0 : 不再递归遍历目录
 *  = -1 : 重启监视
 */
#define FILTER_WPATH_ACCEPT      100
#define FILTER_WPATH_REJECT    (-100)
#define FILTER_WPATH_RELOAD      (-1)
#define FILTER_WPATH_UNUSED        0


__no_warning_unused(static)
int filter_watch_path (XS_client client, const char *path, char pathbuf[PATH_MAX])
{
    int len, ok, wd;
    char *abspath = realpath(path, pathbuf);

    if (! abspath) {
        LOGGER_ERROR("realpath error(%d): %s (%s)", errno, strerror(errno), path);
        return 0;
    }

    // 目录以 '/' 结尾
    len = strlen(abspath);
    abspath[ len++ ] = '/';
    abspath[ len ] = '\0';

    // 调用外部脚本, 过滤监控路径
    ok = 0;
    do {
        char path_filter_sh[XSYNC_PATH_MAXSIZE + XSYNC_PATH_MAXSIZE];

        if (client->is_config_xml) {
            // 取得外部脚本文件全路径名
            LOGGER_ERROR("TODO: get filter shell file if using config xml");


        } else {
            len = snprintf(path_filter_sh, XSYNC_PATH_MAXSIZE, "%spath-filter.sh", client->watch_parent);
            if (len > 0) {
                path_filter_sh[len] = 0;

                if ( access(path_filter_sh, F_OK|R_OK|X_OK) == 0 ) {
                    len = snprintf(path_filter_sh, sizeof(path_filter_sh), "%spath-filter.sh '%s'",
                        client->watch_parent, abspath);

                    if (len > 0) {
                        path_filter_sh[len] = 0;
                        ok = 1;
                    } else {
                        LOGGER_ERROR("path is too long: %spath-filter.sh '%s'", client->watch_parent, abspath);
                    }
                } else {
                    LOGGER_ERROR("path filter cannot access: %s (%s)", strerror(errno), path_filter_sh);
                }
            } else {
                LOGGER_ERROR("path filter is too long: %spath-filter.sh", client->watch_parent);
            }
        }

        if (ok) {
            // 发现有效的脚本, 执行过滤命令
            char outbuf[XSYNC_PATH_MAXSIZE];
            LOGGER_INFO("command: %s", path_filter_sh);
            const char *result = cmd_system(path_filter_sh, outbuf, sizeof(outbuf));

            ok = 0;

            if (result) {
                ok = atoi(result);
            }

            if (ok == FILTER_WPATH_ACCEPT) {
                LOGGER_INFO("result(=%d): ACCEPT", ok);
            } else if (ok == FILTER_WPATH_REJECT) {
                LOGGER_INFO("result(=%d): REJECT", ok);
            } else if (ok == FILTER_WPATH_RELOAD) {
                LOGGER_INFO("result(=%d): RELOAD", ok);
            } else {
                LOGGER_ERROR("result(=%d): UNKNOWN", ok);
                return 0;
            }
        }
    } while (0);

    if (! ok || ok == FILTER_WPATH_ACCEPT) {
        // 不使用过滤脚本或接受目录
        wd = inotifytools_wd_from_filename(abspath);

        if (wd < 1) {
            // 接受的监视目录不存在, 重启监视
            LOGGER_ERROR("path not in watch: %s", abspath);
            return (-1);
        }

        return wd;
    }

    if (ok == FILTER_WPATH_REJECT) {
        // 拒绝目录
        wd = inotifytools_wd_from_filename(abspath);

        if (wd > 0) {
            // 拒绝的监视目录存在, 删除监视
            if (inotifytools_remove_watch_by_wd(wd)) {
                // 移除监视成功, 不再递归
                LOGGER_INFO("remove watch path(wd=%d) ok: %s", wd, abspath);
                return 0;
            } else {
                // 移除监视失败, 重启监控
                LOGGER_ERROR("remove watch path(wd=%d) failed: %s", wd, abspath);
                return (-1);
            }
        }

        // 拒绝目录已经被移除监视, 不再递归
        LOGGER_DEBUG("reject watch path: %s", abspath);
        return 0;
    }

    LOGGER_WARN("reload inotify watch(%d)", ok);
    return (-1);
}


__no_warning_unused(static)
int filter_watch_file (XS_client client, int wd, const char * pathfile, char pathbuf[PATH_MAX])
{
    int len, ok;

    const char *abspathfile = (pathbuf? realpath(pathfile, pathbuf) : pathfile);
    if (! abspathfile) {
        LOGGER_ERROR("realpath error(%d): %s (%s)", errno, strerror(errno), pathfile);
        return 0;
    }

    // 调用外部脚本, 过滤监控路径
    ok = 0;
    do {
        char path_filter_sh[XSYNC_PATH_MAXSIZE + XSYNC_PATH_MAXSIZE];

        if (client->is_config_xml) {
            // 取得外部脚本文件全路径名
            LOGGER_ERROR("TODO: get filter shell file if using config xml");


        } else {
            len = snprintf(path_filter_sh, XSYNC_PATH_MAXSIZE, "%sfile-filter.sh", client->watch_parent);
            if (len > 0) {
                path_filter_sh[len] = 0;

                if ( access(path_filter_sh, F_OK|R_OK|X_OK) == 0 ) {
                    len = snprintf(path_filter_sh, sizeof(path_filter_sh), "%sfile-filter.sh '%s'",
                        client->watch_parent, abspathfile);

                    if (len > 0) {
                        path_filter_sh[len] = 0;
                        ok = 1;
                    } else {
                        LOGGER_ERROR("path is too long: %sfile-filter.sh '%s'", client->watch_parent, abspathfile);
                    }
                } else {
                    LOGGER_ERROR("path filter cannot access: %s (%s)", strerror(errno), path_filter_sh);
                }
            } else {
                LOGGER_ERROR("path filter is too long: %sfile-filter.sh", client->watch_parent);
            }
        }

        if (ok) {
            // 发现有效的脚本, 执行过滤命令
            char outbuf[XSYNC_PATH_MAXSIZE];
            LOGGER_INFO("command: %s", path_filter_sh);
            const char *result = cmd_system(path_filter_sh, outbuf, sizeof(outbuf));

            ok = 0;

            if (result) {
                ok = atoi(result);
            }

            if (ok == FILTER_WPATH_ACCEPT) {
                LOGGER_INFO("result(=%d): ACCEPT", ok);
            } else if (ok == FILTER_WPATH_REJECT) {
                LOGGER_INFO("result(=%d): REJECT", ok);
            } else if (ok == FILTER_WPATH_RELOAD) {
                LOGGER_INFO("result(=%d): RELOAD", ok);
            } else {
                LOGGER_ERROR("result(=%d): UNKNOWN", ok);
                return 0;
            }
        }
    } while (0);

    if (! ok || ok == FILTER_WPATH_ACCEPT) {
        LOGGER_TRACE("accept file: %s", pathfile);
        return 1;
    }

    if (ok == FILTER_WPATH_REJECT) {
        LOGGER_TRACE("reject file: %s", pathfile);
        return 0;
    }

    LOGGER_WARN("unexpected(=%d)", ok);
    return 0;
}


__no_warning_unused(static)
int lscb_sweep_watch_path (const char * path, int pathlen, struct dirent *ent, void *arg1, void *arg2)
{
    int wd;
    char pathbuf[PATH_MAX];

    if (path[pathlen - 1] == '/') {

        if (ent->d_type == DT_DIR) {
            wd = filter_watch_path( (XS_client) arg1, path, pathbuf);

            if (wd > 0) {
                LOGGER_TRACE("sweep path(wd=%d): %s", wd, pathbuf);
                listdir(path, pathbuf, sizeof(pathbuf), (listdir_callback_t) lscb_sweep_watch_path, arg1, int_cast_to_pv(wd));
            } else if (wd == -1) {
                client_set_inotify_reload((XS_client) arg1, 1);
                return 0;
            }
        } else if (ent->d_type == DT_LNK) {
            wd = filter_watch_path( (XS_client) arg1, path, pathbuf);

            if (wd > 0) {
                if (arg2) {
                    // 当前子目录不支持符号链接
                    LOGGER_ERROR("child dir is link: %s -> %s", path, pathbuf);
                } else {
                    listdir(path, pathbuf, sizeof(pathbuf), (listdir_callback_t) lscb_sweep_watch_path, arg1, int_cast_to_pv(wd));
                }
            } else if (wd == -1) {
                client_set_inotify_reload((XS_client) arg1, 1);
                return 0;
            }
        } else {
            LOGGER_WARN("ignored dir type(=%d): %s", ent->d_type, path);
        }

    } else if (ent->d_type == DT_REG) {

        wd = pv_cast_to_int(arg2);

        LOGGER_TRACE("sweep file(wd=%d): %s", wd, path);

        if (filter_watch_file((XS_client) arg1, wd, path, pathbuf) > 0) {
            // 添加任务到线程池
            struct inotify_event_equivalent event = {0};

            client_add_inotify_event((XS_client) arg1, makeup_inotify_event(&event, wd, IN_CLOSE_NOWRITE, 0, path, pathlen));
        }

    } else if (ent->d_type == DT_LNK) {

        char * abspath = realpath(path, pathbuf);

        if (abspath) {
            LOGGER_WARN("ignored file link(=%d): %s -> %s", ent->d_type, path, abspath);
        } else {
            LOGGER_ERROR("realpath error(%d): %s (%s)", errno, strerror(errno), path);
        }

    }

    return 1;
}


/**
 * 刷新目录树工作者函数: 刷新超时尽量短 ( < 1s)
 */
__no_warning_unused(static)
void sweep_worker (void *arg)
{
    int err;

    struct timeval now;
    struct timespec timo;

    XS_client client = (XS_client) arg;

    long timeout_ms = 500;

    char pathbuf[PATH_MAX];

    for (;;) {
        err = XS_client_lock(client, 0);

        if (err == 0) {

            timespec_reset_timeout(&timo, &now, timeout_ms);

            /* Wait on condition variable, check for spurious wakeups.
               When returning from pthread_cond_wait(), we own the lock. */
            err = XS_client_wait_condition(client, &timo);

            if (err == 0) {
                // 在此刷新目录树
                err = listdir(client->watch_parent, pathbuf, sizeof(pathbuf), (listdir_callback_t) lscb_sweep_watch_path, (void*) client, 0);
            }

            /* unlock immediatedly so as to accept other events to be handled */
            XS_client_unlock(client);
        }
    }

    LOGGER_FATAL("thread exit unexpected.");

    pthread_exit (0);
}


__no_warning_unused(static)
void inotifytools_restart(XS_client client)
{
    int err;

    LOGGER_DEBUG("inotifytools_restart");

    inotifytools_cleanup();

	if ( ! inotifytools_initialize()) {
        LOGGER_ERROR("inotifytools_initialize(): %s", strerror(inotifytools_error()));
        exit(XS_ERROR);
    }

    if (client->is_config_xml) {
        // 从 XML 配置文件初始化客户端
        LOGGER_ERROR("TODO: XS_client_conf_load_xml");
        err = XS_client_conf_load_xml(client, client->watch_parent);
    } else {
        // 从监控目录初始化客户端
        err = XS_client_conf_from_watch(client, client->watch_parent);
    }

    if (err) {
        LOGGER_ERROR("inotifytools_restart failed");
        exit(XS_ERROR);
    }
}


/***********************************************************************
 *
 * XS_client application api
 *
 **********************************************************************/
extern XS_RESULT XS_client_create (xs_appopts_t *opts, XS_client *outClient)
{
    XS_client client;

    int i, err;

    int SERVERS;
    int THREADS = opts->threads;
    int QUEUES = opts->queues;

    *outClient = 0;

    client = (XS_client) mem_alloc(1, sizeof(xs_client_t));
    assert(client->thread_args == 0);

    __interlock_release(&client->task_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&client->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));
        free((void*) client);
        return XS_ERROR;
    }

    /* init dhlist for watch path */
    LOGGER_TRACE("hlist_init");
    for (i = 0; i <= XSYNC_WATCH_PATH_HASHMAX; i++) {
        INIT_HLIST_HEAD(&client->wp_hlist[i]);
    }

    client->servers_opts->sidmax = 0;

    /**
     * initialize and watch the entire directory tree from the current working
	 * directory downwards for all events
     */
    LOGGER_DEBUG("inotifytools_initialize");

	if ( ! inotifytools_initialize()) {
        LOGGER_ERROR("inotifytools_initialize(): %s", strerror(inotifytools_error()));

        xs_client_delete((void*) client);
        return XS_ERROR;
    }

    if (opts->from_watch) {
        // 从监控目录初始化客户端: 是否递归?
        err = XS_client_conf_from_watch(client, opts->config);
    } else {
        // 从 XML 配置文件初始化客户端
        if ( ! strcmp(strrchr(opts->config, '.'), ".xml") ) {
            err = XS_client_conf_load_xml(client, opts->config);
        } else {
            LOGGER_ERROR("bad config xml file: %s", opts->config);
            err = XS_E_FILE;
        }
    }

    if (err) {
        xs_client_delete((void*) client);
        return XS_ERROR;
    }

    SERVERS = XS_client_get_server_maxid(client);

    client->threads = THREADS;
    client->queues = QUEUES;
    client->interval_seconds = SWEEP_INTERVAL_SECONDS;

    LOGGER_INFO("threads=%d queues=%d servers=%d interval=%d sec", THREADS, QUEUES, SERVERS, client->interval_seconds);

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        // 服务器数量
        perdata->server_conns[0] = (XS_server_conn) (long) SERVERS;

        perdata->threadid = i + 1;

        client->thread_args[i] = (void*) perdata;
    }

    LOGGER_DEBUG("threadpool_create: (threads=%d, queues=%d)", THREADS, QUEUES);
    client->pool = threadpool_create(THREADS, QUEUES, client->thread_args, 0);
    if (! client->pool) {
        xs_client_delete((void*) client);

        LOGGER_FATAL("threadpool_create error: Out of memory");

        // 失败退出程序
        exit(XS_ERROR);
    }

    // 初始化 event map
    for (i = 0; i <= XSYNC_WATCH_PATH_HASHMAX; i++) {
        INIT_HLIST_HEAD(&client->event_map_hlist[i]);
    }

    threadlock_init(&client->event_map_lock);

    /**
     * output XS_client
     */
    *outClient = (XS_client) RefObjectInit(client);

    return XS_SUCCESS;
}


extern XS_client XS_client_retain (XS_client *pClient)
{
    return (XS_client) RefObjectRetain((void**) pClient);
}


extern XS_VOID XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, xs_client_delete);
}


extern int XS_client_lock (XS_client client, int try)
{
    int err = 0;

    if (try) {
        err = RefObjectTryLock(client);

        if (err) {
            LOGGER_WARN("RefObjectTryLock error(%d): %s", err, strerror(err));
        }
    } else {
        err = RefObjectLock(client);

        if (err) {
            LOGGER_WARN("RefObjectLock error(%d): %s", err, strerror(err));
        }
    }

    return err;
}


extern XS_VOID XS_client_unlock (XS_client client)
{
    RefObjectUnlock(client);
}


extern int XS_client_wait_condition(XS_client client, struct timespec * timo)
{
    int err = pthread_cond_timedwait(&client->condition, &((RefObjectPtr) client)->lock__, timo);

    if (err != 0) {
        if (err == ETIMEDOUT) {
            // LOGGER_TRACE("ETIMEDOUT");
        } else if (err == EINVAL) {
            LOGGER_ERROR("EINVAL: The specified value is invalid.");
        } else if (err == EPERM) {
            LOGGER_ERROR("EPERM: The mutex was not owned by the current thread.");
        } else {
            LOGGER_ERROR("(%d): Unexpected error.", err);
        }
    }

    return err;
}


extern XS_VOID XS_client_bootstrap (XS_client client)
{
    int len, is_dir, is_lnk;

    char pathbuf[XSYNC_PATH_MAXSIZE];

    struct inotify_event *event;

    /* connect to servers for each threads */
    LOGGER_INFO("create connections to servers");

    if (XS_client_get_server_maxid(client) == 0) {
        LOGGER_WARN("no servers: see reference for how to add server!");
    } else {
        int i, sid;

        for (i = 0; i < client->threads; ++i) {
            perthread_data * perdata = (perthread_data *) client->thread_args[i];

            for (sid = 1; sid <= XS_client_get_server_maxid(client); sid++) {
                xs_server_opts * srv = XS_client_get_server_opts(client, sid);

                XS_server_conn_create(srv, client->clientid, &perdata->server_conns[sid]);

                if (perdata->server_conns[sid]->sockfd == -1) {
                    LOGGER_ERROR("[thread_%d] connect server-%d (%s:%d)",
                        perdata->threadid,
                        sid,
                        srv->host,
                        srv->port);
                } else {
                    LOGGER_INFO("[thread_%d] connected server-%d (%s:%d)",
                        perdata->threadid,
                        sid,
                        srv->host,
                        srv->port);
                }
            }
        }
    }


    /* create a sweep thread for readdir */
    pthread_t sweep_thread_id;

    LOGGER_INFO("create sweep worker thread");
    do {
        pthread_attr_t pattr;
        pthread_attr_init(&pattr);
        pthread_attr_setscope(&pattr, PTHREAD_SCOPE_PROCESS);
        pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);

        if (pthread_create(&sweep_thread_id, &pattr, (void *) sweep_worker, (void*) client)) {
            LOGGER_FATAL("pthread_create() error: %s", strerror(errno));
            pthread_attr_destroy(&pattr);
            exit(-1);
        }
        pthread_attr_destroy(&pattr);
    } while(0);


    /**
     * http://inotify-tools.sourceforge.net/api/inotifytools_8h.html
     */
    int timeout = client->interval_seconds;

    for (;;) {
        event = inotifytools_next_event(timeout);

        if (! event || ! event->len) {

            if (client_is_inotify_reload(client)) {
                inotifytools_restart(client);

                client_set_inotify_reload(client, 0);
            }

            if (XS_client_lock(client, 0) == 0) {
                /**
                 * https://bytefreaks.net/programming-2/c-full-example-of-pthread_cond_timedwait
                 */
                pthread_cond_signal(&client->condition);

                XS_client_unlock(client);
            }

            continue;
        }

        char *wpath = inotifytools_filename_from_wd(event->wd);
        if (! wpath) {
            LOGGER_FATAL("bad inotifytools_filename_from_wd(%d)", event->wd);
            continue;
        }
        
        len = snprintf(pathbuf, sizeof(pathbuf), "%s%s", wpath, event->name);
        if (len < event->len) {
            LOGGER_FATAL("event path is too long: %s%s", wpath, event->name);
            continue;
        }
        pathbuf[len] = 0;

        is_dir = isdir(pathbuf);
        is_lnk = fileislink(pathbuf, 0, 0);

        LOGGER_TRACE("events(num=%d): event=%s (isdir=%d islnk=%d)", inotifytools_get_num_watches(), pathbuf, is_dir, is_lnk);

        if (event->mask & IN_ISDIR) {
            pathbuf[len++] = '/';
            pathbuf[len] = '\0';

            int wd = inotifytools_wd_from_filename(pathbuf);

            if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
                if (wd > 0) {
                    // 删除监视
                    if (inotifytools_remove_watch_by_wd(wd)) {
                        LOGGER_INFO("success removed watch dir(wd=%d): %s", wd, pathbuf);
                    } else {
                        LOGGER_ERROR("failed to remove watch dir(wd=%d): %s", wd, pathbuf);
                        client_set_inotify_reload(client, 1);
                    }
                }
            } else if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
                if (wd > 0) {
                    inotifytools_remove_watch_by_wd(wd);

                    wd = inotifytools_wd_from_filename(pathbuf);
                }

                if (wd == -1) {
                    // 添加监视: IN_ALL_EVENTS
                    if (inotifytools_watch_recursively(pathbuf, INOTI_EVENTS_MASK)) {
                        LOGGER_INFO("success add watch dir(wd=%d): %s", inotifytools_wd_from_filename(pathbuf), pathbuf);
                    } else {
                        LOGGER_ERROR("failed to add watch dir: %s", pathbuf);
                        client_set_inotify_reload(client, 1);
                    }
                }
            } else if (event->mask & IN_CLOSE) {
                if (is_dir && wd == -1) {
                    // 添加监视: IN_ALL_EVENTS
                    if (inotifytools_watch_recursively(pathbuf, INOTI_EVENTS_MASK)) {
                        LOGGER_INFO("success add watch dir(wd=%d): %s", inotifytools_wd_from_filename(pathbuf), pathbuf);
                    } else {
                        LOGGER_ERROR("failed to add watch dir: %s", pathbuf);
                        client_set_inotify_reload(client, 1);
                    }
                }
            }
        } else {
            if (filter_watch_file(client, event->wd, pathbuf, 0) > 0) {
                if (event->mask & IN_MODIFY) {
                    LOGGER_INFO("IN_MODIFY: %s", pathbuf);
                    client_add_inotify_event(client, event);
                } else if (event->mask & IN_CLOSE) {
                    LOGGER_INFO("IN_CLOSE: %s", pathbuf);
                    client_add_inotify_event(client, event);
                } else if (event->mask & IN_MOVE) {
                    LOGGER_INFO("IN_MOVE: %s", pathbuf);
                    client_add_inotify_event(client, event);
                }
            } else {
                LOGGER_TRACE("reject file: %s", pathbuf);
            }
        }

        LOGGER_TRACE("events(num=%d)", inotifytools_get_num_watches());
    }

    LOGGER_FATAL("stopped");
}


extern XS_VOID XS_client_list_watch_paths (XS_client client, list_watch_path_cb_t list_wp_cb, void * data)
{
    int hash;
    struct hlist_node *hp, *hn;

    for (hash = 0; hash <= XSYNC_WATCH_PATH_HASHMAX; hash++) {
        hlist_for_each_safe(hp, hn, &client->wp_hlist[hash]) {
            struct xs_watch_path_t *wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);

            if (list_wp_cb(wp, data) != 1) {
                return;
            }
        }
    }
}


extern XS_VOID XS_client_clear_watch_paths (XS_client client)
{
    LOGGER_TRACE("TODO: hlist clear");
}


extern XS_BOOL XS_client_add_watch_path (XS_client client, XS_watch_path wp)
{
    if (! inotifytools_watch_recursively(wp->fullpath, INOTI_EVENTS_MASK) ) {
        LOGGER_ERROR("inotifytools_watch_recursively(): %s", strerror(inotifytools_error()));
        return XS_FALSE;
    }

    return XS_TRUE;
}


extern XS_BOOL XS_client_find_watch_path (XS_client client, char *path, XS_watch_path * outwp)
{
    struct hlist_node * hp;

    // 计算 hash
    int hash = XS_watch_path_hash_get(path);

    LOGGER_DEBUG("fullpath=%s", path);

    hlist_for_each(hp, &client->wp_hlist[hash]) {
        struct xs_watch_path_t * wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);

        LOGGER_DEBUG("hlist(wp=%p): fullpath=(%s)", wp, wp->fullpath);

        if (! strcmp(wp->fullpath, path)) {
            if (outwp) {
                *outwp = (XS_watch_path) RefObjectRetain((void**) &wp);
            }

            return XS_TRUE;
        }
    }

    return XS_FALSE;
}


extern XS_BOOL XS_client_remove_watch_path (XS_client client, char * path)
{
    /*
    struct hlist_node *hp;
    struct hlist_node *hn;

    int hash = XS_watch_path_hash_get(path);

    hlist_for_each_safe(hp, hn, &client->wp_hlist[hash]) {
        struct xs_watch_path_t * wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);
        if (! strcmp(wp->fullpath, path)) {
            int wd = wp->watch_wd;

            if (inotify_rm_watch(client->infd, wd) == 0) {
                LOGGER_DEBUG("xpath=%p, inotify_rm_watch success(0). (%s)", wp, wp->fullpath);

                wp = client_wd_table_remove(client, wp);
                assert(wp->watch_wd == wd);
                wp->watch_wd = -1;

                //?? hlist_del(hp);
                hlist_del(&wp->i_hash);

                XS_watch_path_release(&wp);

                return XS_TRUE;
            } else {
                LOGGER_ERROR("xpath=%p, inotify_rm_watch error(%d: %s). (%s)", wp, errno, strerror(errno), wp->fullpath);
                return XS_FALSE;
            }
        }
    }
    */

    return XS_FALSE;
}
