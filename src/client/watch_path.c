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
 * @version: 0.0.7
 *
 * @create: 2018-01-29
 *
 * @update: 2018-08-10 18:11:59
 */

#include "client_api.h"

#include "server_conn.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/common_util.h"


static inline void free_watch_path (void *pv)
{
    XS_watch_path wp = (XS_watch_path) pv;

    // TODO: free included_filters, excluded_filters

    LOGGER_TRACE("~xpath=%p", wp);

    free(pv);
}


XS_RESULT XS_watch_path_create (const char * pathid, const char * fullpath, uint32_t events_mask, XS_watch_path * outwp)
{
    int sid;
    size_t cb;

    XS_watch_path wpath;

    *outwp = 0;

    cb = strlen(fullpath) + 1;

    wpath = (XS_watch_path) mem_alloc(1, sizeof(*wpath) + sizeof(char) * cb);

    // size of fullpath including '\0'
    wpath->pathsize = cb;

    // 必须初始化=-1
    wpath->watch_wd = -1;

    // inotify 监视掩码: mask of events
    wpath->events_mask = events_mask;

    // path id
    strncpy(wpath->pathid, pathid, XSYNC_CLIENTID_MAXLEN);
    wpath->pathid[XSYNC_CLIENTID_MAXLEN] = 0;

    // 全路径
    memcpy(wpath->fullpath, fullpath, cb);

    // 初始化 sid_masks: sid_masks[0] => sid_max
    for (sid = 0; sid <= XSYNC_SERVER_MAXID; sid++) {
        wpath->sid_masks[sid] = 0;
    }

    *outwp = (XS_watch_path) RefObjectInit(wpath);

    LOGGER_TRACE("path=%p (%s=>%s)", wpath, wpath->pathid, wpath->fullpath);

    return XS_SUCCESS;
}


XS_VOID XS_watch_path_release (xs_watch_path_t ** wp)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) wp, free_watch_path);
}


XS_path_filter XS_watch_path_get_included_filter (XS_watch_path wp, int sid)
{
    LOGGER_TRACE0();

    assert(sid > 0 && sid <= XSYNC_SERVER_MAXID);

    if (! wp->included_filters[sid]) {
        wp->included_filters[sid] = XS_path_filter_create(sid, XS_path_filter_patterns_block);
    }

    XS_path_filter filt = wp->included_filters[sid];
    assert(filt->sid == sid);

    return filt;
}


XS_path_filter XS_watch_path_get_excluded_filter (XS_watch_path wp, int sid)
{
    assert(sid > 0 && sid <= XSYNC_SERVER_MAXID);

    if (! wp->excluded_filters[sid]) {
        wp->excluded_filters[sid] = XS_path_filter_create(sid, XS_path_filter_patterns_block);
    }

    XS_path_filter filt = wp->excluded_filters[sid];
    assert(filt->sid == sid);

    return filt;
}


extern XS_RESULT XS_watch_path_sweep (XS_watch_path wp)
{
    LOGGER_WARN("TODO: sweep %s", wp->fullpath);

    return XS_SUCCESS;
}

