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
 * @version: 0.1.9
 *
 * @create: 2018-01-25
 *
 * @update: 2018-10-18 21:13:26
 */

/******************************************************************************
 * client_init_from_watch
 *
 * 从 watch 目录初始化 client. watch 目录遵循下面的规则:
 *
 *        xclient/                               (客户端主目录)
 *            |
 *            +---- CLIENTID  (客户端唯一标识 ID, 确保名称符合文件目录名规则, 长度不大于 36 字符)
 *            |
 *            +---- LICENCE                      (客户端版权文件)
 *            |
 *            +---- bin/                         (常用脚本目录)
 *            |       |
 *            |       +---- 常用脚本
 *            |
 *            +---- sbin/                        (客户端程序目录)
 *            |       |
 *            |       +---- xsync-client -> xsync-client-$verno (客户端程序链接)
 *            |       |
 *            |       +---- xsync-client-$verno  (客户端程序)
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
 *            |       +---- pepstack-server/     (服务器 1 名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |          ...
 *            |       |
 *            |       +---- giant-logserver/     (服务器 2 名称目录)
 *            |       |        |
 *            |       |        +---- SERVERID    (内容仅1行 = "host:port#magic")
 *            |       |
 *            |       +---- ...
 *            |
 *            |
 *            +---- watch/             (可选, 客户端 watch 目录 - 符号链接)
 *                    |
 *                    +---- pathid0 -> /path/to/A/
 *                    |
 *                    +---- pathid1 -> /path/to/B/
 *                    |
 *                    +---- pathid2 -> /path/to/C/
 *                    |        ...
 *                    +---- pathid255 -> /path/to/N/      (最多 256 个 pathid)
 *                    |
 *                    +---- 1   (服务器 sid=1, 内容仅1行 = "host:port#magic";
 *                    |          或者为链接文件: 1 -> ../sid/nameserver/SERVERID)
 *                    |
 *                    +---- 2   (服务器 sid=2, 内容仅1行 = "host:port#magic";
 *                    |          或者为链接文件: 2 -> ../sid/?/SERVERID)
 *                    |    ...
 *                    |
 *                    +---- i   (服务器 sid=i. 服务器 sid顺序编制, 不可以跳跃 !!
 *                    |          最大服务号不可以超过 XSYNC_SERVER_MAXID. 参考手册)
 *                    |
 *                    |     (以下文件为可选)
 *                    |
 *                    +---- i.included
 *                    |
 *                    +---- path-filter.sh  (路径和文件名过滤脚本: 用户实现)
 *                    |
 *                    +---- event-task.sh   (事件任务执行脚本: 用户实现)
 *
 *****************************************************************************/
#include "client_api.h"
#include "client_conf.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/common_util.h"

#include "../kafkatools/kafkatools.h"


static int send_kafka_message(kafkatools_producer_api_t *api, const char *kafka_topic, int kafka_partition, const char *msg, int msglen)
{
    int ret;

    kt_topic topic = api->kt_get_topic(api->producer, kafka_topic);

    if (! topic) {
        LOGGER_ERROR("no topic: %s", api->kt_producer_get_errstr(api->producer));
        return (-1);
    } else {
        LOGGER_TRACE("topic(%p): %s", topic, api->kt_topic_name(topic));
    }

    ret = api->kt_produce_message_sync(api->producer, msg, msglen, topic, kafka_partition, -1);

    if (ret == KAFKATOOLS_SUCCESS) {
        LOGGER_TRACE("kafkatools_produce_message_sync success: %s", msg);
    } else {
        LOGGER_ERROR("kafkatools_produce_message_sync fail: %s", api->kt_producer_get_errstr(api->producer));
    }

    return ret;
}


