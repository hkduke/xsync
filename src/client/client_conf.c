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

#include <client.h>


static inline void free_xsync_client (void *pv)
{
    int i, sid;
    xsync_client * client = (xsync_client *) pv;

    LOGGER_TRACE("pthread_cond_destroy");
    pthread_cond_destroy(&client->condition);

    int infd = client->infd;
    if (infd != -1) {
        client->infd = -1;

        LOGGER_DEBUG("close inotify");
        close(infd);
    }

    if (client->pool) {
        LOGGER_DEBUG("threadpool_destroy");
        threadpool_destroy(client->pool, 0);
    }

    if (client->thread_args) {
        for (i = 0; i < client->threads; ++i) {
            perthread_data * perdata = client->thread_args[i];
            client->thread_args[i] = 0;

            for (sid = 1; sid <= client->servers_opts->servers; sid++) {
                int sockfd = perdata->sockfds[sid];

                perdata->sockfds[sid] = SOCKAPI_ERROR_SOCKET;

                if (sockfd != SOCKAPI_ERROR_SOCKET) {
                    close(sockfd);
                }
            }

            free(perdata);
        }

        free(client->thread_args);
    }

    LOGGER_TRACE("object=%#lx", (uint64_t)(void*)(client));

    free(client);
}


int xsync_client_create (const char * xmlconf, xsync_client ** outClient)
{
    int i, sid, err;

    xsync_client * client;

    int SERVERS = 4;
    int THREADS = 20;
    int QUEUES = 256;
    uint16_t BUFSIZE = 8192;

    *outClient = 0;

    client = (xsync_client *) mem_alloc(1, sizeof(xsync_client));

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&client->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));
        free((void*) client);
        return (-1);
    };

    client->servers_opts->servers = SERVERS;
    client->bufsize = BUFSIZE;
    client->threads = THREADS;
    client->queues = QUEUES;
    client->sendfile = 1;
    client->infd = -1;

    /* populate server_opts from xmlconf */
    for (sid = 1; sid <= SERVERS; sid++) {
        xsync_server_opts * server_opts = xsync_client_get_server_by_sid(client, sid);

        server_opts_init(server_opts, sid);

        // TODO:
    }

    /* create per thread data */
    client->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data * perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data) + sizeof(char) * BUFSIZE);

        perdata->threadid = i + 1;
        perdata->bufsize = BUFSIZE;

        // TODO: socket
        for (sid = 1; sid <= SERVERS; sid++) {
            xsync_server_opts * server = xsync_client_get_server_by_sid(client, sid);

            int sockfd = opensocket(server->host, server->port, server->sockopts.timeosec, server->sockopts.nowait, &err);

            if (sockfd == SOCKAPI_ERROR_SOCKET) {
                LOGGER_ERROR("[thread_%d] connect server-%d (%s:%d) error(%d): %s",
                    perdata->threadid,
                    server->serverid,
                    server->host,
                    server->port,
                    err,
                    strerror(errno));
            } else {
                LOGGER_INFO("[thread_%d] connected server-%d (%s:%d)",
                    perdata->threadid,
                    server->serverid,
                    server->host,
                    server->port);
            }

            perdata->sockfds[sid] = sockfd;
        }

        client->thread_args[i] = (void*) perdata;
    }

    LOGGER_DEBUG("threadpool_create: (threads=%d, queues=%d)", THREADS, QUEUES);
    client->pool = threadpool_create(THREADS, QUEUES, client->thread_args, 0);
    if (! client->pool) {
        LOGGER_FATAL("threadpool_create error: Out of memory");
        free_xsync_client((void*) client);
        return (-1);
    }

    /* http://www.cnblogs.com/jimmychange/p/3498862.html */
    LOGGER_DEBUG("inotify_init");
    client->infd = inotify_init();
    if (client->infd == -1) {
        LOGGER_ERROR("inotify_init() error(%d): %s", errno, strerror(errno));
        free_xsync_client((void*) client);
        return (-1);
    }

    if ((err = RefObjectInit(client)) == 0) {
        *outClient = client;
        LOGGER_TRACE("object=%#lx", (uint64_t)(void*)(client));
        return 0;
    } else {
        LOGGER_FATAL("RefObjectInit error(%d): %s", err, strerror(err));
        free_xsync_client((void*) client);
        return (-1);
    }
}


void xsync_client_release (xsync_client ** inClient)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) inClient, free_xsync_client);
}
