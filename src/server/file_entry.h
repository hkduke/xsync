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
 * @file: file_entry.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.3
 *
 * @create: 2018-01-29
 *
 * @update: 2018-09-05 16:31:15
 */

#ifndef FILE_ENTRY_H_INCLUDED
#define FILE_ENTRY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common/common_util.h"
#include "../common/mul_timer.h"

#include "../xsync-error.h"
#include "../xsync-config.h"

#include "../redisapi/redis_api.h"


typedef struct PollinData_t
{
    struct epoll_event event;

    int epollfd;

    void *arg;

    char buf[256];
} PollinData_t;


typedef struct perthread_data
{
    int threadid;

    PollinData_t pollin;

    RedisConn_t  redconn;

    char buffer[XSYNC_BUFSIZE];
} perthread_data;


typedef struct xs_file_entry_t
{
    EXTENDS_REFOBJECT_TYPE();

    int volatile in_use;

    /* 8 bytes: entry id used to identify a file */
    uint64_t entryid;

    /* 8 bytes: offset position of the file */
    int64_t offset;

    /* 4 bytes: status code */
    int32_t status;

    /* server-side file descriptor for writing */
    int wofd;

    /* stat of wofd */
    struct stat wofd_sb;

    /* position in entry db */
    int64_t db_position;

    /**
     * hlist node in entry_hlist of XS_client_session
     */
    struct hlist_node i_hash;

    int pathlen;
    char fullpath[0];
} * XS_file_entry, xs_file_entry_t;


/* close entry file */
__no_warning_unused(static)
void file_entry_close_file (XS_file_entry entry)
{
    int wofd = entry->wofd;

    if (wofd != -1) {
        entry->wofd = -1;

        if (close(wofd) == -1) {
            const char * entryfile = entry->fullpath;

            /* exception error */
            LOGGER_ERROR("close file error(%d): %s. (%s)", errno, strerror(errno), entryfile);
        } else {
            const char * entryfile = entry->fullpath;

            LOGGER_DEBUG("close file ok. (%s)", entryfile);
        }
    }

    assert(entry->wofd == -1);
}


/* open entry file: create target file with filemode */
__no_warning_unused(static)
int file_entry_open_file (XS_file_entry entry, mode_t filemode)
{
    //TODO:
    const char *entryfile = entry->fullpath;

    /* make sure file is closed */
    file_entry_close_file(entry);

    int wofd = open(entryfile, O_WRONLY | O_CREAT, (mode_t) (S_IRUSR| S_IWUSR | filemode));
    if (wofd == -1) {
        LOGGER_ERROR("open error(%d): %s. (%s)", errno, strerror(errno), entryfile);
        return -1;
    }

    /* update file status for last modification time */
    if (fstat(wofd, &entry->wofd_sb) != 0) {
        LOGGER_ERROR("fstat error(%d): %s. (%s)", errno, strerror(errno), entryfile);
        close(wofd);
        return -1;
    }

    entry->wofd = wofd;

    LOGGER_DEBUG("open success: size=%lld, mtime=%lld. (%s)",
        (long long) entry->wofd_sb.st_size,
        (long long) entry->wofd_sb.st_mtime,
        entryfile);

    return wofd;
}


__no_warning_unused(static)
inline void xs_file_entry_delete (void *pv)
{
    XS_file_entry entry = (XS_file_entry) pv;

    LOGGER_TRACE0();

    // TODO:
    file_entry_close_file(entry);

    free(pv);
}


extern XS_VOID XS_file_entry_create (XS_file_entry * outEntry);

extern XS_VOID XS_file_entry_release (XS_file_entry * inEntry);

extern XS_BOOL XS_file_entry_not_in_use (XS_file_entry entry);


#if defined(__cplusplus)
}
#endif

#endif /* FILE_ENTRY_H_INCLUDED */
