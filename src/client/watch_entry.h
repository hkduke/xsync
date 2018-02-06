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

#include "../common/mul_timer.h"
#include "../common/common_util.h"


#define  MD5_HASH_FIXLEN     32

// 得到 nameid 的 hash
#define xs_watch_entry_hash(path)   BKDRHash2(path, XSYNC_WATCH_ENTRY_HASHMAX)

// 得到 file 文件名字符串的位置
#define xs_entry_nameid(entry)    (entry->namebuf)

// 得到 file 文件名字符串的位置
#define xs_entry_filename(entry)    (&entry->namebuf[entry->nameoff])

// 得到 md5 签名的字符串的位置
#define xs_entry_md5sig(entry)     (&entry->namebuf[entry->nameoff + entry->namelen + sizeof('\0')])

// 得到 file 全路径字符串的位置: /var/log/abc.log
#define xs_entry_fullpath(entry)    (&entry->namebuf[entry->nameoff + entry->namelen + sizeof('\0') + MD5_HASH_FIXLEN + sizeof('\0')])

#define xs_entry_path_endpos(entry)    (entry->pathsize - 1)

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

    int status;                         /* status code: */

    int hash;

    /**
     * namebuf
     *   entry 的系统唯一名称
     *
     * [sid:wd/filename\0md5hash\0fullpath\0\0]
     * [1:23/abc.log\0d963cac3278ea9adf57cc1a816542913\0/var/log/abc.log\0\0]
     */
    int nameoff;       // = 0 + strlen("1:23/") = 5
    int namelen;       // = strlen("abc.log")   = 7
    int pathsize;      // = strlen("/var/log/abc.log") + sizeof('\0')
    char namebuf[0];
} xs_watch_entry_t;


__no_warning_unused(static)
void xs_watch_entry_delete(void *pv)
{
    XS_watch_entry entry = (XS_watch_entry) pv;

    LOGGER_TRACE0();

    assert(entry->next == 0);

    // TODO:


    free((void *) entry);
}


extern XS_RESULT XS_watch_entry_create (const XS_watch_path wp, int sid, char *filename, int namelen, XS_watch_entry * outEntry);

extern void XS_watch_entry_release (XS_watch_entry * inEntry);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_ENTRY_H_INCLUDED */
