/***********************************************************************
* Copyright (c) 2018 pepstack, pepstack.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*   claim that you wrote the original software. If you use this software
*   in a product, an acknowledgment in the product documentation would be
*   appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
*   misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/
#include "client_api.h"

#include "server_opts.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/common_util.h"


static inline void free_watch_path (void *pv)
{
    xs_watch_path_t * wpath = (xs_watch_path_t *) pv;

    // TODO:

    LOGGER_TRACE("~xpath=%p", wpath);

    free(pv);
}


XS_RESULT XS_watch_path_create (const char * pathid, const char * fullpath, uint32_t events_mask, XS_watch_path * outwp)
{
    int sid, err;
    size_t cb;

    XS_watch_path wpath;

    *outwp = 0;

    cb = strlen(fullpath) + 1;

    wpath = (XS_watch_path) mem_alloc(1, sizeof(*wpath) + sizeof(char) * cb);

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

    if ((err = RefObjectInit(wpath)) == 0) {
        LOGGER_TRACE("xpath=%p (%s=>%s)", wpath, wpath->pathid, wpath->fullpath);
        *outwp = wpath;
        return XS_SUCCESS;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        free_watch_path((void*) wpath);
        return XS_ERROR;
    }
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
    LOGGER_TRACE0();

    assert(sid > 0 && sid <= XSYNC_SERVER_MAXID);

    if (! wp->excluded_filters[sid]) {
        wp->excluded_filters[sid] = XS_path_filter_create(sid, XS_path_filter_patterns_block);
    }

    XS_path_filter filt = wp->excluded_filters[sid];
    assert(filt->sid == sid);

    return filt;
}