static void do_event_task (thread_context_t *thread_ctx)
{
    threadpool_task_t *task = thread_ctx->task;

    if (task->flags == 100) {
        int ok;

        int msglen;
        char *message;

        char *kafka_topic;
        int partition = 0;

        // TODO:
        int sid = 0;

        perthread_data *perdata = (perthread_data *) thread_ctx->thread_arg;
        XS_client client = (XS_client) perdata->xclient;

        red_black_node_t *node = (red_black_node_t *) task->argument;
        XS_watch_event event = (XS_watch_event) node->object;

        bzero(perdata->buffer, sizeof(perdata->buffer));

        char *v_type = v_type_buf(perdata);
        char *v_time =  v_time_buf(perdata);
        char *v_sid =  v_sid_buf(perdata);
        char *v_thread =  v_thread_buf(perdata);
        char *v_event =  v_event_buf(perdata);
        char *v_clientid =  v_clientid_buf(perdata);
        char *v_pathid =  v_pathid_buf(perdata);
        char *v_file =  v_file_buf(perdata);
        char *v_route =  v_route_buf(perdata);
        char *v_path =  v_path_buf(perdata);
        char *v_eventmsg =  v_eventmsg_buf(perdata);

        ok = 0;

        __inotifytools_lock();
        {
            if (xs_client_find_wpath_inlock(client, event->pathname,
                perdata->buffer, sizeof(perdata->buffer),
                v_clientid, v_clientid_cb(perdata),
                v_pathid, v_pathid_cb(perdata),
                v_route, v_route_cb(perdata)) != -1) {
                /**
                 * 查找路由成功, 设置字段值
                 */
                snprintf(v_type_buf(perdata), v_type_cb(perdata), "%d", task->flags);

                now_time_str(v_time_buf(perdata), v_time_cb(perdata));

                snprintf(v_thread, v_thread_cb(perdata), "%d", perdata->threadid);
                snprintf(v_event, v_event_cb(perdata), "%s", inotifytools_event_to_str(event->mask));
                snprintf(v_file, v_file_cb(perdata), "%s", event->name);
                snprintf(v_path, v_path_cb(perdata), "%s", event->pathname);
                snprintf(v_sid, v_sid_cb(perdata), "%d", sid);

                // 默认的 kafka 消息
                kafka_topic = v_pathid;

                ok = 1;
            }
        }
        __inotifytools_unlock();

        if (! ok) {
            LOGGER_FATAL("should never run to this: xs_client_find_wpath_inlock fail");
            exit(-10);
        }

        msglen = 0;
        message = 0;

        if (perdata->luactx) {
            // 使用脚本过滤事件消息
            if (LuaCtxLockState(perdata->luactx)) {
                const char *keys[] = {
                    "type", "time", "clientid", "thread", "sid", "event", "pathid", "path", "file", "route"
                };

                const char *values[] = {
                    v_type, v_time, v_clientid, v_thread, v_sid, v_event, v_pathid, v_path, v_file, v_route
                };

                if (LuaCtxCallMany(perdata->luactx, "on_event_task", keys, values, sizeof(keys)/sizeof(keys[0])) == LUACTX_SUCCESS) {
                    char *result;

                    if (LuaCtxGetValueByKey(perdata->luactx, "result", 6, &result) && !strcmp(result, "SUCCESS")) {
                        // 得到用户处理后的消息
                        msglen = LuaCtxGetValueByKey(perdata->luactx, "message", 7, &message);

                        if (perdata->kafka_producer_ready) {
                            // 如果要求写入 kafka, 取得当前文件的 kafka 配置: topic, partition
                            if (LuaCtxGetValueByKey(perdata->luactx, "kafka_partition", 15, &result)) {
                                partition = atoi(result);
                            } else {
                                LOGGER_WARN("using default kafka partition: %d", partition);
                            }

                            if (! LuaCtxGetValueByKey(perdata->luactx, "kafka_topic", 11, &kafka_topic)) {
                                LOGGER_WARN("using default kafka topic: %s", kafka_topic);
                            }
                        }
                    } else {
                        LOGGER_WARN("on_event_task() result not SUCCESS");
                    }
                } else {
                    LOGGER_WARN("LuaCtxCallMany fail: on_event_task()");
                }

                LuaCtxUnlockState(perdata->luactx);
            }
        }

        if (! message) {
            msglen = snprintf(v_eventmsg, v_eventmsg_cb(perdata), "{%s|%s|%s|%s|%s|%s|%s|%s|%s|%s}",
                v_type, v_time, v_clientid, v_thread, v_sid, v_event, v_pathid, event->pathname, event->name, v_route);

            if (msglen > 0 && msglen < v_eventmsg_cb(perdata)) {
                message = v_eventmsg;
            } else {
                LOGGER_FATAL("application error: buffer is too small (see perthread_data.h).");
                exit(-10);
            }
        }

        if (perdata->kafka_producer_ready) {
            // 发送消息到 kafka (同步)
            LOGGER_DEBUG("send event to kafka (%s:%d)", kafka_topic, partition);

            send_kafka_message(&perdata->kt_producer_api, kafka_topic, partition, message, msglen);
        }

        // 发送消息到日志文件. TODO: 得到 loglevel
        LOGGER_DEBUG("event(%d)=%s", msglen, message);

        msglen = 0;

        // 使用完毕必须删除 !!
        event_rbtree_lock();
        {
            node->object = 0;
            watch_event_free(event);

            rbtree_remove_at(&client->event_rbtree, node);

            msglen = rbtree_size(&client->event_rbtree);
        }
        event_rbtree_unlock();

        LOGGER_DEBUG("rbtree_size=%d", msglen);
    } else {
        LOGGER_ERROR("unknown event task flags(=%d)", task->flags);
    }

    task->argument = 0;
    task->flags = 0;
}


