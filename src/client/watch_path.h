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
 * @file: watch_path.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.8
 *
 * @create: 2018-01-06
 *
 * @update: 2018-09-25 14:04:51
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

    /* 路径 ID */
    char pathid[XSYNC_CLIENTID_MAXLEN + 1];

    /* parent path */
    struct xs_watch_path_t *parent_wp;

    /* inotify watch mask */
    uint32_t  events_mask;

    /**
     * 指定该目录下的文件需要同步到那些服务器
     * sid_masks[0] = sid_max
     *    sid = [ 1 : sid_max ]
     */
    int sid_masks[XSYNC_SERVER_MAXID + 1];

    /**
     * see wpath_hmap of XS_client
     */
    struct hlist_node i_hash;

    /** absolute path for watch */
    int pathlen;
    char fullpath[0];
} __attribute((packed)) xs_watch_path_t;


__no_warning_unused(static)
inline int watch_path_get_sid_max (XS_watch_path wp)
{
    return wp->sid_masks[0];
}


extern XS_RESULT XS_watch_path_create (const char * pathid, const char * fullpath, uint32_t events_mask, XS_watch_path parent, XS_watch_path * outwp);

extern XS_VOID XS_watch_path_release (XS_watch_path * wp);

#if defined(__cplusplus)
}
#endif

#endif /* WATCH_PATH_H_INCLUDED */
