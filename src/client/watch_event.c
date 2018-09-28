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
 * @file: watch_event.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.7
 *
 * @create: 2018-01-24
 *
 * @update: 2018-09-21 18:40:42
 */

#include "client_api.h"

#include "watch_event.h"
#include "server_conn.h"
#include "watch_path.h"
#include "watch_entry.h"

#include "client_conf.h"

#include "../common/common_util.h"


XS_watch_event XS_watch_event_create (struct inotify_event *inevent, const char *path)
{
    int len = (int) strlen(path);

    if (!len || !inevent->len || inevent->len >= 256) {
        LOGGER_FATAL("should never run to this!");
        exit(-1);
    } else {
        XS_watch_event event;

        event = (XS_watch_event) mem_alloc(1, sizeof(struct xs_watch_event_t) + len + 1);

        event->wd = inevent->wd;
        event->mask = inevent->mask;
        event->cookie = inevent->cookie;

        event->len = inevent->len;
        memcpy(event->name, inevent->name, event->len);

        event->pathlen = len;
        memcpy(event->pathname, path, event->pathlen);

        return event;
    }
}


XS_VOID XS_watch_event_free (XS_watch_event event)
{
    free(event);
}

