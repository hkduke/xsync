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

    /**
     * 是否处于使用中
     *
     *   1: in use
     *   0: not in use
     */
    int volatile in_use;

    /**
     * server-side entry id for file, must valid ( > 0)
     * 服务端返回的唯一文件代码, 必须 > 0. 否则需要向服务端请求这个代码
     *   0 - 未初始化
     * > 0 - 初始化
     *  -1 - 有错误
     */
    uint64_t entryid;

    int rofd;                           /* local file descriptor for read only: -1 error or uninit */
    struct stat rofd_sb;                /* stat of rofd */

    uint64_t offset;                    /* current offset position */

    /* read only members */
    int  sid;                           /* server id receive event */
    int  wd;                            /* watch path descriptor */
    int hash;                           /* hash id */

    time_t cretime;                     /* creation time of the entry */
    time_t curtime;                     /* current time of the entry */

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


/* close entry file */
__no_warning_unused(static)
void watch_entry_close_file (XS_watch_entry entry)
{
    if (entry->rofd != -1) {
        const char * entryfile = xs_entry_fullpath(entry);

        int fd = entry->rofd;
        entry->rofd = -1;

        if (close(fd) == -1) {
            /* exception error */
            LOGGER_ERROR("close file error(%d): %s. (%s)", errno, strerror(errno), entryfile);
        } else {
            LOGGER_DEBUG("close file ok. (%s)", entryfile);
        }
    }

    assert(entry->rofd == -1);
}


/* open entry file */
__no_warning_unused(static)
int watch_entry_open_file (XS_watch_entry entry)
{
    const char *entryfile = xs_entry_fullpath(entry);

    watch_entry_close_file(entry);

    entry->rofd = open(entryfile, O_RDONLY | O_NONBLOCK| O_NOFOLLOW);

    if (entry->rofd == -1) {
        LOGGER_ERROR("open error(%d): %s. (%s)", errno, strerror(errno), entryfile);
        return -1;
    }

    /* update file status for last modification time */
    if (fstat(entry->rofd, &entry->rofd_sb) != 0) {
        LOGGER_ERROR("fstat error(%d): %s. (%s)", errno, strerror(errno), entryfile);
        watch_entry_close_file(entry);
        return -1;
    }

    /* update time of entry */
    entry->curtime = time(0);

    LOGGER_DEBUG("open success: size=%lld, mtime=%lld. (%s)",
        (long long) entry->rofd_sb.st_size,
        (long long) entry->rofd_sb.st_mtime,
        entryfile);

    return entry->rofd;
}


__no_warning_unused(static)
inline void watch_entry_delete(void *pv)
{
    XS_watch_entry entry = (XS_watch_entry) pv;

    LOGGER_TRACE0();

    assert(entry->next == 0);

    watch_entry_close_file(entry);

    free(pv);
}


extern XS_VOID XS_watch_entry_create (const XS_watch_path wp, int sid, const char *filename, XS_watch_entry * outEntry);

extern XS_VOID XS_watch_entry_release (XS_watch_entry * inEntry);

extern XS_BOOL XS_watch_entry_not_in_use (XS_watch_entry entry);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_ENTRY_H_INCLUDED */
