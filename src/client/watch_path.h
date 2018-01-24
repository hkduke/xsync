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

#include "../common.h"

#include "../common/dhlist.h"


/**
 * xs_watch_path_t
 *   对应每个要监控的目录
 */
typedef struct xs_watch_path_t
{
    EXTENDS_REFOBJECT_TYPE();

    char pathid[XSYNC_CLIENTID_MAXLEN + 1];

    /* inotify watch mask */
    uint32_t  watch_mask;
    int watch_wd;

//    char pathprefix[FSYNC_PATHPREFIX_LEN + 1];

    /* path-pattern */
    //filter_pattern_t path_included;
    //filter_pattern_t path_excluded;

    /* file-pattern
    filter_pattern_t file_included;
    filter_pattern_t file_excluded;
 */

    /* Sync file timeout in seconds
    uint32_t synctimeo;
*/
    /* Zombie file timeout in seconds
    uint32_t zombietimeo;
*/

    /**
     * 指定该目录下的文件需要同步到那些服务器
     * serverid_list[0] = servers
     *    sid = [ 1 : servers ]
     */
    int serverid_list[XSYNC_SERVER_MAXID + 1];

    /**
     * dhlist node */
    struct list_head  i_list;
    struct hlist_node i_hash;

    /**
     * hash map for watch_wd */
    struct xs_watch_path_t * next;

    /* absolute path for monitor */
    char fullpath[0];
} * XS_watch_path, xs_watch_path_t;


__attribute__((used))
static inline int watch_path_get_servers (XS_watch_path wp)
{
    return wp->serverid_list[0];
}


extern int XS_watch_path_create (const char * fullpath, uint32_t mask, XS_watch_path * outwp);

extern void XS_watch_path_release (XS_watch_path * wp);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_PATH_H_INCLUDED */
