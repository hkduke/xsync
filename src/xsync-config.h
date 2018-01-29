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
 * update: 2018-01-25
 */

#ifndef XSYNC_CONFIG_H_
#define XSYNC_CONFIG_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#ifndef XSYNC_CLIENT_APPNAME
#  define XSYNC_CLIENT_APPNAME   "xsync-client"
#endif

#ifndef XSYNC_CLIENT_VERSION
#  define XSYNC_CLIENT_VERSION   "0.0.1"
#endif


#ifndef XSYNC_SERVER_APPNAME
#  define XSYNC_SERVER_APPNAME   "xsync-server"
#endif

#ifndef XSYNC_SERVER_VERSION
#  define XSYNC_SERVER_VERSION   "0.0.1"
#endif


/**
 * define max size of path file
 */
#ifndef XSYNC_PATH_MAX_SIZE
#  define XSYNC_PATH_MAX_SIZE        PATH_MAX
#endif


/**
 * The directory ("/var/run/xsync") specified by XSYNC_PID_PREFIX
 *   must has R|W permission for current runuser.
 */
#ifndef XSYNC_PID_PREFIX
#  define XSYNC_PID_PREFIX  "/var/run/xsync"
#endif


/**
 * 8192 can hold 30 (num_inevents=30) inotify_events in minimum.
 *   INEVENT_BUFSIZE = num_inevents * (sizeof(struct inotify_event) + NAME_MAX + 1)
 * If you want to hold more events you should increment INEVENT_BUFSIZE (for examle: 16384).
 * Note that you should not set it less than 4096 or more than 65536
 */
#ifndef XSYNC_INEVENT_BUFSIZE
#  define XSYNC_INEVENT_BUFSIZE   8192
#endif

/**
 * SHOULD = 1024 * n (n = 2, 4, 8, 16)
 */
#ifndef XSYNC_IO_BUFSIZE
#  define XSYNC_IO_BUFSIZE        4096
#endif


/**
 * max threads and queues for client
 */
#ifndef XSYNC_CLIENT_THREADS_MAX
#  define XSYNC_CLIENT_THREADS_MAX    16
#elif XSYNC_CLIENT_THREADS_MAX > 128
#  error XSYNC_CLIENT_THREADS_MAX is more than 128
#elif XSYNC_CLIENT_THREADS_MAX < 8
#  error XSYNC_CLIENT_THREADS_MAX is less than 8
#endif


/**
 * XSYNC_CLIENT_QUEUES_MAX = XSYNC_CLIENT_THREADS_MAX * tasks_per_thread
 * 512 = 16 * 32, tasks_per_thread = 32
 */
#ifndef XSYNC_CLIENT_QUEUES_MAX
#  define XSYNC_CLIENT_QUEUES_MAX    512
#elif XSYNC_CLIENT_QUEUES_MAX > 4096
#  error XSYNC_CLIENT_QUEUES_MAX is more than 4096
#elif XSYNC_CLIENT_QUEUES_MAX < 256
#  error XSYNC_CLIENT_QUEUES_MAX is less than 256
#endif


/**
 * max threads and queues for server
 */
#ifndef XSYNC_SERVER_THREADS_MAX
#  define XSYNC_SERVER_THREADS_MAX    256
#elif XSYNC_SERVER_THREADS_MAX > 1024
#  error XSYNC_SERVER_THREADS_MAX is more than 1024
#elif XSYNC_SERVER_THREADS_MAX < 8
#  error XSYNC_SERVER_THREADS_MAX is less than 8
#endif


/**
 * XSYNC_SERVER_QUEUES_MAX = XSYNC_SERVER_THREADS_MAX * tasks_per_thread
 * 8192 = 256 * 32, tasks_per_thread = 32
 */
#ifndef XSYNC_SERVER_QUEUES_MAX
#  define XSYNC_SERVER_QUEUES_MAX    4096
#elif XSYNC_SERVER_QUEUES_MAX > 16384
#  error XSYNC_SERVER_QUEUES_MAX is more than 16384
#elif XSYNC_SERVER_QUEUES_MAX < 1024
#  error XSYNC_SERVER_QUEUES_MAX is less than 1024
#endif


#ifndef XSYNC_CLIENTID_MAXLEN
#  define XSYNC_CLIENTID_MAXLEN     40
#endif


#ifndef XSYNC_PATHFILE_MAXLEN
#  define XSYNC_PATHFILE_MAXLEN     0xff
#endif


/**
 * 支持的最大的服务器数目. 根据需要可以更改这个值 (不可以大于 30)
 */
#ifndef XSYNC_SERVER_MAXID
#  define XSYNC_SERVER_MAXID             10
#elif XSYNC_SERVER_MAXID > 30
#  error XSYNC_SERVER_MAXID is more than 30
#elif XSYNC_SERVER_MAXID < 8
#  error XSYNC_SERVER_MAXID is less than  8
#endif


#ifndef XSYNC_ERRBUF_MAXLEN
#  define XSYNC_ERRBUF_MAXLEN        255
#endif


#ifndef XSYNC_HOSTNAME_MAXLEN
#  define XSYNC_HOSTNAME_MAXLEN      128
#endif


#ifndef XSYNC_PATH_HASH_MAXID
#  define XSYNC_PATH_HASH_MAXID     0xff
#endif


#ifndef XSYNC_ENTRY_HASH_MAXID
#  define XSYNC_ENTRY_HASH_MAXID    1023
#endif


#if defined(__cplusplus)
}
#endif

#endif /* XSYNC_CONFIG_H_ */