__no_warning_unused(static)
XS_RESULT client_add_inotify_event (XS_client client, struct watch_event_buf_t *evbuf)
{
    int result;
    red_black_node_t *node;

    if (XS_client_threadpool_unused_queues(client) < 1) {
        LOGGER_TRACE("threadpool queues is full");
        return XS_E_POOL;
    }

    result = XS_ERROR;

    event_rbtree_lock();

    node = rbtree_find(&client->event_rbtree, evbuf);

    if (node) {
        event_rbtree_unlock();

        LOGGER_WARN("existing node(=%p)", node);
        return XS_SUCCESS;
    } else {
        int is_new_node;

        XS_watch_event newevent = watch_event_clone((const watch_event_t *) evbuf);

        node = rbtree_insert_unique(&client->event_rbtree, (void *)newevent, &is_new_node);
        if (node) {
            if (is_new_node) {
                result = threadpool_add(client->pool, do_event_task, (void*)node, 100);

                if (result) {
                    LOGGER_ERROR("threadpool_add node(=%p) fail: %s", node, threadpool_error_messages[-result]);
                    node->object = 0;
                    watch_event_free(newevent);
                    rbtree_remove_at(&client->event_rbtree, node);
                    result = XS_E_POOL;
                } else {
                    LOGGER_DEBUG("threadpool_add node(=%p) success", node);
                    result = XS_SUCCESS;
                }
            } else {
                LOGGER_ERROR("should never run to this: rbtree_insert_unique existed");
                watch_event_free(newevent);
            }
        } else {
            LOGGER_ERROR("should never run to this: rbtree_insert_unique unexpected");
            watch_event_free(newevent);
        }
    }

    event_rbtree_unlock();

    return result;
}


