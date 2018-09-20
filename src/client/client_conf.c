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
 * @version: 0.0.5
 *
 * @create: 2018-01-26
 *
 * @update: 2018-09-10 11:48:26
 */

#include "client_api.h"

#include "client_conf.h"

#include "../xsync-xmlconf.h"
#include "../common/readconf.h"

#include "../common/common_util.h"


extern void xs_client_delete (void *pv)
{
    int i, sid;

    XS_client client = (XS_client) pv;

    LOGGER_TRACE("inotifytools_cleanup()");
    inotifytools_cleanup();

    if (client->pool) {
        LOGGER_DEBUG("threadpool_destroy");
        threadpool_destroy(client->pool, 0);
    }

    if (client->thread_args) {
        perthread_data * perdata;
        int sid_max;

        for (i = 0; i < client->threads; ++i) {
            perdata = client->thread_args[i];
            client->thread_args[i] = 0;

            sid_max = (int)(long)(void *)perdata->server_conns[0];

            for (sid = 1; sid <= sid_max; sid++) {
                XS_server_conn conn = perdata->server_conns[sid];

                if (conn) {
                    perdata->server_conns[sid] = 0;

                    XS_server_conn_release(&conn);
                }
            }

            free(perdata);
        }

        free(client->thread_args);
    }

    if (__interlock_get(&client->size_paths)) {
        __interlock_set(&client->size_paths, 0);

        free(client->paths);
        client->paths = 0;      
    }

    //??DEL
    client_clear_entry_map(client);

    //??DEL
    XS_client_clear_watch_paths(client);

    LOGGER_TRACE("threadlock_destroy event_map_lock");
    threadlock_destroy(&client->event_map_lock);

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&client->condition);

    LOGGER_TRACE("%p", client);

    free(client);
}


__no_warning_unused(static)
XS_RESULT client_read_filter_file (XS_client client, const char * filter_file, int sid, int filter_type)
{
    int ret, lineno, err;
    char * endpath;

    char pathid_file[XSYNC_BUFSIZE];
    char resolved_path[XSYNC_BUFSIZE];

    FILE * fp = 0;

    ret = snprintf(pathid_file, sizeof(pathid_file), "%s", filter_file);
    if (ret < 0) {
        LOGGER_ERROR("snprintf error(%d): %s", errno, strerror(errno));
        return (-1);
    }

    if (ret >= sizeof(pathid_file)) {
        LOGGER_ERROR("insufficent buffer for file: %s", filter_file);
        return (-2);
    }

    endpath = strrchr(pathid_file, '/');
    if (! endpath) {
        LOGGER_ERROR("invalid filter file: %s", filter_file);
        return (-3);
    }

    endpath++;
    *endpath = 0;

    assert(filter_type == XS_path_filter_type_excluded || filter_type == XS_path_filter_type_included);

    fp = fopen(filter_file, "r");
    if (! fp) {
        LOGGER_ERROR("fopen error(%d): %s", errno, strerror(errno));
        return (-4);
    }

    err = -5;

    do {
        char line[XSYNC_BUFSIZE];
        char * p, * filter;

        char * sid_filter = strrchr(filter_file, '/');
        sid_filter++;

        lineno = 0;

        while (fgets(line, sizeof(line), fp)) {
            lineno++;

            p = strrchr(line, '\n');
            if (p) {
                *p = 0;
            }
            p = strrchr(line, '\r');
            if (p) {
                *p = 0;
            }

            if (line[0] == '#') {
                // 忽略注释行
                LOGGER_TRACE("[%s - line: %d] %s", sid_filter, lineno, line);
                continue;
            }

            filter = strchr(line, '/');
            if (! filter) {
                LOGGER_ERROR("[%s - line: %d] %s", sid_filter, lineno, line);
                goto error_exit;
            }

            LOGGER_DEBUG("[%s - line: %d] %s", sid_filter, lineno, line);

            *filter++ = 0;

            *endpath = 0;
            strcat(pathid_file, line);

            char *fullpath = realpath(pathid_file, resolved_path);

            if (fullpath) {
                XS_watch_path wp = 0;

                if (XS_client_find_watch_path(client, fullpath, &wp)) {
                    XS_path_filter pf = 0;

                    if (filter_type == XS_path_filter_type_excluded) {
                        pf = XS_watch_path_get_excluded_filter(wp, sid);
                    } else {
                        assert(filter_type == XS_path_filter_type_included);
                        pf = XS_watch_path_get_included_filter(wp, sid);
                    }

                    LOGGER_DEBUG("pathid=%s, pathid-file=%s", line, pathid_file);

                    ret = XS_path_filter_add_patterns(pf, filter);
                    if (ret != 0) {
                        err = -6;
                        goto error_exit;
                    }
                } else {
                    err = -7;
                    LOGGER_WARN("not found path: (pathid=%s, pathid-file=%s)", line, pathid_file);
                    goto error_exit;
                }
            } else {
                err = -8;
                LOGGER_ERROR("realpath error(%d): %s. (%s)", errno, strerror(errno), pathid_file);
                goto error_exit;
            }
        }

        // success return here
        fclose(fp);
        return 0;
    } while(0);

error_exit:
    fclose(fp);
    return err;
}


