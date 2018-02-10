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
#include "server_api.h"

#include "file_entry.h"


extern XS_VOID XS_file_entry_create (XS_file_entry *outEntry)
{
    XS_file_entry entry;

    *outEntry = 0;

    entry = (XS_file_entry) mem_alloc(1, sizeof(struct xs_file_entry_t));

    entry->wofd = -1;

    __interlock_set(&entry->in_use, 1);

    *outEntry = (XS_file_entry) RefObjectInit(entry);

    LOGGER_TRACE("entry=%p", entry);
}


extern XS_VOID XS_file_entry_release (XS_file_entry * inEntry)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inEntry, xs_file_entry_delete);
}


extern XS_BOOL XS_file_entry_not_in_use (XS_file_entry entry)
{
    int in_use = __interlock_get(&entry->in_use);

    return (in_use == 0? 1 : 0);
}