/**
 * 调用外部脚本 path-filter.lua 过滤文件路径目录
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
    int len, ret, wd;

    char *abspath = realpath(path, pathbuf);
    if (! abspath) {
        LOGGER_ERROR("realpath error(%d): %s (%s)", errno, strerror(errno), path);
        return 0;
    }

    len = slashpath(abspath, PATH_MAX);
    if (len <= 0) {
        LOGGER_ERROR("path too long: %s", path);
        return 0;
    }

    LOGGER_TRACE("path={%s}", abspath);

    // 调用外部脚本, 过滤监视路径
    ret = FILTER_WPATH_ACCEPT;

    if (LuaCtxLockState(client->luactx)) {
        LuaCtxCall(client->luactx, "filter_path", "path", abspath);

        // TODO: ret

        LuaCtxUnlockState(client->luactx);
    }

    if (ret == FILTER_WPATH_ACCEPT) {
        // 接受目录
        LOGGER_DEBUG("ACCEPT(=%d): %s", ret, abspath);

        wd = inotifytools_wd_from_filename_s(abspath);
        if (wd < 1) {
            // 接受的监视目录不存在, 重启监视
            LOGGER_ERROR("path not in watch: %s -> %s", client->watch_config, abspath);
            return (-1);
        }
        return wd;
    }

    if (ret == FILTER_WPATH_REJECT) {
        // 拒绝目录
        LOGGER_DEBUG("REJECT(=%d): %s", ret, abspath);

        wd = inotifytools_wd_from_filename_s(abspath);
        if (wd > 0) {
            // 拒绝的监视目录存在, 删除监视
            if (inotifytools_remove_watch_by_wd_s(wd)) {
                // 移除监视成功, 不再递归
                LOGGER_INFO("remove watch path(wd=%d) ok: %s", wd, abspath);
                return 0;
            } else {
                // 移除监视失败, 重启监视
                LOGGER_ERROR("remove watch path(wd=%d) failed: %s", wd, abspath);
                return (-1);
            }
        }

        // 拒绝目录已经被移除监视, 不再递归
        return 0;
    }

    if (ret == FILTER_WPATH_RELOAD) {
        // 重置
        LOGGER_DEBUG("RELOAD(=%d): %s", ret, abspath);
        return (-1);
    }

    LOGGER_ERROR("UNEXPECTED(=%d): %s", ret, abspath);
    return 0;
}


__no_warning_unused(static)
int filter_watch_file (XS_client client, char *path, const char *name, int namelen)
{
    int retcode = FILTER_WPATH_ACCEPT;

    if (! path || ! name) {
        LOGGER_ERROR("application error: null name");
        return 0;
    }

    if (LuaCtxLockState(client->luactx)) {
        const char *keys[] = {"path", "file"};
        const char *values[] = {path, name};

        LuaCtxCallMany(client->luactx, "filter_file", keys, values, 2);

        // TODO: retcode

        LuaCtxUnlockState(client->luactx);
    }

    if (retcode == FILTER_WPATH_ACCEPT) {
        LOGGER_DEBUG("ACCEPT(=%d): {%s%s}", retcode, path, name);
        return 1;
    }

    if (retcode == FILTER_WPATH_REJECT) {
        LOGGER_DEBUG("REJECT(=%d): {%s%s}", retcode, path, name);
        return 0;
    }

    if (retcode == FILTER_WPATH_RELOAD) {
        LOGGER_WARN("RELOAD(=%d): {%s%s}", retcode, path, name);
        return (-1);
    }

    LOGGER_ERROR("UNEXPECTED(=%d): {%s%s}", retcode, path, name);
    return 0;
}

/**
 * 返回值:
 *   1: 继续
 *   0: 中止
 */
