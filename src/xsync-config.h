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
 * @file: xsync-config.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.9
 *
 * @create: 2018-01-24
 *
 * @update: 2018-08-16 20:15:28
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
#define XSYNC_VERSION    "0.0.9"

#ifndef XSYNC_CLIENT_VERSION
#  define XSYNC_CLIENT_VERSION          XSYNC_VERSION
#endif

#define XSYNC_CLIENT_APPNAME            "xsync-client"
#define XSYNC_CLIENT_XMLNS              "http://github.com/pepstack/xsync/client"

#ifndef XSYNC_SERVER_VERSION
#  define XSYNC_SERVER_VERSION          XSYNC_VERSION
#endif

#define XSYNC_SERVER_APPNAME            "xsync-server"
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

#ifndef XSYNC_CLIENT_THREADS_MAX
#define XSYNC_CLIENT_THREADS_MAX        8192
#endif

/**
 * default threads and queues for client and server
 */
#ifndef XSYNC_CLIENT_THREADS
#  define XSYNC_CLIENT_THREADS          4
#endif

#ifndef XSYNC_CLIENT_QUEUES
#  define XSYNC_CLIENT_QUEUES           256
#endif

#ifndef XSYNC_SERVER_THREADS
#  define XSYNC_SERVER_THREADS          16
#endif

#ifndef XSYNC_SERVER_QUEUES
#  define XSYNC_SERVER_QUEUES           1024
#endif

#ifndef XSYNC_SERVER_EVENTS
#  define XSYNC_SERVER_EVENTS           1024
#endif

#ifndef XSYNC_SERVER_SOMAXCONN
#  define XSYNC_SERVER_SOMAXCONN        1024
#endif


/**
 * for both server and client
 *   系统支持的最大长度客户ID, 必须最大为: 40 个ANSI字符
 */
#define XSYNC_CLIENTID_MAXLEN           40


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
