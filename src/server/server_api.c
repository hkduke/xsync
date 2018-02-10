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

#include "server_api.h"
#include "server_conf.h"

#include "../common/common_util.h"

__no_warning_unused(static)
int epapi_on_epevent_callback (int eventid, epevent_data_t *epdata, void *arg)
{
    //XS_server server = (XS_server) arg;

    switch (eventid) {
    case EPEVENT_MSG_ERROR:

        switch (epdata->status) {
        case EPOLL_WAIT_TIMEOUT:
            LOGGER_TRACE("EPOLL_WAIT_TIMEOUT");
            return 1;

        case EPOLL_WAIT_EINTR:
            LOGGER_WARN("EPOLL_WAIT_TIMEOUT");
            return 1;

        case EPOLL_WAIT_ERROR:
            LOGGER_FATAL("EPOLL_WAIT_ERROR: %s", epdata->msg);
            return 0;

        case EPOLL_WAIT_OK:
            LOGGER_TRACE("EPOLL_WAIT_OK");
            return 1;

        case EPOLL_EVENT_NOTREADY:
            LOGGER_WARN("EPOLL_EVENT_NOTREADY");
            return 1;

        case EPOLL_ACPT_EAGAIN:
            LOGGER_WARN("EPOLL_ACPT_EAGAIN");
            return 0;

        case EPOLL_ACPT_EWOULDBLOCK:
            LOGGER_WARN("EPOLL_ACPT_EWOULDBLOCK");
            return 0;

        case EPOLL_ACPT_ERROR:
            LOGGER_WARN("EPOLL_ACPT_ERROR: %s", epdata->msg);
            return 0;

        case EPOLL_NAMEINFO_ERROR:
            LOGGER_WARN("EPOLL_NAMEINFO_ERROR: %s", epdata->msg);
            // 0 or -1
            return 0;

        case EPOLL_SETSOCKET_ERROR:
            LOGGER_WARN("EPOLL_SETSOCKET_ERROR: %s", epdata->msg);
            // 0 or -1
            return 0;

        case EPOLL_SETSTATUS_ERROR:
            LOGGER_WARN("EPOLL_SETSTATUS_ERROR: %s", epdata->msg);
            // 0 or -1
            return -1;

        case EPOLL_ERROR_UNEXPECT:
            LOGGER_WARN("EPOLL_ERROR_UNEXPECT");
            // go on
            return 1;
        }

    case EPEVENT_PEER_ADDRIN:
        //TODO: 1: accept, 0: decline
        // 0 or 1
        return 1;

    case EPEVENT_PEER_INFO:
        //TODO: 1: accept, 0: decline
        return 1;

    case EPEVENT_PEER_ACPTED:
        //TODO: 1: accept, 0: decline
        return 1;

    case EPEVENT_PEER_POLLIN:
        //TODO: 1: accept, 0: decline
        return 1;
    }

    return (-1);
}


