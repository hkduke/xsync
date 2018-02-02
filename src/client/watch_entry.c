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


extern XS_RESULT XS_watch_entry_create (int wd, int sid, char *filename, int namelen, XS_watch_entry *outEntry)
{
    int err;

    XS_watch_entry entry;

    ssize_t size = namelen + MD5_HASH_FIXLEN + 30;

    *outEntry = 0;

    if (! filename || namelen == 0) {
        LOGGER_WARN("invalid file name");
        return XS_E_FILE;
    }

    entry = (XS_watch_entry) mem_alloc(1, sizeof(struct xs_watch_entry_t) + sizeof(char)*size);

    entry->rofd = -1;
    entry->wd = wd;
    entry->sid = sid;

    entry->namelen = snprintf(entry->name, size, "%d:%d/%s", sid, wd, filename);
    entry->hash = XS_watch_entry_hash_get(entry->name);
    entry->cretime = time(0);

    LOGGER_TRACE("hash=%d, name='%s'", entry->hash, entry->name);

    /**
     * output XS_watch_entry
     */
    if ((err = RefObjectInit(entry)) == 0) {
        *outEntry = entry;
        LOGGER_TRACE("entry=%p", entry);
        return XS_SUCCESS;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        xs_watch_entry_delete((void*) entry);
        return XS_ERROR;
    }
}


extern void XS_watch_entry_release (XS_watch_entry * inEntry)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inEntry, xs_watch_entry_delete);
}
