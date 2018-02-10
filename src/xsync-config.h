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
 * xsync-config.h
 *   Definitions for xsync-client and xsync-server
 *
 * author: master@pepstack.com
 *
 * create: 2018-01-24
 * update: 2018-01-30
 */

#ifndef XSYNC_CONFIG_H_
#define XSYNC_CONFIG_H_


#if defined(__cplusplus)
extern "C"
{
#endif

#include "../common/common_incl.h"


/**
 * DO NOT CHANGE BELOW VALUE
 */
#define XSYNC_CLIENT_APPNAME            "xsync-client"
#define XSYNC_CLIENT_VERSION            "0.0.1"
#define XSYNC_CLIENT_XMLNS              "http://github.com/pepstack/xsync/client"

#define XSYNC_SERVER_APPNAME            "xsync-server"
#define XSYNC_SERVER_VERSION            "0.0.1"
#define XSYNC_SERVER_XMLNS              "http://github.com/pepstack/xsync/server"

#define XSYNC_COPYRIGHT                 "pepstack.com"
#define XSYNC_AUTHOR                    "master@pepstack.com"


/**
 * DO NOT CHANGE BELOW VALUE
 *   4096, 8192
 */
#define XSYNC_BUFSIZE                   BUFFER_MAXSIZE


/**
 * define max size of path file
 */
#ifndef XSYNC_PATH_MAXSIZE
#  define XSYNC_PATH_MAXSIZE            (PATHFILE_MAXLEN + 1)
#endif


/**
 * 是否使用 sendfile 直接传输文件. sendfile 只在 Linux 平台支持
 */
#ifndef XSYNC_LINUX_SENDFILE
#  define XSYNC_LINUX_SENDFILE          1
#endif


/**
 * 定义 1 次 (ONE BATCH) 传送文件的最大字节数
 * 文件应该分次传送，因此 1 次不宜过大.
 * 默认=INT_MAX : 2GB (=2147483647 Bytes)
 */
#ifndef XSYNC_BATCH_SEND_MAXSIZE
#  define XSYNC_BATCH_SEND_MAXSIZE      INT_MAX
#endif


/**
 * The directory ("/var/run/xsync") specified by XSYNC_PID_PREFIX
 *   must has R|W permission for current runuser.
 */
#ifndef XSYNC_PID_PREFIX
#  define XSYNC_PID_PREFIX              "/var/run/xsync"
#endif


/**
 * 8192 can hold 30 (num_inevents=30) inotify_events in minimum.
 *   INEVENT_BUFSIZE = num_inevents * (sizeof(struct inotify_event) + NAME_MAX + 1)
 * If you want to hold more events you should increment INEVENT_BUFSIZE (for examle: 16384).
 * Note that you should not set it less than 4096 or more than 65536
 */
#ifndef XSYNC_INEVENT_BUFSIZE
#  define XSYNC_INEVENT_BUFSIZE         XSYNC_BUFSIZE
#endif


/**
 * max threads and queues for client and server
 */
#define XSYNC_TASKS_PERTHREAD           64
#define XSYNC_THREADS_MAXIMUM           256
#define XSYNC_QUEUES_MAXIMUM            4096

#ifndef XSYNC_CLIENT_THREADS
#  define XSYNC_CLIENT_THREADS          4
#endif

#ifndef XSYNC_SERVER_THREADS
#  define XSYNC_SERVER_THREADS          16
#endif

#ifndef XSYNC_CLIENT_QUEUES
#  define XSYNC_CLIENT_QUEUES           256
#endif

#ifndef XSYNC_SERVER_QUEUES
#  define XSYNC_SERVER_QUEUES           1024
#endif


/**
 * for both server and client
 *   系统支持的最大长度客户ID
 */
#ifndef XSYNC_CLIENTID_MAXLEN
#  define XSYNC_CLIENTID_MAXLEN         40
#endif


/**
 * for both server and client
 *   系统支持的最大长度路径文件名
 */
#ifndef XSYNC_PATHFILE_MAXLEN
#  define XSYNC_PATHFILE_MAXLEN         PATHFILE_MAXLEN
#endif


/**
 * 支持的最大的服务器数目. 根据需要可以更改这个值 (不可以大于 30)
 */
#ifndef XSYNC_SERVER_MAXID
#  define XSYNC_SERVER_MAXID            255
#endif


#ifndef XSYNC_PORT_DEFAULT
#  define XSYNC_PORT_DEFAULT            "8960"
#endif


#ifndef XSYNC_MAGIC_DEFAULT
#  define XSYNC_MAGIC_DEFAULT           "89604916"
#endif


#ifndef XSYNC_ERRBUF_MAXLEN
#  define XSYNC_ERRBUF_MAXLEN           ERRORMSG_MAXLEN
#endif


#ifndef XSYNC_HOSTNAME_MAXLEN
#  define XSYNC_HOSTNAME_MAXLEN         HOSTNAME_MAXLEN
#endif


#ifndef XSYNC_PORTNUMB_MAXLEN
#  define XSYNC_PORTNUMB_MAXLEN         8
#endif


/**
 * for xsync client
 */
#ifndef XSYNC_WATCH_PATH_HASHMAX
#  define XSYNC_WATCH_PATH_HASHMAX      255
#endif


/**
 * for xsync client
 */
#ifndef XSYNC_WATCH_ENTRY_HASHMAX
#  define XSYNC_WATCH_ENTRY_HASHMAX     1023
#endif


/**
 * watch path sweeping interval in seconds
 *   should in [60, 300]
 */
#ifndef XSYNC_SWEEP_PATH_INTERVAL
#  define XSYNC_SWEEP_PATH_INTERVAL     10
#endif


/**
 * only for client
 *  watch entry session timeout in seconds:
 *    should in [300, 900]
 */
#ifndef XSYNC_ENTRY_SESSION_TIMEOUT
#  define XSYNC_ENTRY_SESSION_TIMEOUT   30
#endif


/**
 * only for xsync server:
 *
 *   XSYNC_CLIENT_SESSION_HASHMAX = 2^n - 1 (n = 8, 10, 12)
 *
 * 1) if has a lot of clients, set it as bigger as: n = 10, 12
 *
 * 2) if has few clients, let it be the default: 255
 */
#ifndef XSYNC_CLIENT_SESSION_HASHMAX
#  define XSYNC_CLIENT_SESSION_HASHMAX      255
#endif


/**
 * only for xsync server:
 *
 *   XSYNC_FILE_ENTRY_HASHMAX = 2^n - 1 (n = 4, 5, 6, 7, 8, 9, 10)
 *
 * 1) if has a lot of clients, set it as smaller as:
 *      n = 15, 63
 *
 * 2) if one client has many files to sync at same time, set as bigger as:
 *      n = 63, 127, 255
 *
 * 3) nerver set if more than 1023 (2^10-1)
 **/
#ifndef XSYNC_FILE_ENTRY_HASHMAX
#  define XSYNC_FILE_ENTRY_HASHMAX      255
#endif


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_CONFIG_H_ */
