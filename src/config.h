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
 * config.h
 *
 *   Compile Definitions can be overwrite by SRC_DEFS in:
 *      client/client.mk
 *      server/server.mk
 *
 * author: master@pepstack.com
 *
 * create: 2018-01-21
 * update: 2018-01-24
 */

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#ifndef IO_BUFSIZE
#  define IO_BUFSIZE        8192
#endif


/**
 * 8192 can hold 30 (num_inevents=30) inotify_events in minimum.
 *   INEVENT_BUFSIZE = num_inevents * (sizeof(struct inotify_event) + NAME_MAX + 1)
 * If you want to hold more events you should increment INEVENT_BUFSIZE (for examle: 16384).
 * Note that you should not set it less than 4096 or more than 65536
 */
#ifndef INEVENT_BUFSIZE
#  define INEVENT_BUFSIZE   8192
#endif


#ifndef IDS_MAXLEN
#  define IDS_MAXLEN          40
#endif


#ifndef PATHFILE_MAXLEN
#  define PATHFILE_MAXLEN    0xff
#endif


#ifndef SERVER_MAXID
#  define SERVER_MAXID       0xff
#endif


#ifndef HOSTNAME_MAXLEN
#  define HOSTNAME_MAXLEN    128
#endif


#ifndef WPATH_HASH_MAXID
#  define WPATH_HASH_MAXID    0xff
#endif


#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_H_INCLUDED */
