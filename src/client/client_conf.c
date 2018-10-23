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
 * @file: client_conf.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.6
 *
 * @create: 2018-01-26
 *
 * @update: 2018-10-22 10:56:41
 */

#include "client_api.h"

#include "client_conf.h"

#include "../xsync-xmlconf.h"
#include "../common/readconf.h"


void * perthread_data_create (XS_client client, int servers, int threadid, const char *taskscriptfile)
{
    perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

    if (taskscriptfile) {
        LOGGER_INFO("[thread-%d] loading lua script: %s", threadid, taskscriptfile);

        if (LuaCtxNew(taskscriptfile, LUACTX_THREAD_MODE_SINGLE, &perdata->luactx) != LUACTX_SUCCESS) {
            LOGGER_FATAL("LuaCtxNew fail");

            free(perdata);
            exit(-1);
        }
    }

    if (client->kafka) {
        if (perdata->luactx) {
            /* '/home/root1/Workspace/github.com/pepstack/xsync/target/libkafkatools.so.1' */
            client->apphome[client->apphome_len] = 0;
            strcat(client->apphome, "libkafkatools.so.1");

            /* 得到 kafka configuration */
            if (LuaCtxCall(perdata->luactx, "kafka_config", "kafkalib", client->apphome) == LUACTX_SUCCESS) {
                char *result = 0;
                char *bootstrap_servers = 0;
                char *socket_timeout_ms = 0;

                if (LuaCtxGetValueByKey(perdata->luactx, "result", 6, &result) && !strcmp(result, "SUCCESS")) {

                    if (LuaCtxGetValueByKey(perdata->luactx, "bootstrap_servers", 17, &bootstrap_servers)) {
                        char default_timeout_ms[] = "1000";

                        LuaCtxGetValueByKey(perdata->luactx, "socket_timeout_ms", 17, &socket_timeout_ms);
                        if (! socket_timeout_ms) {
                            socket_timeout_ms = default_timeout_ms;
                        }

                        LOGGER_INFO("[thread-%d] create kafka producer (bootstrap.servers=%s)", threadid, bootstrap_servers);

                        do {
                            const char *names[] = {
                                "bootstrap.servers",
                                "socket.timeout.ms",
                                0
                            };

                            const char *values[] = {
                                bootstrap_servers,
                                socket_timeout_ms,
                                0
                            };

                            if (kafka_producer_api_create(&perdata->kt_producer_api, client->apphome, names, values)) {
                                LOGGER_FATAL("kafka_producer_api_create fail");

                                LuaCtxFree(&perdata->luactx);

                                free(perdata);
                                exit(-1);
                            }

                            perdata->kafka_producer_ready = 1;
                        } while (0);
                    } else {
                        LOGGER_FATAL("kafka_config() fail to gey key: bootstrap_servers");

                        LuaCtxFree(&perdata->luactx);
                        free(perdata);
                        exit(-1);
                    }
                } else {
                    LOGGER_FATAL("LuaCtxCall kafka_config() result != SUCCESS");

                    LuaCtxFree(&perdata->luactx);
                    free(perdata);
                    exit(-1);
                }
            } else {
                LOGGER_FATAL("LuaCtxCall kafka_config() fail. see: %s", taskscriptfile);

                LuaCtxFree(&perdata->luactx);
                free(perdata);
                exit(-1);
            }
        } else {
            LOGGER_FATAL("luacontext not set");

            free(perdata);
            exit(-1);
        }
    }

    /* here all is ok */
    client->apphome[client->apphome_len] = 0;

    /* 服务器数量 */
    perdata->server_conns[0] = (XS_server_conn) int_cast_to_pv(servers);

    /* 线程 id: 1 based */
    perdata->threadid = threadid;

    /* 没有引用计数 */
    perdata->xclient = (void *) client;

    return (void *) perdata;
}


void perthread_data_free (perthread_data *perdata)
{
    int sid;

    int sid_max = pv_cast_to_int(perdata->server_conns[0]);

    for (sid = 1; sid <= sid_max; sid++) {
        XS_server_conn conn = perdata->server_conns[sid];

        if (conn) {
            perdata->server_conns[sid] = 0;

            XS_server_conn_release(&conn);
        }
    }

    kafka_producer_api_free(&perdata->kt_producer_api);

    LuaCtxFree(&perdata->luactx);

    free(perdata);
}



