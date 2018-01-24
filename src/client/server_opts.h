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

typedef struct xs_server_opts_t
{
    union {
        int magic;
        int servers;
    };

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    ushort port;

    struct sockconn_opts  sockopts;
} * XS_server_opts, xs_server_opts_t;


__attribute__((used))
static void server_opt_init (XS_server_opts opts)
{
    bzero(opts, sizeof(xs_server_opts_t));

    strcpy(opts->host, "pepstack.com");

    opts->port = 8960;

    sockconn_opts_init_default(&opts->sockopts);
}


#if defined(__cplusplus)
}
#endif

#endif /* SERVER_OPTS_H_INCLUDED */