int lscb_add_watch_path (const char *path, int pathlen, struct dirent *ent, void *arg1, void *arg2)
{
    char pathbuf[XSYNC_PATH_MAXSIZE];

    XS_client client = (XS_client) arg1;

    assert(arg2 == 0);

    if (isdir(path)) {
        char *abspath = realpath(path, pathbuf);

        if (abspath) {
            /**
             * abspath = '/path/to/watch-local/logs'
             * path   = '/path/to/watch/logs'
             * pathid = 'logs'
             */
            XS_watch_path  wp;
            char *pathid = strrchr(path, '/') + 1;

            LOGGER_INFO("pathid=%s -> (%s)", pathid, abspath);

            if (XS_watch_path_create(pathid, abspath, INOTI_EVENTS_MASK, 0, &wp) == XS_SUCCESS) {
                /*
                // route 是服务端的路径
                len = XS_watch_path_get_pathid_route(wp, pathbuf);

                pathbuf[len] = 0;

                LOGGER_DEBUG("path-route(%s): %s -> (%s)", pathbuf, pathid, abspath);

                */

                if (! XS_client_add_watch_path(client, wp)) {
                    XS_watch_path_release(&wp);
                    return (-4);
                }
            }
        } else {
            LOGGER_WARN("realpath error(%d): %s - (%s)", errno, strerror(errno), path);
        }
    }

    // continue to next
    return 1;
}


int lscb_init_watch_path (const char * path, int pathlen, struct dirent *ent, XS_client client, XS_watch_path parent)
{
    int  lnk, sid, err;

    char resolved_path[XSYNC_PATH_MAXSIZE];

    lnk = fileislink(path, 0, 0);

    if (! isdir(path)) {
        char sid_table[10];
        char sid_included[20];
        char sid_excluded[20];

        char * sidfile = realpath(path, resolved_path);

        if (! sidfile) {
            LOGGER_ERROR("realpath error(%d): %s - (%s)", errno, strerror(errno), path);
            return (-4);
        }

        for (sid = 1; sid < XSYNC_SERVER_MAXID; sid++) {
            snprintf(sid_table, sizeof(sid_table), "%d", sid);
            snprintf(sid_included, sizeof(sid_included), "%d.included", sid);
            snprintf(sid_excluded, sizeof(sid_excluded), "%d.excluded", sid);

            if (! strcmp(sid_table, ent->d_name)) {
                // 设置 servers_opts
                assert(client->servers_opts[sid].magic == 0);

                if (lnk) {
                    LOGGER_INFO("init sid=%d from: %s -> %s", sid, path, sidfile);
                } else {
                    LOGGER_INFO("init sid=%d from: %s", sid, path);
                }

                err = XS_server_opts_init(&client->servers_opts[sid], sid, sidfile);
                if (err) {
                    // 有错误, 中止运行
                    return 0;
                }

                if (client->servers_opts[0].sidmax < sid) {
                    client->servers_opts[0].sidmax = sid;
                }
            }

            if (! strcmp(sid_included, ent->d_name)) {
                // 读 sid.included 文件, 设置 sid 包含哪些目录文件
                err = client_read_filter_file(client, path, sid, XS_path_filter_type_included);

                if (! err) {
                    LOGGER_INFO("read included filter: %s", path);
                } else {
                    LOGGER_ERROR("read included filter: %s", path);

                    // 有错误, 中止运行
                    return 0;
                }
            } else if (! strcmp(sid_excluded, ent->d_name)) {
                // 读 sid.excluded 文件, 设置 sid 排除哪些目录文件
                err = client_read_filter_file(client, path, sid, XS_path_filter_type_excluded);

                if (! err) {
                    LOGGER_INFO("read excluded filter: %s", path);
                } else {
                    LOGGER_ERROR("read excluded filter: %s", path);

                    // 有错误, 中止运行
                    return 0;
                }
            }
        }
    }

    // continue to next
    return 1;
}