void xs_client_delete (void *pv)
{
    int i;

    XS_client client = (XS_client) pv;

    LOGGER_TRACE("inotifytools_cleanup()");
    inotifytools_cleanup_s();

    if (client->pool) {
        LOGGER_DEBUG("threadpool_destroy");
        threadpool_destroy(client->pool, 0);
    }

    if (client->thread_args) {
        for (i = 0; i < client->threads; ++i) {
            perthread_data * perdata = client->thread_args[i];
            client->thread_args[i] = 0;
            perthread_data_free(perdata);
        }
        free(client->thread_args);
    }

    LOGGER_TRACE("clean event_rbtree");
    threadlock_destroy(&client->rbtree_lock);
    do {
        // TODO:
        rbtree_clean(&client->event_rbtree);
    } while (0);

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&client->condition);

    LuaCtxFree(&client->luactx);

#ifdef XSYNC_USE_STATIC_PATHID_TABLE
    for (i = 0; i < XSYNC_WATCH_PATHID_MAX; i++) {
        char *pathid = client->wd_pathid_table[i];
        if (pathid) {
            client->wd_pathid_table[i] = 0;
            free(pathid);
        }
    }
#else
    // TODO:
    rbtree_clean(&client->wd_pathid_rbtree);
#endif

    LOGGER_TRACE("~XS_client(%p)", client);
    free(client);
}


/**
 * 根据 wd 查找路径表. 这个函数总应该成功. 否则是编程问题 !!
 */
int xs_client_find_wpath_inlock (XS_client client, const char *wpath, char *pathroute, ssize_t pathsize,
    char *clientid_buf, int clientid_cb, char *pathid_buf, int pathid_cb, char *route_buf, int route_cb)
{
    char *pathid;
    int wd, pathlen;
    int wpath_wd = inotifytools_wd_from_filename(wpath);

    if (wpath_wd < 0) {
        LOGGER_FATAL("should never run to this! bad wpath wd");
        return (-1);
    } else {
        wd = wpath_wd;
        pathid = 0;
    }

    while (! pathid) {
        const char *wdpath;

        // 取得 wd 对应的全路径
        wdpath = (wd == wpath_wd? wpath : inotifytools_filename_from_wd(wd));
        if (! wdpath) {
            LOGGER_FATAL("should never run to this! inotifytools_filename_from_wd fail");
            break;
        }

        pathlen = strlen(wdpath);
        if (pathlen >= pathsize) {
            LOGGER_FATAL("should never run to this! path is too long");
            break;
        }

#ifdef XSYNC_USE_STATIC_PATHID_TABLE
        if ( wd < XSYNC_WATCH_PATHID_MAX ) {
            // 取得 wd 对应的 pathid
            pathid = client->wd_pathid_table[wd];
        }
#else
        do {
            red_black_node_t *node;
            struct wd_pathid_t wdpbuf;

            wdpbuf.wd = wd;
            node = rbtree_find(&client->wd_pathid_rbtree, &wdpbuf);
            if (node) {
                pathid = ((struct wd_pathid_t *) node->object)->pathid;
            }
        } while (0);
#endif

        if (pathid) {
            // pathid => wdpath
            LOGGER_DEBUG("found pathid (%s => %s)", pathid, wdpath);

            snprintf(clientid_buf, clientid_cb, "%s", client->clientid);
            snprintf(pathid_buf, pathid_cb, "%s", pathid);

            pathlen = snprintf(route_buf, route_cb, "%s", wpath + pathlen);
            if (pathlen >= route_cb) {
                LOGGER_FATAL("should never run to this! bad wpath: %s", wpath);
                break;
            }

            // 成功返回: >= 0
            return pathlen;
        }

        // 复制路径并去掉路径结尾 '/' 字符
        memcpy(pathroute, wdpath, pathlen);
        pathroute[pathlen--] = '\0';

        // 继续沿路径向上查找
        wd = -1;
        while (wd < 0 && pathlen-- > 0) {
            char *p = pathroute + pathlen;

            if (pathlen == 0) {
                break;
            }

            if (*p++ == '/') {
                *p = '\0';
                pathlen = p - pathroute;

                // 先得到当前路径的父目录: pathroute, 然后得到父目录的 wd
                wd = inotifytools_wd_from_filename(pathroute);
            }
        }

        if (pathlen == 0) {
            // 到路径结束也没有发现 wd
            LOGGER_FATAL("should never run to this! bad wpath: %s", wdpath);
            break;
        }
    }

    // 失败返回
    return (-1);
}


