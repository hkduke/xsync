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
 * @update: 2018-08-10 18:11:59
 */

#include "client_api.h"

#include "watch_event.h"
#include "server_conn.h"
#include "watch_path.h"
#include "watch_entry.h"

#include "client_conf.h"

#include "../common/common_util.h"


__no_warning_unused(static)
void xs_watch_event_delete (void *pv)
{
    XS_watch_event event = (XS_watch_event) pv;

    LOGGER_TRACE0();

    // TODO: 本对象(event)即将被删除

    // 从 event_map_hlist 清除自身
    hlist_del(&event->i_hash);

    // 释放引用计数
    XS_client_release(&event->client);

    event->client = 0;

    // DONOT release XS_server_conn since it not a RefObject !

    free(pv);
}


extern XS_VOID XS_watch_event_create (struct inotify_event * inevent, XS_client client, int hash, int sid, XS_watch_event *outEvent)
{
    XS_watch_event event;

    *outEvent = 0;

    event = (XS_watch_event) mem_alloc(1, sizeof(struct xs_watch_event_t) + inevent->len + 1);

    // 增加 client 计数
    event->client = XS_client_retain(&client);

    event->server = XS_client_get_server_opts(client, sid);

    event->inevent_mask = inevent->mask;

    // 复制文件名
    event->namelen = inevent->len;
    memcpy(event->pathname, inevent->name, inevent->len);
    event->pathname[event->namelen] = 0;

    if (hash == -1) {
        event->hash = BKDRHash2(event->pathname, XSYNC_WATCH_PATH_HASHMAX);
    } else {
        event->hash = hash;
    }

    event->taskid = __interlock_add(&client->task_counter);

    *outEvent = (XS_watch_event) RefObjectInit(event);

    LOGGER_DEBUG("new event=%p (task=%lld name=%s)", event, (long long) event->taskid, (char*) event->pathname);
}


extern XS_VOID XS_watch_event_release (XS_watch_event *inEvent)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**)inEvent, xs_watch_event_delete);
}


/* called in do_event_task() */
extern ssize_t XS_watch_event_sync_file (XS_watch_event event, perthread_data *perdata)
{
    off_t pos;
    ssize_t cbread;

    unsigned int sendbytes = 0;

    XS_watch_entry entry = event->entry;
    assert(entry);
    assert(entry->in_use);

    //int sid = entry->sid;
    //TODO: int sockfd = perdata->sockfds[sid];

    return sendbytes;

    int rofd = watch_entry_open_file(entry);

    if (rofd != -1) {
        pos = (off_t) entry->offset;

        while (sendbytes < XSYNC_BATCH_SEND_MAXSIZE &&
            (cbread = pread_len(rofd, (ub1 *) perdata->buffer, sizeof(perdata->buffer), pos)) > 0) {

            #if XSYNC_LINUX_SENDFILE == 1
                //watch_entry_read_and_sendfile
                LOGGER_WARN("TODO: sendfile: %lld bytes", (long long) cbread);
            #else
                //watch_entry_read_and_senddata
                LOGGER_WARN("TODO: sendbytes: %lld bytes", (long long) cbread);
            #endif

            entry->offset += cbread;
            sendbytes += cbread;

            pos = (off_t) entry->offset;
        }

        watch_entry_close_file(event->entry);
    }

    return sendbytes;
}

