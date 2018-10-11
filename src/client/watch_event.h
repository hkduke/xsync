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
 * @file: watch_event.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.1
 *
 * @create: 2018-01-24
 *
 * @update: 2018-09-30 10:43:37
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
#include "../kafkatools/kafkatools.h"

#include "inotifyapi.h"


typedef struct perthread_data
{
    int    threadid;

    void  *xclient;

    XS_server_conn server_conns[XSYNC_SERVER_MAXID + 1];;

    kafkatools_producer_api_t kt_producer_api;

    char buffer[XSYNC_BUFSIZE];
} perthread_data;


typedef struct watch_event_t
{
    union {
        struct {
            int      wd;           /* Watch descriptor */
            uint32_t mask;         /* Mask of events */
            uint32_t cookie;       /* Unique cookie associating related events (for rename(2)) */
            uint32_t len;          /* Size of name field */
            char     name[256];    /* Optional null-terminated name */
        };

        struct inotify_event inevent;
    };

    /* 文件的全路径名长度和全路径名 */
    int pathlen;
    char pathname[0];
} watch_event_t;


struct watch_event_buf_t
{
    union {
        struct {
            int      wd;           /* Watch descriptor */
            uint32_t mask;         /* Mask of events */
            uint32_t cookie;       /* Unique cookie associating related events (for rename(2)) */
            uint32_t len;          /* Size of name field */
            char     name[256];    /* Optional null-terminated name */
        };

        struct inotify_event inevent;
    };

    /* 文件的全路径名长度和全路径名 */
    int pathlen;
    char pathname[PATH_MAX + 1];
} watch_event_buf_t;


__no_warning_unused(static)
inline int watch_event_compare(const watch_event_t *inNew, const watch_event_t *inNode)
{
    if (inNew->wd > inNode->wd) {
        return 1;
    } else if (inNew->wd < inNode->wd) {
        return -1;
    } else {
        return strcmp(inNew->name, inNode->name);
    }
}


__no_warning_unused(static)
int inotify_event_dump(watch_event_t *outevent, int pathlenmax, struct inotify_event *inevent, char *wpath)
{
    if (! wpath) {
        return 0;
    }
    outevent->pathlen = strlen(wpath);

    if (! outevent->pathlen || outevent->pathlen > pathlenmax) {
        return 0;
    }
    if (! inevent->len || inevent->len >= sizeof(outevent->name)) {
        return 0;
    }

    outevent->wd = inevent->wd;
    outevent->mask = inevent->mask;
    outevent->cookie = inevent->cookie;
    outevent->len = inevent->len;

    memcpy(outevent->name, inevent->name, outevent->len);
    outevent->name[outevent->len] = 0;

    memcpy(outevent->pathname, wpath, outevent->pathlen);
    outevent->pathname[outevent->pathlen] = 0;

    // ok
    return 1;
}


__no_warning_unused(static)
watch_event_t * watch_event_clone(const watch_event_t *inevent)
{
    ssize_t cb = sizeof(watch_event_t) + sizeof(char)*(inevent->pathlen + 1);

    watch_event_t *outevent = (watch_event_t *) malloc(cb);
    if (! outevent) {
        // outof memory
        exit(-4);
    }

    memcpy(outevent, inevent, cb);

    return outevent;
}


__no_warning_unused(static)
void watch_event_free(watch_event_t *event)
{
    free(event);
}


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_EVENT_H_INCLUDED */