__no_warning_unused(static)
int lscb_sweep_watch_path (const char *path, int pathlen, struct mydirent *myent, void *arg1, void *arg2)
{
    struct watch_event_buf_t evbuf = {0};

    XS_client client = (XS_client) arg1;

    sleep_ms(10);

    if (! pathlen || pathlen >= PATH_MAX) {
        LOGGER_FATAL("bad pathlen: %d", pathlen);
        exit(-1);
    }

    if (myent->isdir) {
        // path 是目录
        if (myent->islnk) {
            // path 是目录链接
            evbuf.wd = filter_watch_path((XS_client) arg1, path, evbuf.pathname);

            if (evbuf.wd > 0) {
                if (arg2) {
                    // 当前子目录不支持符号链接
                    LOGGER_ERROR("child dir link not supported: %s -> %s", path, evbuf.pathname);
                } else {
                    listdir(path, evbuf.pathname, sizeof(evbuf.pathname), (listdir_callback_t) lscb_sweep_watch_path, arg1, int_cast_to_pv(evbuf.wd));
                }
            } else if (evbuf.wd == -1) {
                client_set_inotify_reload(client, 1);
                return 0;
            }
        } else {
            // path 是物理目录
            if (arg2) {
                evbuf.wd = filter_watch_path(client, path, evbuf.pathname);

                if (evbuf.wd > 0) {
                    char * wpath = evbuf.pathname;

                    LOGGER_TRACE("sweep path(wd=%d): %s", evbuf.wd, wpath);

                    listdir(path, evbuf.pathname, sizeof(evbuf.pathname), (listdir_callback_t) lscb_sweep_watch_path, arg1, int_cast_to_pv(evbuf.wd));

                } else if (evbuf.wd == -1) {
                    client_set_inotify_reload(client, 1);
                    return 0;
                }
            }
        }
    } else if (myent->isreg) {
        char *name = strrchr(path, '/');

        if (name && *name++) {
            int result = 0;

            evbuf.wd = pv_cast_to_int(arg2);
            evbuf.mask = IN_CLOSE_NOWRITE;
            evbuf.cookie = 0;
            evbuf.len = 0;

            if (evbuf.wd > 0) {
                __inotifytools_lock();
                {
                    // 监视的绝对目录: evbuf.pathname
                    char *abspath = inotifytools_filename_from_wd(evbuf.wd);

                    if (abspath) {
                        evbuf.pathlen = strlen(abspath);
                        memcpy(evbuf.pathname, abspath, evbuf.pathlen);
                        evbuf.pathname[evbuf.pathlen] = 0;

                        evbuf.len = (int) strlen(name);

                        if (evbuf.len < sizeof(evbuf.name)) {
                            memcpy(evbuf.name, name, evbuf.len);
                            evbuf.name[evbuf.len] = '\0';
                        } else {
                            // error name buffer
                            evbuf.len = 0;
                        }
                    }
                }
                __inotifytools_unlock();
            } else {
                // 监视的绝对目录: evbuf.pathname
                evbuf.pathlen = name - path;
                memcpy(evbuf.pathname, path, evbuf.pathlen);
                evbuf.pathname[evbuf.pathlen] = 0;

                evbuf.wd = 0;
                evbuf.len = (int) strlen(name);

                if (evbuf.len < sizeof(evbuf.name)) {
                    memcpy(evbuf.name, name, evbuf.len);
                    evbuf.name[evbuf.len] = '\0';
                } else {
                    // error name buffer
                    evbuf.len = 0;
                }
            }

            if (evbuf.len) {
                red_black_node_t *node = 0;

                LOGGER_TRACE("sweep event(wd=%d)[%s]: %s", evbuf.wd, inotifytools_event_to_str(evbuf.mask), name);

                /**
                 * 判断当前文件是否正在任务队列中处理, 如果在, 则忽略之
                 */
                if (pthread_mutex_lock(&client->rbtree_lock) == 0) {
                    node = rbtree_find(&client->event_rbtree, &evbuf);

                    pthread_mutex_unlock(&client->rbtree_lock);

                    if (! node) {
                        result = filter_watch_file(client, evbuf.pathname, evbuf.name, evbuf.len);
                    }
                } else {
                    LOGGER_WARN("pthread_mutex_lock failed on rbtree_lock");
                }
            }

            if (result > 0) {
                // 添加任务到线程池, 如果任务队列忙, 则重试
                while (client_add_inotify_event(client, &evbuf) == XS_E_POOL) {
                    sleep_ms(10);
                }
            } else if (result == -1) {
                // 要求重启服务
                client_set_inotify_reload(client, 1);
                return 0;
            }
        }
    } else if (myent->islnk) {
        if (! arg2) {
            // 忽略根目录的文件链接
        } else {
            // 忽略子目录的文件链接
            char *abspath = realpath(path, evbuf.pathname);

            if (abspath) {
                LOGGER_WARN("ignored file link: %s -> %s", path, abspath);
            } else {
                LOGGER_ERROR("realpath error(%d): %s (%s)", errno, strerror(errno), path);
            }
        }
    }

    return 1;
}


__no_warning_unused(static)
void xs_inotifytools_restart(XS_client client)
{
    int err = (-1);

    LOGGER_DEBUG("inotifytools_cleanup()");

    inotifytools_cleanup_s();

    if ( ! inotifytools_initialize_s()) {
        LOGGER_ERROR("inotifytools_initialize(): %s", strerror(inotifytools_error()));
        exit(XS_ERROR);
    }

    if (client->from_watch) {
        // 从监视目录初始化客户端
        err = XS_client_conf_from_watch(client, 0);
    } else {
        // 从 XML 配置文件初始化客户端
        err = XS_client_conf_from_xml(client, 0);
    }

    if (err != XS_SUCCESS) {
        inotifytools_cleanup_s();

        LOGGER_ERROR("failed");

        exit(XS_ERROR);
    }
}


