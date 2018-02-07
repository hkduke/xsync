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

#include "watch_event.h"
#include "server_opts.h"
#include "watch_path.h"
#include "watch_entry.h"

#include "client_conf.h"

#include "../common/common_util.h"


__no_warning_unused(static)
void xs_watch_event_delete (void *pv)
{
    XS_watch_event event = (XS_watch_event) pv;

    LOGGER_TRACE0();

    // 本对象(event)即将被删除, 先设置本对象不再被 entry 使用
    assert(event->entry);

    __interlock_release(&event->entry->in_use);

    // 释放引用计数
    XS_watch_entry_release(&event->entry);
    XS_client_release(&event->client);

    event->entry = 0;
    event->client = 0;

    // DONOT release XS_server_opts since it not a RefObject !

    free(pv);
}


extern XS_VOID XS_watch_event_create (int eventid, XS_client client, XS_watch_entry entry, XS_watch_event *outEvent)
{
    XS_watch_event event;

    *outEvent = 0;

    event = (XS_watch_event) mem_alloc(1, sizeof(struct xs_watch_event_t));

    // 增加 client 计数
    event->client = (XS_client) RefObjectRetain((void**) &client);

    // 增加 entry 计数
    event->entry = (XS_watch_entry) RefObjectRetain((void**) &entry);

    if (! event->client || ! event->entry) {
        LOGGER_FATAL("exit for application error");
        exit(XS_ERROR);
    }

    event->server = XS_client_get_server_opts(client, entry->sid);
    event->ineventid = eventid;
    event->taskid = __interlock_add(&client->task_counter);

    *outEvent = (XS_watch_event) RefObjectInit(event);

    LOGGER_TRACE("event=%p (task=%lld)", event, (long long) event->taskid);
}


extern XS_VOID XS_watch_event_release (XS_watch_event *inEvent)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**)inEvent, xs_watch_event_delete);
}


