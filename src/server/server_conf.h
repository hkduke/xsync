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

#ifndef SERVER_CONF_H_INCLUDED
#define SERVER_CONF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common/common_util.h"

// #include "client_info.h"

#include "../xsync-error.h"
#include "../xsync-config.h"


typedef struct perthread_data
{
    int    threadid;

    int    sessions[XSYNC_SERVER_MAXID + 1];
    int    sockfds[XSYNC_SERVER_MAXID + 1];

    byte_t buffer[XSYNC_IO_BUFSIZE];
} perthread_data;


/**
 * xs_server_t type
 */
typedef struct xs_server_t
{
    EXTENDS_REFOBJECT_TYPE();


} * XS_server, xs_server_t;



#if defined(__cplusplus)
}
#endif

#endif /* SERVER_CONF_H_INCLUDED */
