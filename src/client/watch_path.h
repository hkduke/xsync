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

/**
 * watch_path.h
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2017-01-06
 * update: 2018-01-24
 *
 */
#ifndef WATCH_PATH_H_INCLUDED
#define WATCH_PATH_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../xsync-error.h"
#include "../xsync-config.h"

#include "../common/common_util.h"

#include "path_filter.h"


/**
 * xs_watch_path_t
 *   对应每个要监控的目录
 */
typedef struct xs_watch_path_t
{
    EXTENDS_REFOBJECT_TYPE();

    char pathid[XSYNC_CLIENTID_MAXLEN + 1];

    /* inotify watch mask */
    uint32_t  events_mask;
    int watch_wd;

    /**
     * 指定该目录下的文件需要同步到那些服务器
     * sid_masks[0] = sid_max
     *    sid = [ 1 : sid_max ]
     */
    int sid_masks[XSYNC_SERVER_MAXID + 1];

    /**
     * 包含的文件的正则表达式
     */
    XS_path_filter included_filters[XSYNC_SERVER_MAXID + 1];

    /**
     * 排除文件的正则表达式
     */
    XS_path_filter excluded_filters[XSYNC_SERVER_MAXID + 1];

    /**
     * hlist node
     */
    struct hlist_node i_hash;

    /**
     * hash map for watch_wd
     */
    struct xs_watch_path_t * next;

    /** absolute path for watch */
    int pathsize;
    char fullpath[0];
} xs_watch_path_t;


__no_warning_unused(static)
inline int watch_path_get_sid_max (XS_watch_path wp)
{
    return wp->sid_masks[0];
}


extern XS_RESULT XS_watch_path_create (const char * pathid, const char * fullpath, uint32_t events_mask, XS_watch_path * outwp);

extern XS_VOID XS_watch_path_release (XS_watch_path * wp);

extern XS_path_filter XS_watch_path_get_included_filter (XS_watch_path wp, int sid);

extern XS_path_filter XS_watch_path_get_excluded_filter (XS_watch_path wp, int sid);

extern XS_RESULT XS_watch_path_sweep (XS_watch_path wp);

#if defined(__cplusplus)
}
#endif

#endif /* WATCH_PATH_H_INCLUDED */
