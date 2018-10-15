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
 * @file: watch_path.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.4
 *
 * @create: 2018-01-29
 *
 * @update: 2018-09-21 16:14:23
 */

#include "client_api.h"

#include "server_conn.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "client_conf.h"

#include "../common/common_util.h"


static inline void xs_watch_path_delete (void *pv)
{
    XS_watch_path wp = (XS_watch_path) pv;

    // 从 event_hmap 清除自身
    hlist_del(&wp->i_hash);

    LOGGER_TRACE("~xpath=%p", wp);
    free(pv);
}


XS_RESULT XS_watch_path_create (const char *pathid, const char * fullpath, uint32_t events_mask, XS_watch_path parent, XS_watch_path * outwp)
{
    int sid, pathlen, cbsize;

    XS_watch_path wpath;

    *outwp = 0;

    pathlen = (int) strlen(fullpath);

    // 8 字节对齐
    cbsize = ((pathlen + sizeof(XS_watch_path)) / sizeof(XS_watch_path)) * sizeof(XS_watch_path);

    wpath = (XS_watch_path) mem_alloc(1, sizeof(xs_watch_path_t) + sizeof(char) * cbsize);

    // inotify 监视掩码: mask of events
    wpath->events_mask = events_mask;

    // path id
    strncpy(wpath->pathid, pathid, XSYNC_CLIENTID_MAXLEN);
    wpath->pathid[XSYNC_CLIENTID_MAXLEN] = 0;

    // 全路径
    memcpy(wpath->fullpath, fullpath, pathlen);
    wpath->fullpath[pathlen] = '\0';
    wpath->pathlen = pathlen;

    // 初始化 sid_masks: sid_masks[0] => sid_max
    for (sid = 0; sid <= XSYNC_SERVER_MAXID; sid++) {
        wpath->sid_masks[sid] = 0;
    }

    wpath->parent_wp = parent;

    LOGGER_DEBUG("wpath(%p): %s => %s", wpath, wpath->pathid, wpath->fullpath);

    // 增加引用计数
    *outwp = (XS_watch_path) RefObjectInit(wpath);

    return XS_SUCCESS;
}


XS_VOID XS_watch_path_release (xs_watch_path_t ** wp)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) wp, xs_watch_path_delete);
}