/***********************************************************************
 *
 * XS_client application api
 *
 **********************************************************************/
XS_RESULT XS_client_create (xs_appopts_t *opts, XS_client *outClient)
{
    XS_client client;

    int i, err;

    int SERVERS = 0;
    int THREADS = opts->threads;
    int QUEUES = opts->queues;

    *outClient = 0;

    client = (XS_client) mem_alloc(1, sizeof(xs_client_t));

    /* xsync-client app home dir */
    memcpy(client->apphome, opts->apphome, opts->apphome_len);
    client->apphome_len = opts->apphome_len;

    client->kafka = opts->kafka;

    if (opts->clientid[0]) {
        // 通过命令行参数设置 clientid
        memcpy(client->clientid, opts->clientid, sizeof(client->clientid));
    } else {
        // TODO: 从本地磁盘得到 clientid
        LOGGER_WARN("TODO: no CLIENTID specified");
    }
    client->clientid[sizeof(client->clientid) - 1] = '\0';

    __interlock_release(&client->task_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&client->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));

        free((void *)client);

        exit(XS_ERROR);
    }

    client->servers_opts->sidmax = 0;

    /**
     * initialize and watch the entire directory tree from the current working
     * directory downwards for all events
     */
    LOGGER_DEBUG("inotifytools_initialize");

    if (! inotifytools_initialize()) {
        LOGGER_ERROR("inotifytools_initialize(): %s", strerror(inotifytools_error()));
        xs_client_delete((void*) client);

        exit(XS_ERROR);
    }

    if (opts->from_watch) {
        // 从监视目录初始化客户端: 是否递归
        client->from_watch = 1;

        err = XS_client_conf_from_watch(client, opts->config);
    } else {
        // 从 XML 配置文件初始化客户端
        client->from_watch = 0;

        err = XS_client_conf_from_xml(client, opts->config);
    }

    if (err) {
        xs_client_delete((void*)client);

        exit(XS_ERROR);
    }

    SERVERS = XS_client_get_server_maxid(client);

    client->threads = THREADS;
    client->queues = QUEUES;
    client->interval_seconds = opts->sweep_interval;

    LOGGER_INFO("CLIENTID(=%s): threads=%d queues=%d servers=%d interval=%d", client->clientid, THREADS, QUEUES, SERVERS, client->interval_seconds);

    /* create per thread data and initialize */
    snprintf(client->buffer, sizeof(client->buffer), "%sevent-task.lua", client->watch_config);
    if (access(client->buffer, F_OK|R_OK|X_OK)) {
        LOGGER_WARN("event-task.lua cannot access (%d): %s (%s)", errno, strerror(errno), client->buffer);
        *client->buffer = 0;
    }

    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));
    for (i = 0; i < THREADS; ++i) {
        const char *luascriptfile = (client->buffer[0]? client->buffer : 0);

        client->thread_args[i] = perthread_data_create(client, SERVERS, i+1, luascriptfile);
    }

    LOGGER_DEBUG("threadpool_create: (threads=%d, queues=%d)", THREADS, QUEUES);
    client->pool = threadpool_create(THREADS, QUEUES, client->thread_args, 0);
    if (! client->pool) {
        xs_client_delete((void*) client);

        LOGGER_FATAL("threadpool_create error: Out of memory");

        // 失败退出程序
        exit(XS_ERROR);
    }

#ifdef XSYNC_USE_STATIC_PATHID_TABLE
    LOGGER_WARN("XSYNC_USE_STATIC_PATHID_TABLE");
#else
    LOGGER_TRACE("wd_pathid_rbtree init");
    rbtree_init(&client->wd_pathid_rbtree, (fn_comp_func*) wd_pathid_rbtree_cmp);
