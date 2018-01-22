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

#ifndef SERVER_OPTS_H_INCLUDED
#define SERVER_OPTS_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common.h"

typedef struct xsync_server_opts
{
    union {
        int serverid;
        int servers;
    };

    int magic;

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    ushort port;

    struct sockconn_opts  sockopts;
} xsync_server_opts;


__attribute__((unused))
static void server_opts_init (xsync_server_opts * opts, int id)
{
    bzero(opts, sizeof(xsync_server_opts));

    opts->serverid = id;

    strcpy(opts->host, "pepstack.com");
    opts->port = 18960;

    sockconn_opts_init_default(&opts->sockopts);
}


#if defined(__cplusplus)
}
#endif

#endif /* SERVER_OPTS_H_INCLUDED */
