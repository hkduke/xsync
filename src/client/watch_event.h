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
 * watch_event.h
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-24
 * update: 2018-01-24
 *
 */
#ifndef WATCH_EVENT_H_INCLUDED
#define WATCH_EVENT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../xsync-error.h"
#include "../xsync-config.h"
#include "../xsync-protocol.h"

#include "server_conn.h"

#include "../common/common_util.h"


typedef struct perthread_data
{
    int    threadid;

    int    sessions[XSYNC_SERVER_MAXID + 1];
    int    sockfds[XSYNC_SERVER_MAXID + 1];

    XS_server_conn srvconn;

    unsigned char buffer[XSYNC_BUFSIZE];
} perthread_data;


typedef struct xs_watch_event_t
{
    EXTENDS_REFOBJECT_TYPE();

    /* inotify event mask */
    int               inevent_mask;

    /* task serial id */
    int64_t          taskid;

    /* reference of XS_client */
    XS_client        client;

    /* reference of XS_watch_entry */
    XS_watch_entry  entry;

    /* const reference of server opts for read only */
    xs_server_opts *server;

} xs_watch_event_t;


extern XS_VOID XS_watch_event_create (int inevent_mask, XS_client client, XS_watch_entry entry, XS_watch_event *outEvent);

extern XS_VOID XS_watch_event_release (XS_watch_event *inEvent);

/* called in event_task() */
extern ssize_t XS_watch_event_sync_file (XS_watch_event event, perthread_data *perdata);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_EVENT_H_INCLUDED */
