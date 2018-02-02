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
 * watch_entry.h
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-24
 * update: 2018-02-02
 *
 */
#ifndef WATCH_ENTRY_H_INCLUDED
#define WATCH_ENTRY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../xsync-error.h"
#include "../xsync-config.h"

#include "../common/common_util.h"


#define  MD5_HASH_FIXLEN     32


#define XS_watch_entry_hash_get(path)   \
    ((int)(BKDRHash(path) & XSYNC_WATCH_ENTRY_HASHMAX))


/**
 * xs_watch_entry_t
 *
 */
typedef struct xs_watch_entry_t
{
    EXTENDS_REFOBJECT_TYPE();

    struct xs_watch_entry_t *next;

    uint64_t entryid;                   /* server-side entry id for file, must valid (> 0) */

    /* read only members */
    int  sid;                           /* server id receive event */
    int  wd;                            /* watch path descriptor */

    time_t cretime;                     /* creation time of the entry */
    time_t curtime;                     /* current time of the entry */

    uint64_t offset;                    /* current offset position */

    int rofd;                           /* read only file descriptor */

    int status;                         /* status code */

    int hash;

    /**
     * name
     *   entry 的系统唯一名称
     *
     * [sid:wd/filename\0md5hash\0\0]
     *
     * [1:8/abc.log\0d963cac3278ea9adf57cc1a816542913\0\0]
     * namelen = strlen("1:8/abc.log") = 11
     * md5hash = name + namelen + 1 = 'd963cac3278ea9adf57cc1a816542913'
     */
    int namelen;
    char name[0];
} * XS_watch_entry, xs_watch_entry_t;


// 得到 md5 签名的字符串的位置
#define XS_watch_entry_md5hash(entry)   (& entry->name[namelen + 1])

// 得到 file 文件名字符串的位置
#define XS_watch_entry_filename(entry)  (strchr(entry->name, '/') + 1)


__attribute__((used))
static void xs_watch_entry_delete(void *pv)
{
    XS_watch_entry entry = (XS_watch_entry) pv;

    assert(entry->next == 0);

    // TODO:


    free(pv);

    LOGGER_TRACE0();
}


extern XS_RESULT XS_watch_entry_create (int wd, int sid, char *filename, int namelen, XS_watch_entry * outEntry);

extern void XS_watch_entry_release (XS_watch_entry * inEntry);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_ENTRY_H_INCLUDED */