__no_warning_unused(static)
int client_init_watch_path (const char *path, int pathlen, struct mydirent *myent, void *arg1, void *arg2)
{
    XS_client client = (XS_client) arg1;

    assert(arg2 == 0);

    if (myent->isdir) {
        if (myent->islnk) {
            char *abspath = realpath(path, client->buffer);

            if (abspath) {
                // 目录名必须以 '/' 结尾
                slashpath(client->buffer, sizeof(client->buffer));

                LOGGER_INFO("inotify add wpath success: (%s => %s)", myent->ent.d_name, abspath);

                __inotifytools_lock();
                {
                    if (! inotifytools_watch_recursively(abspath, INOTI_EVENTS_MASK, on_inotify_add_wpath, client)) {
                        // 添加目录监视失败
                        LOGGER_ERROR("inotify add wpath fail: (%s => %s)", myent->ent.d_name, abspath);

                        __inotifytools_unlock();
                        return (-4);
                    } else {
                        // 添加目录监视成功
                        int wd_pathid = inotifytools_wd_from_filename(abspath);

#ifdef XSYNC_USE_STATIC_PATHID_TABLE
                        if (wd_pathid < 0 || wd_pathid >= XSYNC_WATCH_PATHID_MAX) {
                            LOGGER_ERROR("too many watch pathid(wd=%d) in table. see XSYNC_WATCH_PATHID_MAX (=%d) in client.mk", wd_pathid, XSYNC_WATCH_PATHID_MAX);

                            __inotifytools_unlock();
                            return (-4);
                        } else {
                            int len = strlen(myent->ent.d_name);

                            char *pathid = client->wd_pathid_table[wd_pathid];
                            if (pathid) {
                                // 如果已经存在则先删除
                                free(pathid);
                            }

                            pathid = malloc(len + 1);
                            memcpy(pathid, myent->ent.d_name, len);
                            pathid[len] = '\0';

                            client->wd_pathid_table[wd_pathid] = pathid;
                        }
#else
                        if (wd_pathid < 0 || rbtree_size(&client->wd_pathid_rbtree) >= XSYNC_WATCH_PATHID_MAX) {
                            LOGGER_ERROR("too many watch pathid(wd=%d) in tree. see XSYNC_WATCH_PATHID_MAX (=%d) in client.mk", wd_pathid, XSYNC_WATCH_PATHID_MAX);

                            __inotifytools_unlock();
                            return (-4);
                        } else {
                            int is_new_node;
                            red_black_node_t * node;
                            int len = strlen(myent->ent.d_name);
                            struct wd_pathid_t * wdpObject = (struct wd_pathid_t *) mem_alloc(1, sizeof(*wdpObject) + len + 1);

                            wdpObject->wd = wd_pathid;
                            wdpObject->len = len;
                            memcpy(wdpObject->pathid, myent->ent.d_name, len + 1);

                            node = rbtree_insert_unique(&client->wd_pathid_rbtree, (void *) wdpObject, &is_new_node);
                            if (! is_new_node) {
                                // 如果已经存在则先删除
                                void * object = node->object;
                                rbtree_remove_at(&client->wd_pathid_rbtree, node);
                                free(object);

                                // 再次插入节点
                                rbtree_insert_unique(&client->wd_pathid_rbtree, (void *) wdpObject, &is_new_node);
                                if (! is_new_node) {
                                    LOGGER_ERROR("should never run to this! see rbtree_insert_unique() in red_black_tree.c");

                                    __inotifytools_unlock();
                                    return (-4);
                                }
                            }
                        }
#endif
                    }
                }
                __inotifytools_unlock();
            } else {
                LOGGER_WARN("realpath error(%d): %s - (%s)", errno, strerror(errno), path);
            }
        } else {
            LOGGER_WARN("watch path not a link: %s", path);
        }
    }

    // continue to next
    return 1;
}


/**
 * 根据 watch 目录初始化
 *
 */
