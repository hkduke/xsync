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
 * server_conf.c
 *
 *   server config file api
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-02-02
 * update: 2018-02-02
 *
 */
#include "server_api.h"
#include "server_conf.h"

#include "../xsync-xmlconf.h"
#include "../common/readconf.h"
#include "../common/common_util.h"


extern void xs_server_delete (void *pv)
{
    XS_server server = (XS_server) pv;

    LOGGER_TRACE("pause and destroy timer");
    mul_timer_pause();
    if (mul_timer_destroy() != 0) {
        LOGGER_ERROR("mul_timer_destroy failed");
    }

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&server->condition);

    if (server->events) {
        LOGGER_DEBUG("destroy epoll_events");

        fcntl(server->listenfd, F_SETFL, server->listenfd_oldopt);

        close(server->listenfd);
        close(server->epollfd);

        free(server->events);

        server->epollfd = -1;
        server->listenfd = -1;
        server->events = 0;
    }

    if (server->pool) {
        LOGGER_DEBUG("threadpool_destroy");
        threadpool_destroy(server->pool, 0);
    }

    if (server->thread_args) {
        int i;
        perthread_data *pdata;

        LOGGER_DEBUG("destroy thread_args");

        for (i = 0; i < server->threads; ++i) {
            pdata = server->thread_args[i];
            server->thread_args[i] = 0;

            free(pdata);
        }

        free(server->thread_args);
    }

    XS_server_clear_client_sessions(server);

    LOGGER_TRACE("%p", server);

    free(server);
}
