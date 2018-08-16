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

    // 本对象(event)即将被删除, 先设置本对象不再被 entry 使用
    assert(event->entry);

    // 告诉 entry 不再使用, 先关闭文件对象
    watch_entry_close_file(event->entry);

    __interlock_release(&event->entry->in_use);

    // 释放引用计数
    XS_watch_entry_release(&event->entry);
    XS_client_release(&event->client);

    event->entry = 0;
    event->client = 0;

    // DONOT release XS_server_conn since it not a RefObject !

    free(pv);
}


extern XS_VOID XS_watch_event_create (int inevent_mask, XS_client client, XS_watch_entry entry, XS_watch_event *outEvent)
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

    __interlock_set(&entry->in_use, 1);

    event->server = XS_client_get_server_opts(client, entry->sid);

    event->inevent_mask = inevent_mask;

    event->taskid = __interlock_add(&client->task_counter);

    *outEvent = (XS_watch_event) RefObjectInit(event);

    LOGGER_TRACE("event=%p (task=%lld)", event, (long long) event->taskid);
}


extern XS_VOID XS_watch_event_release (XS_watch_event *inEvent)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**)inEvent, xs_watch_event_delete);
}


/* called in event_task() */
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
            (cbread = pread_len(rofd, perdata->buffer, sizeof(perdata->buffer), pos)) > 0) {

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