#endif

    // 初始化 event_rbtree
    LOGGER_TRACE("event_rbtree");
    rbtree_init(&client->event_rbtree, (fn_comp_func*) event_rbtree_cmp);
    threadlock_init(&client->rbtree_lock);

    /**
     * output XS_client
     */
    *outClient = (XS_client) RefObjectInit(client);

    return XS_SUCCESS;
}


XS_client XS_client_retain (XS_client *pClient)
{
    return (XS_client) RefObjectRetain((void**) pClient);
}


XS_VOID XS_client_release (XS_client * inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, xs_client_delete);
}


int XS_client_lock (XS_client client, int try)
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


XS_VOID XS_client_unlock (XS_client client)
{
    RefObjectUnlock(client);
}


/**
 * callback when inotify add watch
 */
int on_inotify_add_wpath (int flag, const char *wpath, void *arg)
{
    int err, i, num;

    XS_client client = (XS_client) arg;

    if (flag == INO_WATCH_ON_QUERY) {
        LOGGER_INFO("INO_WATCH_ON_QUERY: %s", wpath);

        if (LuaCtxLockState(client->luactx)) {
            err = LuaCtxCall(client->luactx, "inotify_watch_on_query", "wpath", wpath);

            if (! err) {
                num = LuaCtxNumPairs(client->luactx);

                for (i = 0; i < num; i++) {
                    char *key, *value;

                    LuaCtxGetKey(client->luactx, i, &key);

                    LuaCtxGetValue(client->luactx, i, &value);

                    printf("inotify_watch_on_query output table[%d] = {%s => %s}\n", i, key, value);
                }
            }

            LuaCtxUnlockState(client->luactx);
        }
    } else if (flag == INO_WATCH_ON_READY) {
        LOGGER_INFO("INO_WATCH_ON_READY: %s", wpath);

        if (LuaCtxLockState(client->luactx)) {
            err = LuaCtxCall(client->luactx, "inotify_watch_on_ready", "wpath", wpath);

            if (! err) {
                num = LuaCtxNumPairs(client->luactx);

                for (i = 0; i < num; i++) {
                    char *key, *value;

                    LuaCtxGetKey(client->luactx, i, &key);

                    LuaCtxGetValue(client->luactx, i, &value);

                    printf("inotify_watch_on_ready output table[%d] = {%s => %s}\n", i, key, value);
                }
            }

            LuaCtxUnlockState(client->luactx);
        }
    } else if (flag == INO_WATCH_ON_ERROR) {
        LOGGER_ERROR("INO_WATCH_ON_ERROR: %s", wpath);

        if (LuaCtxLockState(client->luactx)) {
            err = LuaCtxCall(client->luactx, "inotify_watch_on_error", "wpath", wpath);

            if (! err) {
                num = LuaCtxNumPairs(client->luactx);

                for (i = 0; i < num; i++) {
                    char *key, *value;

                    LuaCtxGetKey(client->luactx, i, &key);

                    LuaCtxGetValue(client->luactx, i, &value);

                    printf("inotify_watch_on_error output table[%d] = {%s => %s}\n", i, key, value);
                }
            }

            LuaCtxUnlockState(client->luactx);
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
    char pathbuf[PATH_MAX];

    XS_client client = (XS_client) arg;

    for (;;) {
        sleep_ms(client->interval_seconds * 1000);

        listdir(client->watch_config, pathbuf, sizeof(pathbuf), (listdir_callback_t) lscb_sweep_watch_path, (void*) client, 0);
    }

    LOGGER_FATAL("thread exit unexpected.");

    pthread_exit (0);
}


XS_VOID XS_client_bootstrap (XS_client client)
{
    char pathbuf[PATH_MAX];

    struct inotify_event *inevent;

    struct watch_event_buf_t evbuf = {0};

    pthread_t sweep_thread_id;

    /**
     * connect to servers for each threads
     */
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

    /**
     * create a sweep thread for readdir
     */
    if (client->interval_seconds > 0 && client->interval_seconds < 864000) {
        LOGGER_INFO("create sweep worker with interval seconds=%d", client->interval_seconds);
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
    } else {
        LOGGER_WARN("no sweep worker needed: wrong interval seconds(=%d)", client->interval_seconds);
    }

    /**
     * http://inotify-tools.sourceforge.net/api/inotifytools_8h.html
     */
    for (;;) {
        if (client_is_inotify_reload(client)) {
            xs_inotifytools_restart(client);
            client_set_inotify_reload(client, 0);
        }

        __inotifytools_lock();
        {
            // 必须是立即返回
            inevent = inotifytools_next_event(0);

            if (! inevent) {
                __inotifytools_unlock();
                sleep_ms(10);
                continue;
            }

            LOGGER_DEBUG("inotify event(wd=%d)[%s]: %s (len=%d)", inevent->wd, inotifytools_event_to_str(inevent->mask), inevent->name, inevent->len);

            if (inevent->mask & (IN_DELETE_SELF | IN_IGNORED) && ! inevent->len) {
                __inotifytools_unlock();
                continue;
            }

            if (! inotify_event_dump((watch_event_t *)&evbuf, PATH_MAX, inevent)) {
                __inotifytools_unlock();

                LOGGER_FATAL("unexpected error");
                continue;
            }
        }
        __inotifytools_unlock();

        LOGGER_TRACE("inotify event(wd=%d)[%s]: %s", evbuf.wd, inotifytools_event_to_str(evbuf.mask), evbuf.name);

        if (evbuf.mask & IN_ISDIR) {
            int len = snprintf(pathbuf, sizeof(pathbuf), "%s%s/", evbuf.pathname, evbuf.name);
            if (len < 0 || len >= sizeof(pathbuf)) {
                LOGGER_FATAL("pathbuf was truncated for: '%s%s/'", evbuf.pathname, evbuf.name);
                continue;
            }
            pathbuf[len] = 0;

            int wd = inotifytools_wd_from_filename_s(pathbuf);

            if (evbuf.mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
                if (wd > 0) {
                    // 删除监视
                    if (inotifytools_remove_watch_by_wd_s(wd)) {
                        LOGGER_INFO("inotify remove wpath success: (%d: %s)", wd, pathbuf);
                    } else {
                        LOGGER_ERROR("inotify remove wpath fail: (%d: %s)", wd, pathbuf);

                        client_set_inotify_reload(client, 1);
                    }
                }
            } else if (evbuf.mask & (IN_CREATE | IN_MOVED_TO)) {
                if (wd > 0) {
                    // 删除监视
                    inotifytools_remove_watch_by_wd_s(wd);
                    wd = inotifytools_wd_from_filename_s(pathbuf);
                }

                if (wd == -1) {
                    // 添加监视:
                    if (inotifytools_watch_recursively_s(pathbuf, INOTI_EVENTS_MASK, on_inotify_add_wpath, client)) {
                        LOGGER_INFO("inotify add wpath success: (%s)", pathbuf);
                    } else {
                        LOGGER_ERROR("inotify add wpath fail: (%s)", pathbuf);

                        client_set_inotify_reload(client, 1);
                    }
                }
            } else if (evbuf.mask & IN_CLOSE) {
                LOGGER_ERROR("should never run to this!");
            }

            continue;
        } else if (evbuf.mask & INOTI_EVENTS_MASK) {
            red_black_node_t *node = 0;

            /**
             * 判断当前文件是否正在任务队列中处理, 如果在, 则忽略之
             */
            event_rbtree_lock();
            node = rbtree_find(&client->event_rbtree, (watch_event_t *)&evbuf);
            event_rbtree_unlock();

            if (! node) {
                /**
                 * evbuf.len 总是等于 strlen(evbuf.name)
                 */
                if (filter_watch_file(client, evbuf.pathname, evbuf.name, evbuf.len) > 0) {
                    // 循环直到添加成功
                    while (client_add_inotify_event(client, &evbuf) == XS_E_POOL) {
                        sleep_ms(1);
                    }
                } else {
                    LOGGER_TRACE("reject file: %s%s", evbuf.pathname, evbuf.name);
                }
            }

            continue;
        }

        LOGGER_WARN("unhandled inotify event(wd=%d)", evbuf.wd);
    }

    LOGGER_FATAL("unexpected end.");
}
