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
 * update: 2018-01-24
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


#ifndef XSYNC_DEF_SENDFILE
#  define XSYNC_DEF_SENDFILE         1
#endif


#ifndef XSYNC_CLIENTID_MAXLEN
#  define XSYNC_CLIENTID_MAXLEN     40
#endif


#ifndef XSYNC_PATHFILE_MAXLEN
#  define XSYNC_PATHFILE_MAXLEN     0xff
#endif


/**
 * 支持的最大的服务器数目. 根据需要可以更改这个值
 */
#ifndef XSYNC_SERVER_MAXID
#  define XSYNC_SERVER_MAXID          64
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
