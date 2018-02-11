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

#ifndef SERVER_API_H_INCLUDED
#define SERVER_API_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#define LOGGER_CATEGORY_NAME  XSYNC_SERVER_APPNAME
#include "../common/log4c_logger.h"

#include "../common/threadlock.h"

#include "../xsync-error.h"
#include "../xsync-config.h"
#include "../xsync-protocol.h"

#include "../common/epollapi.h"


typedef struct xs_server_t * XS_server;


typedef struct xs_appopts_t
{
    // singleton
    char *startcmd;

    int from_watch;

    int timeout_ms;

    int isdaemon;
    int interactive;

    int maxclients;
    int threads;
    int queues;

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    char port[XSYNC_PORTNUMB_MAXLEN + 1];

    int somaxconn;
    int maxevents;

    char config[XSYNC_PATHFILE_MAXLEN + 1];
} xs_appopts_t;


__no_warning_unused(static)
int appopts_validate_threads (int threads)
{
    int valid_threads = threads;

    if (threads == 0 || threads == INT_MAX) {
        valid_threads = XSYNC_SERVER_THREADS;
    } else if (threads == -1) {
        valid_threads = XSYNC_THREADS_MAXIMUM;
    }

    if (valid_threads > XSYNC_THREADS_MAXIMUM) {
        LOGGER_WARN("too many THREADS(%d) expected. coerce THREADS=%d", valid_threads, XSYNC_THREADS_MAXIMUM);
        valid_threads = XSYNC_THREADS_MAXIMUM;
    } else if (valid_threads < 1) {
        LOGGER_WARN("too less THREADS(%d) expected. coerce THREADS=%d", valid_threads, 1);
        valid_threads = 1;
    }

    return valid_threads;
}


__no_warning_unused(static)
int appopts_validate_queues (int threads, int queues)
{
    int valid_queues = queues;

    threads = appopts_validate_threads(threads);

    if (queues == 0 || queues == INT_MAX) {
        valid_queues = threads * XSYNC_TASKS_PERTHREAD;
    } else if (queues == -1) {
        valid_queues = XSYNC_QUEUES_MAXIMUM;
    }

    if (valid_queues > XSYNC_QUEUES_MAXIMUM) {
        LOGGER_WARN("too many QUEUES(%d) expected. coerce QUEUES=%d", valid_queues, XSYNC_QUEUES_MAXIMUM);
        valid_queues = XSYNC_QUEUES_MAXIMUM;
    } else if (valid_queues < XSYNC_TASKS_PERTHREAD) {
        LOGGER_WARN("too less QUEUES(%d) expected. coerce QUEUES=%d", valid_queues, XSYNC_TASKS_PERTHREAD);
        valid_queues = XSYNC_TASKS_PERTHREAD;
    }

    if (valid_queues < threads * 8) {
        valid_queues = threads * 8;
    }

    return valid_queues;
}


/**
 * XS_server application api
 */
extern XS_RESULT XS_server_create (xs_appopts_t *opts, XS_server *outServer);

extern XS_VOID XS_server_release (XS_server *pServer);

extern int XS_server_lock (XS_server server);

extern void XS_server_unlock (XS_server server);

extern XS_VOID XS_server_bootstrap (XS_server server);

/**
 * XS_server configuration api
 *
 * implemented in:
 *   server_conf.c
 */



/**
 * XS_server internal api
 */

extern XS_VOID XS_server_clear_client_sessions (XS_server server);


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_API_H_INCLUDED */
