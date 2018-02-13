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
 * server_conn.c
 *   server opts and server connection
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-02-12
 * update: 2018-02-12
 *
 */

#include "client_api.h"

#include "server_conn.h"

#include "../common/common_util.h"


extern XS_RESULT XS_server_conn_create (const xs_server_opts *servOpts, char clientid[40], XS_server_conn *outSConn)
{
    XS_server_conn sconn;

    sconn = (XS_server_conn) mem_alloc(1, sizeof(struct xs_server_conn_t) + sizeof(xs_server_opts));

    randctx_init(&sconn->rctx, servOpts->magic + time(0));

    memcpy(sconn->srvopts, servOpts, sizeof(xs_server_opts));

    do {




    } while(0);

    *outSConn = (XS_server_conn) RefObjectInit(sconn);

    return XS_SUCCESS;
}


extern XS_VOID XS_server_conn_release (XS_server_conn *inSConn)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inSConn, server_conn_delete);
}
