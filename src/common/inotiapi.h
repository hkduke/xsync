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
 * inotiapi.h
 *
 *    inotify api
 *
 * 2014-08-01: init created
 * 2018-01-22: last updated
 * zhangliang
 */
#ifndef INOTIAPI_H_INCLUDED
#define INOTIAPI_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include <sys/inotify.h>


#define INOMSG_IN_ACCESS            "IN_ACCESS - file was accessed"
#define INOMSG_IN_MODIFY            "IN_MODIFY - file was modified"
#define INOMSG_IN_ATTRIB            "IN_ATTRIB - metadata changed"
#define INOMSG_IN_CLOSE_WRITE       "IN_CLOSE_WRITE - writtable file was closed"
#define INOMSG_IN_CLOSE_NOWRITE     "IN_CLOSE_NOWRITE - unwrittable file closed"
#define INOMSG_IN_OPEN              "IN_OPEN - file was opened"
#define INOMSG_IN_MOVED_FROM        "IN_MOVED_FROM - file was moved from X"
#define INOMSG_IN_MOVED_TO          "IN_MOVED_TO - file was moved to Y"
#define INOMSG_IN_CREATE            "IN_CREATE - subfile was created"
#define INOMSG_IN_DELETE            "IN_DELETE - subfile was deleted"
#define INOMSG_IN_DELETE_SELF       "IN_DELETE_SELF - self was deleted"
#define INOMSG_IN_MOVE_SELF         "IN_MOVE_SELF - self was moved"
#define INOMSG_IN_UNMOUNT           "IN_UNMOUNT - backing fs was unmounted"
#define INOMSG_IN_Q_OVERFLOW        "IN_Q_OVERFLOW - event queued overflowed"
#define INOMSG_IN_IGNORED           "IN_IGNORED - file was ignored"

#define INOMSG_IN_ONLYDIR           "IN_ONLYDIR - only watch the path if it is a directory"
#define INOMSG_IN_DONT_FOLLOW       "IN_DONT_FOLLOW - don't follow a sym link"
#define INOMSG_IN_MASK_ADD          "IN_MASK_ADD - add to the mask of an already existing watch"
#define INOMSG_IN_ISDIR             "IN_ISDIR - event occurred against dir"
#define INOMSG_IN_ONESHOT           "IN_ONESHOT - only send event once"        


__no_warning_unused(static)
ssize_t inoti_read_len (int infd, char *inbuf, size_t len)
{
    char *pb = inbuf;

    while (1) {
        ssize_t cb = read(infd, (void*) pb, len);

        if (cb == -1) {
            if (errno == EAGAIN) {
                /* read ok */
                break;
            }

            /* read error */
            perror("read");
            return (-1);
        }

        len -= cb;
        pb += cb;
    }

    return (ssize_t) (pb - inbuf);
}


#if defined(__cplusplus)
}
#endif

#endif /* INOTIAPI_H_INCLUDED */
