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
 * @version: 0.0.7
 *
 * @create: 2018-01-24
 *
 * @update: 2018-09-21 19:11:00
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
    void * xclient;

    int    threadid;

    XS_server_conn server_conns[XSYNC_SERVER_MAXID + 1];;

    char buffer[XSYNC_BUFSIZE];
} perthread_data;


typedef struct xs_watch_event_t
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
} xs_watch_event_t;


extern XS_watch_event XS_watch_event_create (struct inotify_event *inevent, const char *path);

extern XS_VOID XS_watch_event_free (XS_watch_event event);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_EVENT_H_INCLUDED */