extern XS_RESULT XS_server_create (serverapp_opts *opts, XS_server *outServer)
{
    int i, old_sockopt;

    int MAXCLIENTS = opts->maxclients;
    int THREADS = opts->threads;
    int QUEUES = opts->queues;
    int MAXEVENTS = opts->maxclients;
    int BACKLOGS = opts->somaxconn;
    int TIMEOUTMS = opts->timeout_ms;

    XS_server server;

    *outServer = 0;

    server = (XS_server) mem_alloc(1, sizeof(xs_server_t));
    assert(server->thread_args == 0);

    __interlock_release(&server->session_counter);

    /* PTHREAD_PROCESS_PRIVATE = 0 */
    LOGGER_TRACE("pthread_cond_init(%d)", PTHREAD_PROCESS_PRIVATE);
    if (0 != pthread_cond_init(&server->condition, PTHREAD_PROCESS_PRIVATE)) {
        LOGGER_FATAL("pthread_cond_init() error(%d): %s", errno, strerror(errno));
        free((void*) server);
        return XS_ERROR;
    }

    /* init dhlist for client_session */
    LOGGER_TRACE("hlist_init client_session");
    for (i = 0; i <= XSYNC_CLIENT_SESSION_HASHMAX; i++) {
        INIT_HLIST_HEAD(&server->client_hlist[i]);
    }

    LOGGER_INFO("threads=%d queues=%d maxclients=%d", THREADS, QUEUES, MAXCLIENTS);

    /* create per thread data */
    server->thread_args = (void **) mem_alloc(THREADS, sizeof(void*));

    for (i = 0; i < THREADS; ++i) {
        perthread_data *perdata = (perthread_data *) mem_alloc(1, sizeof(perthread_data));

        perdata->threadid = i + 1;

        server->thread_args[i] = (void*) perdata;
    }

    LOGGER_DEBUG("threadpool_create: threads=%d queues=%d", THREADS, QUEUES);
    server->pool = threadpool_create(THREADS, QUEUES, server->thread_args, 0);
    if (! server->pool) {
        LOGGER_FATAL("threadpool_create error: out of memory");

        xs_server_delete((void*) server);

        // 失败退出程序
        exit(XS_ERROR);
    }

    LOGGER_INFO("epapi_create_and_bind: %s:%s", opts->host, opts->port);
    do {
        int listenfd, epollfd;
        struct epoll_event *events;

        listenfd = epapi_create_and_bind(opts->host, opts->port, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (listenfd == -1) {
            LOGGER_FATAL("epapi_create_and_bind error: %s", server->msgbuf);

            xs_server_delete((void*) server);

            // 失败退出程序
            exit(XS_ERROR);
        }

        old_sockopt = epapi_set_nonblock(listenfd, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (old_sockopt == -1) {
            LOGGER_FATAL("epapi_set_nonblock error: %s", server->msgbuf);

            close(listenfd);
            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }

        LOGGER_INFO("epapi_init_epoll_event (backlog=%d)", BACKLOGS);
        epollfd = epapi_init_epoll_event(listenfd, BACKLOGS, server->msgbuf, XSYNC_ERRBUF_MAXLEN);
        if (epollfd == -1) {
            LOGGER_FATAL("epapi_init_epoll_event error: %s", server->msgbuf);

            fcntl(listenfd, F_SETFL, old_sockopt);
            close(listenfd);
            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }

        LOGGER_INFO("create epoll events (maxevents=%d)", MAXEVENTS);
        events = (struct epoll_event *) calloc(MAXEVENTS, sizeof(struct epoll_event));
        if (! events) {
            LOGGER_FATAL("out of memory");

            fcntl(listenfd, F_SETFL, old_sockopt);
            close(listenfd);
            close(epollfd);
            xs_server_delete((void*) server);

            exit(XS_ERROR);
        }
    } while(0);

    /**
     * here we got success
     *   output XS_server
     */
    server->maxclients = MAXCLIENTS;
    server->threads = THREADS;
    server->queues = QUEUES;
    server->numevents = MAXEVENTS;
    server->timeout_ms = TIMEOUTMS;

    server->listenfd_oldopt = old_sockopt;

    *outServer = (XS_server) RefObjectInit(server);

    LOGGER_TRACE("%p", server);

    mul_timer_start();

    return XS_SUCCESS;
}


extern XS_VOID XS_server_release (XS_server *pServer)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) pServer, xs_server_delete);
}


extern int XS_server_lock (XS_server server)
{
    int err = RefObjectTryLock(server);

    if (err) {
        LOGGER_WARN("RefObjectTryLock error(%d): %s", err, strerror(err));
    }

    return err;
}


extern void XS_server_unlock (XS_server server)
{
    RefObjectUnlock(server);
}


extern XS_VOID XS_server_bootstrap (XS_server server)
{
    LOGGER_INFO("server starting ...");

    epapi_loop_epoll_events(server->epollfd,
        server->listenfd,
        server->events,
        server->numevents,
        server->timeout_ms,
        epapi_on_epevent_callback, server);

    LOGGER_FATAL("server stopped.");
}


extern XS_VOID XS_server_clear_client_sessions (XS_server server)
{
    int hash;
    struct hlist_node *hp, *hn;

    LOGGER_TRACE("hlist clear");

    for (hash = 0; hash <= XSYNC_CLIENT_SESSION_HASHMAX; hash++) {
        hlist_for_each_safe(hp, hn, &server->client_hlist[hash]) {
            struct xs_client_session_t *client = hlist_entry(hp, struct xs_client_session_t, i_hash);

            hlist_del(&client->i_hash);

            XS_client_session_release(&client);
        }
    }
}
