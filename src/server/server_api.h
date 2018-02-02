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

#include "../xsync-error.h"
#include "../xsync-config.h"


typedef struct xs_server_t * XS_server;


typedef struct serverapp_opts
{
    // singleton

    char *startcmd;

    int isdaemon;

    int threads;
    int queues;

    char config[XSYNC_PATHFILE_MAXLEN + 1];
} serverapp_opts;


/**
 * XS_server application api
 */



/**
 * XS_server configuration api
 *
 * implemented in:
 *   server_conf.c
 */



/**
 * XS_server internal api
 */



#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_API_H_INCLUDED */
