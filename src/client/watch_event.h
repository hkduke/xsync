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
 * @update: 2018-08-10 18:11:59
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

    XS_server_conn server_conns[XSYNC_SERVER_MAXID + 1];;

    char buffer[XSYNC_BUFSIZE];
} perthread_data;


struct xs_watch_event_t
{
    EXTENDS_REFOBJECT_TYPE();

    /* inotify event mask */
    int inevent_mask;

    /* task serial id */
    int64_t taskid;

    /* reference of XS_client */
    XS_client client;

    /* reference of XS_watch_entry */
    XS_watch_entry entry;

    /* const reference of server opts for read only */
    xs_server_opts *server;

    /* see event_map_hlist of XS_client */
    int hash;
    struct hlist_node i_hash;

    /* 文件的全路径名长度和全路径名 */
    int namelen;
    char pathname[0];
} __attribute((packed));


extern XS_VOID XS_watch_event_create (struct inotify_event * inevent, XS_client client, int hash, int sid, XS_watch_event *outEvent);

extern XS_VOID XS_watch_event_release (XS_watch_event *inEvent);

/* called in do_event_task() */
extern ssize_t XS_watch_event_sync_file (XS_watch_event event, perthread_data *perdata);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_EVENT_H_INCLUDED */
