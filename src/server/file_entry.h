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

#ifndef FILE_ENTRY_H_INCLUDED
#define FILE_ENTRY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common/epollapi.h"

#include "../common/common_util.h"
#include "../common/mul_timer.h"

#include "../xsync-error.h"
#include "../xsync-config.h"


typedef struct perthread_data
{
    epollin_data_t  pollin_data;
    epevent_data_t  event_data;

    int threadid;

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
