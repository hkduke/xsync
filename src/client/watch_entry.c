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
#include "client_api.h"

#include "watch_entry.h"
#include "server_opts.h"
#include "watch_path.h"
#include "watch_event.h"

#include "client_conf.h"

#include "../common/common_util.h"


extern XS_RESULT XS_watch_entry_create (const XS_watch_path wp, int sid, char *filename, int namelen, XS_watch_entry *outEntry)
{
    XS_watch_entry entry;

    char nameid[20];

    int nameoff = snprintf(nameid, sizeof(nameid), "%d:%d/", sid, wp->watch_wd);

    ssize_t nbsize = nameoff + namelen + sizeof('\0') + MD5_HASH_FIXLEN + sizeof('\0') + wp->pathsize + namelen + sizeof('\0');

    entry = (XS_watch_entry) mem_alloc(1, sizeof(struct xs_watch_entry_t) + sizeof(char) * nbsize);

    entry->nameoff = nameoff;
    entry->namelen = namelen;
    entry->pathsize = wp->pathsize;

    memcpy(xs_entry_nameid(entry), nameid, entry->nameoff);
    memcpy(xs_entry_filename(entry), filename, namelen);
    memcpy(xs_entry_fullpath(entry), wp->fullpath, wp->pathsize);
    memcpy(xs_entry_fullpath(entry) + wp->pathsize, filename, namelen);

    // xs_entry_fullpath(entry)[xs_entry_path_endpos(entry)] = '/';

    entry->rofd = -1;

    entry->wd = wp->watch_wd;
    entry->sid = sid;

    entry->hash = xs_watch_entry_hash(entry->namebuf);

    entry->cretime = time(0);

    LOGGER_TRACE("'%s' (hash=%d fullpath='%s')", xs_entry_nameid(entry), entry->hash, xs_entry_fullpath(entry));

    if (RefObjectInit(entry) == 0) {
        *outEntry = entry;
        LOGGER_TRACE("entry=%p", entry);
        return XS_SUCCESS;
    } else {
        LOGGER_FATAL("RefObjectInit");
        *outEntry = 0;
        xs_watch_entry_delete((void*) entry);
        return XS_ERROR;
    }
}


extern void XS_watch_entry_release (XS_watch_entry * inEntry)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inEntry, xs_watch_entry_delete);
}