XS_RESULT XS_client_conf_from_watch (XS_client client, const char *watch_root)
{
    int err, len;

    char pathbuf[PATH_MAX];

    if (watch_root) {
        err = getfullpath(watch_root, client->watch_config, sizeof(client->watch_config) - 1);
        if (err != 0) {
            LOGGER_ERROR("bad watch root: %s", watch_root);
            return XS_ERROR;
        }

        len = slashpath(client->watch_config, sizeof(client->watch_config));

        if (! strncmp(watch_root, client->watch_config, len - 1)) {
            // 绝对路径
            LOGGER_INFO("init watch root: %s", watch_root);
        } else {
            // 符号链接路径
            LOGGER_INFO("init watch root: %s -> %s", watch_root, client->watch_config);
        }

        /* initialize lua context */
        snprintf(pathbuf, sizeof(pathbuf), "%spath-filter.lua", client->watch_config);

        if (access(pathbuf, F_OK|R_OK|X_OK) == 0) {
            LOGGER_INFO("loading lua script: %s", pathbuf);

            if (LuaCtxNew(pathbuf, LUACTX_THREAD_MODE_MULTI, &client->luactx) != LUACTX_SUCCESS) {
                LOGGER_FATAL("LuaCtxNew fail");
                return XS_ERROR;
            }
        } else {
            LOGGER_WARN("file access error(%d): %s (%s)", errno, strerror(errno), pathbuf);
        }
    }

    err = listdir(client->watch_config, pathbuf, sizeof(pathbuf), (listdir_callback_t) client_init_watch_path, (void*) client, 0);

    return (err == 0? XS_SUCCESS : XS_ERROR);
}


__no_warning_unused(static)
int list_server_cb (mxml_node_t *server_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(server_node, "sid", 0, attr);
    xmlconf_element_attr_read(server_node, "host", 0, attr);
    xmlconf_element_attr_read(server_node, "port", 0, attr);
    xmlconf_element_attr_read(server_node, "magic", 0, attr);

    return 1;
}


__no_warning_unused(static)
int list_watch_path_cb (mxml_node_t *wp_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(wp_node, "pathid", 0, attr);
    xmlconf_element_attr_read(wp_node, "fullpath", 0, attr);

    return 1;
}


/**
 * https://www.systutorials.com/docs/linux/man/3-mxml/
 */
extern XS_RESULT XS_client_conf_save_xml (XS_client client, const char * config_file)
{
    //TODO:

    return XS_ERROR;
}


// 只能初始化一次
//
XS_RESULT XS_client_conf_from_xml (XS_client client, const char *config_xml)
{
    int len;

    FILE *fp;
    const char * attr;

    mxml_node_t *xml;
    mxml_node_t *root;
    mxml_node_t *node;

    if (config_xml) {
        if (strcmp(strrchr(config_xml, '.'), ".xml")) {
            LOGGER_ERROR("config file end with not '.xml': %s", config_xml);
            return XS_E_PARAM;
        }

        if (strstr(strrchr(config_xml, '/'), XSYNC_CLIENT_APPNAME) !=  strrchr(config_xml, '/') + 1) {
            LOGGER_ERROR("config file start without '%s': %s", XSYNC_CLIENT_APPNAME, config_xml);
            return XS_E_PARAM;
        }

        len = (int) strlen(config_xml);
        if (len >= sizeof(client->watch_config)) {
            LOGGER_ERROR("config xml name too long (more than 255): %s", config_xml);
            return XS_E_PARAM;
        }

        memcpy(client->watch_config, config_xml, len + 1);
    }

    fp = fopen(client->watch_config, "r");
    if (! fp) {
        LOGGER_ERROR("fopen error(%d): %s. (%s)", errno, strerror(errno), client->watch_config);
        return XS_ERROR;
    }

    xml = mxmlLoadFile(0, fp, MXML_TEXT_CALLBACK);
    if (! xml) {
        LOGGER_ERROR("mxmlLoadFile error. (%s)", client->watch_config);
        fclose(fp);
        return XS_ERROR;
    }

    xmlconf_find_element_node(xml, "xs:"XSYNC_CLIENT_APPNAME"-conf", root);
    {
        xmlconf_element_attr_check(root, "xmlns:xs", XSYNC_CLIENT_XMLNS, attr);
        xmlconf_element_attr_check(root, "copyright", XSYNC_COPYRIGHT, attr);
        xmlconf_element_attr_check(root, "version", XSYNC_CLIENT_VERSION, attr);
    }

    xmlconf_find_element_node(root, "xs:application", node);
    {
        xmlconf_element_attr_read(node, "clientid", 0, attr);
        xmlconf_element_attr_read(node, "threads", 0, attr);
        xmlconf_element_attr_read(node, "queues", 0, attr);
    }

    xmlconf_find_element_node(root, "xs:server-list", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:server", (xmlconf_list_node_cb_t) list_server_cb, (void *) client);
    }

    xmlconf_find_element_node(root, "xs:watch-path-list", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:watch-path", (xmlconf_list_node_cb_t) list_watch_path_cb, (void *) client);
    }

    mxmlDelete(xml);
    fclose(fp);
    return XS_SUCCESS;

error_exit:
    mxmlDelete(xml);
    fclose(fp);
    return XS_ERROR;
}
