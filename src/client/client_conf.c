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
 * @version: 0.1.5
 *
 * @create: 2018-01-26
 *
 * @update: 2018-10-15 18:04:33
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

    // TODO: kafka config from lua
    
    // '/home/root1/Workspace/github.com/pepstack/xsync/target/libkafkatools.so.1'
    client->apphome[client->apphome_len] = 0;
    strcat(client->apphome, "libkafkatools.so.1");

    LOGGER_DEBUG("[thread-%d] create kafka producer. (%s)", threadid, client->apphome);
    if (kafka_producer_api_create(&perdata->kt_producer_api, client->apphome)) {
        LOGGER_FATAL("kafka_producer_api_create fail");

        LuaCtxFree(&perdata->luactx);

        free(perdata);
        exit(-1);
    }

    /* 没有引用计数 */
    perdata->xclient = (void *) client;

    /* 服务器数量 */
    perdata->server_conns[0] = (XS_server_conn) int_cast_to_pv(servers);

    /* 线程 id: 1 based */
    perdata->threadid = threadid;

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

    if (__interlock_get(&client->size_paths)) {
        __interlock_set(&client->size_paths, 0);

        free(client->paths);
        client->paths = 0;
    }

    LOGGER_TRACE("clean event_rbtree");
    threadlock_destroy(&client->rbtree_lock);
    do {
        // TODO:
        rbtree_clean(&client->event_rbtree);
    } while (0);

    LOGGER_TRACE("clean wpath_hmap");
    threadlock_destroy(&client->wpath_lock);
    do {
        struct hlist_node *hp, *hn;
        int hash;

        for (hash = 0; hash <= XSYNC_HASHMAP_MAX_LEN; hash++) {
            hlist_for_each_safe(hp, hn, &client->wpath_hmap[hash]) {
                XS_watch_path wp = hlist_entry(hp, struct xs_watch_path_t, i_hash);
                XS_watch_path_release(&wp);
            }
        }
    } while (0);

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&client->condition);

    LuaCtxFree(&client->luactx);

    LOGGER_TRACE("~XS_client(%p)", client);
    free(client);
}


int lscb_init_watch_path (const char *path, int pathlen, struct mydirent *myent, void *arg1, void *arg2)
{
    XS_client client = (XS_client) arg1;

    assert(arg2 == 0);

    if (myent->isdir) {
        if (myent->islnk) {
            char *abspath = realpath(path, client->buffer);

            if (abspath) {
                XS_watch_path  wp;

                LOGGER_INFO("pathid:%s => (%s)", myent->ent.d_name, abspath);

                if (XS_watch_path_create(myent->ent.d_name, abspath, INOTI_EVENTS_MASK, &wp) == XS_SUCCESS) {
                    // 添加到监视目录
                    if (! XS_client_add_watch_path(client, wp)) {
                        XS_watch_path_release(&wp);
                        return (-4);
                    }
                }
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


__no_warning_unused(static)
int watch_path_set_xmlnode_cb (XS_watch_path wp, void *data)
{
    int sid_max;

    mxml_node_t *watchpathsNode = (mxml_node_t *) data;

    mxml_node_t *watch_path_node;

    watch_path_node = mxmlNewElement(watchpathsNode, "xs:watch-path");

    mxmlElementSetAttrf(watch_path_node, "pathid", "%s", wp->pathid);

    mxmlElementSetAttrf(watch_path_node, "fullpath", "%s", wp->fullpath);

    sid_max = wp->sid_masks[0];

    return sid_max;
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

    if (config_xml && ! __interlock_get(&client->size_paths)) {
        int size_paths;
        char *paths;

        int offs_config_xml;

        if (strcmp(strrchr(config_xml, '.'), ".xml")) {
            LOGGER_ERROR("config file end with not '.xml': %s", config_xml);
            return XS_E_PARAM;
        }

        if (strstr(strrchr(config_xml, '/'), XSYNC_CLIENT_APPNAME) !=  strrchr(config_xml, '/') + 1) {
            LOGGER_ERROR("config file start without '%s': %s", XSYNC_CLIENT_APPNAME, config_xml);
            return XS_E_PARAM;
        }

        len = (int) strlen(config_xml);

        /* paths =  'w'+config_xml+'\0'+path_filter+'\0'+event_task+'\0'*/
        size_paths = 1 + (len + 1); //TODO: +   (len + 15) +      (len + 14);

        paths = mem_alloc(1, sizeof(char) * size_paths);
        paths[0] = 'C';

        offs_config_xml = 1;

        memcpy(paths + offs_config_xml, config_xml, len);

        client->offs_watch_root = offs_config_xml;

        client->paths = paths;
        __interlock_set(&client->size_paths, size_paths);
    }

    fp = fopen(client->paths + client->offs_watch_root, "r");
    if (! fp) {
        LOGGER_ERROR("fopen error(%d): %s. (%s)", errno, strerror(errno), client->paths + client->offs_watch_root);
        return XS_ERROR;
    }

    xml = mxmlLoadFile(0, fp, MXML_TEXT_CALLBACK);
    if (! xml) {
        LOGGER_ERROR("mxmlLoadFile error. (%s)", client->paths + client->offs_watch_root);
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


/**
 * 根据 watch 目录初始化
 *
 */
XS_RESULT XS_client_conf_from_watch (XS_client client, const char *watch_root)
{
    int err, len;
    char pathbuf[XSYNC_PATH_MAXSIZE];

    if (watch_root && ! __interlock_get(&client->size_paths)) {
        int size_paths;
        char *paths;

        int offs_watch_root;

        err = getfullpath(watch_root, pathbuf, sizeof(pathbuf));
        if (err != 0) {
            LOGGER_ERROR("bad watch root: %s", pathbuf);
            return XS_ERROR;
        }

        len = (int) strlen(pathbuf);
        if ( pathbuf[len - 1] != '/' ) {
            pathbuf[len++] = '/';
            pathbuf[len] = '\0';
        }

        /* paths =  'W' + watch_root + '\0' */
        size_paths = 1 + (len + 1);

        paths = mem_alloc(1, sizeof(char) * size_paths);
        paths[0] = 'W';

        offs_watch_root = 1;

        memcpy(paths + offs_watch_root, pathbuf, len);

        if (! strncmp(watch_root, pathbuf, len - 1)) {
            // 绝对路径
            LOGGER_INFO("init watch root: %s", paths + offs_watch_root);
        } else {
            // 符号链接路径
            LOGGER_INFO("init watch root: %s -> %s", watch_root, paths + offs_watch_root);
        }

        client->offs_watch_root = offs_watch_root;

        /* initialize lua context */
        len = snprintf(pathbuf, sizeof(pathbuf), "%spath-filter.lua", paths + client->offs_watch_root);

        if (access(pathbuf, F_OK|R_OK|X_OK) == 0) {
            LOGGER_INFO("loading lua script: %s", pathbuf);

            if (LuaCtxNew(pathbuf, LUACTX_THREAD_MODE_MULTI, &client->luactx) != LUACTX_SUCCESS) {
                LOGGER_FATAL("LuaCtxNew fail");
                return XS_ERROR;
            }
        } else {
            LOGGER_WARN("file access error(%d): %s (%s)", errno, strerror(errno), pathbuf);
        }

        client->paths = paths;
        __interlock_set(&client->size_paths, size_paths);
    }

    err = listdir(watch_root_path(client), pathbuf, sizeof(pathbuf), (listdir_callback_t) lscb_init_watch_path, (void*) client, 0);

    return (err == 0? XS_SUCCESS : XS_ERROR);
}