int watch_path_set_sid_masks_cb (XS_watch_path wp, void * data)
{
    int sid;

    XS_client client = (XS_client) data;

    wp->sid_masks[0] = client->servers_opts[0].sidmax;

    // TODO: 设置 watch_path 的 sid_masks
    for (sid = 1; sid <= wp->sid_masks[0]; sid++) {
        // wp->included_filters[sid]

        // wp->excluded_filters[sid]

        // TODO:
        wp->sid_masks[sid] = 0xfffff;

        LOGGER_WARN("pathid: %s (sid=%d, masks=%d)", wp->pathid, sid, 0xfffff);
    }

    return 1;
}


__no_warning_unused(static)
int watch_path_set_xmlnode_cb (XS_watch_path wp, void *data)
{
    int sid, sid_max;

    mxml_node_t *watchpathsNode = (mxml_node_t *) data;

    mxml_node_t *watch_path_node;
    mxml_node_t *included_filters_node;
    mxml_node_t *excluded_filters_node;
    mxml_node_t *filter_node;

    watch_path_node = mxmlNewElement(watchpathsNode, "xs:watch-path");

    mxmlElementSetAttrf(watch_path_node, "pathid", "%s", wp->pathid);

    mxmlElementSetAttrf(watch_path_node, "fullpath", "%s", wp->fullpath);

    included_filters_node = mxmlNewElement(watch_path_node, "xs:included-filters");
    excluded_filters_node = mxmlNewElement(watch_path_node, "xs:excluded-filters");

    sid_max = wp->sid_masks[0];

    for (sid = 1; sid <= sid_max; sid++) {
        XS_path_filter filter = wp->included_filters[sid];
        if (filter) {
            filter_node = mxmlNewElement(included_filters_node, "xs:filter-patterns");

            xs_filter_set_xmlnode(filter, filter_node);
        }
    }

    for (sid = 1; sid <= sid_max; sid++) {
        XS_path_filter filter = wp->excluded_filters[sid];
        if (filter) {
            filter_node = mxmlNewElement(excluded_filters_node, "xs:filter-patterns");

            xs_filter_set_xmlnode(filter, filter_node);
        }
    }

    return 1;
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
int list_sub_pattern_cb (mxml_node_t *subpattern_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(subpattern_node, "regex", 0, attr);
    xmlconf_element_attr_read(subpattern_node, "script", 0, attr);

    return 1;
}


__no_warning_unused(static)
int list_pattern_cb (mxml_node_t *pattern_node, XS_client client)
{
    const char * attr;

    xmlconf_element_attr_read(pattern_node, "regex", 0, attr);
    xmlconf_element_attr_read(pattern_node, "script", 0, attr);

    xmlconf_list_mxml_nodes(pattern_node, "xs:sub-pattern", (xmlconf_list_node_cb_t) list_sub_pattern_cb, (void *) client);

    return 1;
}


__no_warning_unused(static)
int list_included_filter_cb (mxml_node_t *filter_node, XS_client client)
{
    const char * attr;
    xmlconf_element_attr_read(filter_node, "sid", 0, attr);
    xmlconf_element_attr_read(filter_node, "size", 0, attr);

    return 1;
}


__no_warning_unused(static)
int list_excluded_filter_cb (mxml_node_t *filter_node, XS_client client)
{
    const char * attr;
    xmlconf_element_attr_read(filter_node, "sid", 0, attr);
    xmlconf_element_attr_read(filter_node, "size", 0, attr);

    xmlconf_list_mxml_nodes(filter_node, "xs:pattern", (xmlconf_list_node_cb_t) list_pattern_cb, (void *) client);

    return 1;
}


__no_warning_unused(static)
int list_watch_path_cb (mxml_node_t *wp_node, XS_client client)
{
    const char * attr;
    mxml_node_t * node;

    xmlconf_element_attr_read(wp_node, "pathid", 0, attr);
    xmlconf_element_attr_read(wp_node, "fullpath", 0, attr);

    xmlconf_find_element_node(wp_node, "xs:included-filters", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:filter-patterns", (xmlconf_list_node_cb_t) list_included_filter_cb, (void *) client);

    }

    xmlconf_find_element_node(wp_node, "xs:excluded-filters", node);
    {
        xmlconf_list_mxml_nodes(node, "xs:filter-patterns", (xmlconf_list_node_cb_t) list_excluded_filter_cb, (void *) client);
    }

    return 1;

error_exit:
    return 0;
}


// MXML_WS_BEFORE_OPEN, MXML_WS_AFTER_OPEN, MXML_WS_BEFORE_CLOSE, MXML_WS_AFTER_CLOSE
//
static const char * whitespace_cb(mxml_node_t *node, int where)
{
    const char *name;

    static char wrapline[] = "\n";

    if (node->type != MXML_ELEMENT) {
        return 0;
    }

    if (node->value.element.name == 0) {
        return 0;
    }

    name = node->value.element.name;

    if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) {
        if (name[0] == 'x' && name[1] == 's' && name[2] == ':') {
            if (! strcmp(name, "xs:application")) {
                return "\n\t";
            }
            if (! strcmp(name, "xs:server-list")) {
                return "\n\t";
            }
            if (! strcmp(name, "xs:watch-path-list")) {
                return "\n\t";
            }

            if (! strcmp(name, "xs:server")) {
                return "\n\t\t";
            }
            if (! strcmp(name, "xs:watch-path")) {
                return "\n\t\t";
            }

            if (! strcmp(name, "xs:included-filters")) {
                return "\n\t\t\t";
            }
            if (! strcmp(name, "xs:excluded-filters")) {
                return "\n\t\t\t";
            }

            if (! strcmp(name, "xs:filter-patterns")) {
                return "\n\t\t\t\t";
            }

            if (! strcmp(name, "xs:pattern")) {
                return "\n\t\t\t\t\t";
            }

            if (! strcmp(name, "xs:sub-pattern")) {
                return "\n\t\t\t\t\t\t";
            }

            return wrapline;
        }
    }

    // 如果不需要添加空白字符则返回 0
    return 0;
}


