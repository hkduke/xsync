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

#include "../common/common_util.h"

/**
 * https://sourceforge.net/projects/inotify-tools/
 */
#include <inotifytools/inotify.h>
#include <inotifytools/inotifytools.h>


#define INOTI_EVENTS_MASK  (IN_DELETE | IN_DELETE_SELF | IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVE | IN_ONLYDIR)


struct inotify_event_equivalent {
	int		wd;		/* watch descriptor */
	uint32_t		mask;		/* watch mask */
	uint32_t		cookie;		/* cookie to synchronize two events */
	uint32_t		len;		/* length (including nulls) of name */
	char		    name[PATH_MAX + 1];
};


__no_warning_unused(static)
struct inotify_event * makeup_inotify_event (struct inotify_event_equivalent *event,
    int wd, int mask, int cookie,
    const char *name, ssize_t namelen)
{
    event->wd = wd;
    event->mask = mask;
    event->cookie = cookie;
    event->name[0] = 0;
    event->len = 0;

    if (namelen < sizeof(event->name)) {
        memcpy(event->name, name, namelen);
        event->name[namelen] = 0;
        event->len = namelen;
    }

    return (struct inotify_event *) event;
}


static const char *inoti_event_names[] = {
    "CLOSE_NOWRITE",
    "MODIFY",
    "CLOSE_WRITE",
    "CREATE",
    "DELETE",
    "DELETE_SELF",
    "MOVE",
    "ONLYDIR",
    "UNKNOWN"
};


__no_warning_unused(static)
const char * inotify_event_name(int inevent_mask)
{
    if (inevent_mask & IN_CLOSE_NOWRITE) {
        return inoti_event_names[0];
    } else if (inevent_mask & IN_MODIFY) {
        return inoti_event_names[1];
    } else if (inevent_mask & IN_CLOSE_WRITE) {
        return inoti_event_names[2];
    } else if (inevent_mask & IN_CREATE) {
        return inoti_event_names[3];
    } else if (inevent_mask & IN_DELETE) {
        return inoti_event_names[4];
    } else if (inevent_mask & IN_DELETE_SELF) {
        return inoti_event_names[5];
    } else if (inevent_mask & IN_MOVE) {
        return inoti_event_names[6];
    } else if (inevent_mask & IN_ONLYDIR) {
        return inoti_event_names[7];
    } else {
        return inoti_event_names[8];
    }
}



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
