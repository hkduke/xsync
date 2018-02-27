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

#include "client_session.h"

/**
 * http://www.thinkingyu.com/articles/
 */
#include <zdb/zdb.h>


/**
 * xs_server_t type
 */
typedef struct xs_server_t
{
    EXTENDS_REFOBJECT_TYPE();

    pthread_cond_t  condition;

    ref_counter_t session_counter;

    int ssl;

    int maxclients;
    int timeout_ms;

    /**
     * epoll data
     */
    int epollfd;
    int listenfd;
    int listenfd_oldopt;

    int numevents;
    struct epoll_event *events;

    /**
     * thread pool specific
     */
    int queues;

    /** number of threads */
    int threads;

    /** thread pool for handlers */
    threadpool_t *pool;
    void        **thread_args;

    /**
     * hlist for client_session:
     *   it's key also known as session_id is the address of
     *   pointer to XS_client_session object.
     *
     * 根据客户端传过来的 session_id 查找得到真正的 client_session 对象
     * 如果不存在则拒绝客户端的访问，断开 socket 连接。
     */
    struct hlist_head client_hlist[XSYNC_CLIENT_SESSION_HASHMAX + 1];

    /**
     * msg buffer
     */
    char msgbuf[XSYNC_ERRBUF_MAXLEN + 1];
} xs_server_t;


extern void xs_server_delete (void *pv);

#if defined(__cplusplus)
}
#endif

#endif /* SERVER_CONF_H_INCLUDED */
