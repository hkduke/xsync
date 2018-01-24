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
#include "client.h"

#include "watch_path.h"


static inline void free_watch_path (void *pv)
{
    xs_watch_path_t * wpath = (xs_watch_path_t *) pv;

    // TODO:

    LOGGER_TRACE("~xpath=%p", wpath);

    free(pv);
}


int XS_watch_path_create (const char * fullpath, uint32_t mask, xs_watch_path_t ** outwp)
{
    int sid, err;
    size_t cb;

    xs_watch_path_t * wpath;

    *outwp = 0;

    cb = strlen(fullpath) + 1;

    wpath = (xs_watch_path_t *) mem_alloc(1, sizeof(*wpath) + sizeof(char) * cb);

    // 必须初始化=-1
    wpath->watch_wd = -1;

    wpath->watch_mask = mask;

    memcpy(wpath->fullpath, fullpath, cb);

    // [0] : max sid enabled
    wpath->serverid_list[0] = 0;

    for (sid = 1; sid <= XSYNC_SERVER_MAXID; sid++) {
        // 0: disabled; 1: enabled
        wpath->serverid_list[sid] = 0;
    }

    if ((err = RefObjectInit(wpath)) == 0) {
        *outwp = wpath;
        LOGGER_TRACE("xpath=%p (%s)", wpath, wpath->fullpath);
        return 0;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        free_watch_path((void*) wpath);
        return (-1);
    }
}


void XS_watch_path_release (xs_watch_path_t ** wp)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) wp, free_watch_path);
}