/**
 * https://www.systutorials.com/docs/linux/man/3-mxml/
 */
extern XS_RESULT XS_client_conf_save_xml (XS_client client, const char * config_file)
{
    int sid;

    mxml_node_t *xml;
    mxml_node_t *node;
    mxml_node_t *root;
    mxml_node_t *configNode;
    mxml_node_t *serversNode;
    mxml_node_t *watchpathsNode;

    if (strcmp(strrchr(config_file, '.'), ".xml")) {
        LOGGER_ERROR("config file end with not '.xml': %s", config_file);
        return XS_E_PARAM;
    }

    if (strstr(strrchr(config_file, '/'), XSYNC_CLIENT_APPNAME) !=  strrchr(config_file, '/') + 1) {
        LOGGER_ERROR("config file start with not '%s': %s", XSYNC_CLIENT_APPNAME, config_file);
        return XS_E_PARAM;
    }

    xml = mxmlNewXML("1.0");
    if (! xml) {
        LOGGER_ERROR("mxmlNewXML error: out of memory");
        return XS_ERROR;
    }

    mxmlSetWrapMargin(0);

    root = mxmlNewElement(xml, "xs:"XSYNC_CLIENT_APPNAME"-conf");

    // 添加名称空间, 在Firefox中看不到, 在Chrome中可以
    mxmlElementSetAttr(root, "xmlns:xs", XSYNC_CLIENT_XMLNS);
    mxmlElementSetAttr(root, "copyright", XSYNC_COPYRIGHT);
    mxmlElementSetAttr(root, "version", XSYNC_CLIENT_VERSION);
    mxmlElementSetAttr(root, "author", XSYNC_AUTHOR);

    configNode = mxmlNewElement(root, "xs:application");
    serversNode = mxmlNewElement(root, "xs:server-list");

    do {
        mxmlElementSetAttrf(configNode, "clientid", "%s", client->clientid);
        mxmlElementSetAttrf(configNode, "threads", "%d", client->threads);
        mxmlElementSetAttrf(configNode, "queues", "%d", client->queues);
    } while(0);

    for (sid = 1; sid <= XS_client_get_server_maxid(client); sid++) {
        xs_server_opts *server = XS_client_get_server_opts(client, sid);

        node = mxmlNewElement(serversNode, "xs:server");

        mxmlElementSetAttrf(node, "sid", "%d", sid);
        mxmlElementSetAttrf(node, "host", "%s", server->host);
        mxmlElementSetAttrf(node, "port", "%d", server->port);
        mxmlElementSetAttrf(node, "magic", "%d", server->magic);
    } while(0);

    watchpathsNode = mxmlNewElement(root, "xs:watch-path-list");

    XS_client_list_watch_paths(client, watch_path_set_xmlnode_cb, watchpathsNode);

    do {
        FILE * fp;

        fp = fopen(config_file, "w");

        if (fp) {
            mxmlSaveFile(xml, fp, whitespace_cb);

            mxmlDelete(xml);

            fclose(fp);

            LOGGER_INFO("saved config file: %s", config_file);

            return XS_SUCCESS;
        }

        LOGGER_ERROR("fopen error(%d): %s. (%s)", errno, strerror(errno), config_file);
    } while(0);

    // 必须删除整个 xml
    mxmlDelete(xml);

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

        /* TODO:
        int offs_path_filter;
        int offs_event_task;
        */

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

        /* TODO:
        offs_path_filter = offs_config_xml + (len + 1);
        offs_event_task = offs_path_filter + (len + 15);
        */

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


// 只能初始化一次
//
extern XS_RESULT XS_client_conf_from_watch (XS_client client, const char *watch_root)
{
    int err, len;
    char pathbuf[XSYNC_PATH_MAXSIZE];

    if (watch_root && ! __interlock_get(&client->size_paths)) {
        int size_paths;
        char *paths;

        int offs_watch_root;
        int offs_path_filter;
        int offs_event_task;

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

        /* paths =  'w'+watch_root+'\0'+path_filter+'\0'+event_task+'\0'*/
        size_paths = 1 + (len + 1) +   (len + 15) +      (len + 14);

        paths = mem_alloc(1, sizeof(char) * size_paths);
        paths[0] = 'W';

        offs_watch_root = 1;
        offs_path_filter = offs_watch_root + (len + 1);
        offs_event_task = offs_path_filter + (len + 15);

        memcpy(paths + offs_watch_root, pathbuf, len);

        if (! strncmp(watch_root, pathbuf, len - 1)) {
            // 绝对路径
            LOGGER_INFO("init watch root: %s", paths + offs_watch_root);
        } else {
            // 符号链接路径
            LOGGER_INFO("init watch root: %s -> %s", watch_root, paths + offs_watch_root);
        }

        client->offs_watch_root = offs_watch_root;

        // path-filter.sh
        len = snprintf(pathbuf, sizeof(pathbuf), "%spath-filter.sh", paths + client->offs_watch_root);

        if (access(pathbuf, F_OK|R_OK|X_OK) == 0) {
            client->offs_path_filter = offs_path_filter;

            memcpy(paths + client->offs_path_filter, pathbuf, len);

            LOGGER_INFO("access path-filter.sh ok. (%s)", paths + client->offs_path_filter);
        } else {
            LOGGER_ERROR("access path-filter.sh failed(%d): %s (%s)", errno, strerror(errno), pathbuf);
        }

        // event-task.sh
        len = snprintf(pathbuf, sizeof(pathbuf), "%sevent-task.sh", paths + client->offs_watch_root);

        if (access(pathbuf, F_OK|R_OK|X_OK) == 0) {
            client->offs_event_task = offs_event_task;

            memcpy(paths + client->offs_event_task, pathbuf, len);

            LOGGER_INFO("access event-task.sh ok. (%s)", paths + client->offs_event_task);
        } else {
            LOGGER_ERROR("access event-task.sh failed(%d): %s (%s)", errno, strerror(errno), pathbuf);
        }

        client->paths = paths;
        __interlock_set(&client->size_paths, size_paths);
    }

    err = listdir(watch_root_path(client), pathbuf, sizeof(pathbuf), (listdir_callback_t) lscb_add_watch_path, (void*) client, 0);

    return (err == 0? XS_SUCCESS : XS_ERROR);
}


/**
 * 必须在锁 event_map_lock 中调用
 */
extern int XS_client_prepare_watch_events (XS_client client, struct inotify_event *inevent, XS_watch_event events[XSYNC_SERVER_MAXID + 1])
{
    struct hlist_node *hp;

    int num_events = 0;
    XS_watch_event event = 0;

    int hash = BKDRHash2(inevent->name, XSYNC_WATCH_PATH_HASHMAX);

    hlist_for_each(hp, &client->event_map_hlist[hash]) {
        struct xs_watch_event_t *p = hlist_entry(hp, struct xs_watch_event_t, i_hash);

        if (p->namelen == inevent->len && ! strncmp(p->pathname, inevent->name, inevent->len)) {
            LOGGER_WARN("existing event(=%p): (task=%lld name=%s)", p, (long long) p->taskid, p->pathname);
            return 0;
        }
    }

    XS_watch_event_create(inevent, client, hash, 0, &event);

    hlist_add_head(&event->i_hash, &client->event_map_hlist[event->hash]);

    // events[0] 不属于任何 sid 的事件
    // events[1] 是 sid=1 的事件
    // events[2] 是 sid=2 的事件
    // events[maxsid] 是 sid=maxsid 的事件
    //
    events[num_events++] = event;

    return num_events;

    /*
    int events_maxsid = 0;


    int sid = 1;
        int maxsid = XS_client_get_server_maxid(client);

        for (; sid <= maxsid; sid++) {
            XS_watch_entry entry;

            events[sid] = 0;

            if (! client_find_watch_entry_inlock(client, sid, inevent->wd, inevent->name, &entry)) {
                // 没有发现 watch_entry
                // alway success for create watch entry
                XS_watch_entry_create(wp, sid, inevent->name, &entry);

                if (client_add_watch_entry_inlock(client, entry)) {
                    // 成功添加 watch_entry 到 client 的 entry_map 中, 添加到定时器. 增加 entry 计数
                    RefObjectRetain((void**) &entry);

                    // 设置多定时器: 清理 entry_map 事件
                    if (mul_timer_set_event(XSYNC_ENTRY_SESSION_TIMEOUT +
                            (wp->watch_wd & XSYNC_WATCH_PATH_HASHMAX),
                            XSYNC_ENTRY_SESSION_TIMEOUT,
                            MULTIMER_EVENT_INFINITE,
                            OnEventCheckEntrySession,
                            sid,
                            (void*) entry,
                            MULTIMER_EVENT_CB_BLOCK) > 0)
                    {
                        // XS_watch_event_create alway success
                        XS_watch_event event;

                        XS_watch_event_create(inevent->mask, client, entry, &event);

                        events[sid] = event;
                        events_maxsid = sid;
                    } else {
                        XS_watch_entry_release(&entry);
                        LOGGER_FATAL("should nerver run to this: mul_timer_set_event failed");
                    }
                } else {
                    XS_watch_entry_release(&entry);
                    LOGGER_FATAL("should nerver run to this: client_add_watch_entry_inlock failed");
                }
            } else if (XS_watch_entry_not_in_use(entry)) {
                // 发现存在 watch_entry, 且未被使用
                // XS_watch_event_create alway success
                XS_watch_event event;

                XS_watch_event_create(inevent->mask, client, entry, &event);

                events[sid] = event;
                events_maxsid = sid;
            }
        }
    }
    */

    return 0;
}

