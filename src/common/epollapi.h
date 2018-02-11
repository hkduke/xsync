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
 * epollapi.h
 *
 * author: master@pepstack.com
 *
 * reference:
 *
 *  - linux socket:
 *      https://linux.die.net/man/3/getaddrinfo
 *
 *  - epoll man:
 *      https://linux.die.net/man/4/epoll
 *
 *  - epoll with thread
 *      http://blog.csdn.net/liuxuejiang158blog/article/details/12422471
 *
 *  - epoll with ipv6
 *      http://www.cnblogs.com/haippy/archive/2012/01/09/2317269.html
 *
 * create: 2018-02-07
 * update: 2018-02-09
 */

#ifndef EPOLL_API_H_INCLUDED
#define EPOLL_API_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <assert.h>
#include <sys/epoll.h>

#include "sockapi.h"
#include "byteorder.h"


#ifndef EPAPI_BUFSIZE
#  define EPAPI_BUFSIZE    SOCKAPI_BUFSIZE
#endif


#define EPEVENT_MSG_SUCCESS       0

/**
 * msgid = EPEVENT_MSG_ERROR
 *   程序向调用者报告状态，调用者返回默认值(expect)即可
 */
#define EPEVENT_MSG_ERROR       (-1)

/**
 * 如果 msgid = EPEVENT_MSG_ERROR
 *    则 epdata->status 为以下值之一:
 */
#define EPOLL_WAIT_TIMEOUT      (-2)
#define EPOLL_WAIT_EINTR        (-3)
#define EPOLL_WAIT_ERROR        (-4)
#define EPOLL_WAIT_OK           (-5)
#define EPOLL_EVENT_NOTREADY    (-6)
#define EPOLL_ACPT_ERROR        (-7)
#define EPOLL_ACPT_EWOULDBLOCK  (-8)
#define EPOLL_ACPT_EAGAIN       (-9)
#define EPOLL_NAMEINFO_ERROR    (-10)
#define EPOLL_SETNONBLK_ERROR   (-11)
#define EPOLL_ADDONESHOT_ERROR   (-12)
#define EPOLL_ERROR_UNEXPECT    (-13)

/**
 * msgid 为以下值之一，要求调用者处理
 *   epdata->status 应为 0
 */
#define EPEVENT_PEER_ADDRIN       1
#define EPEVENT_PEER_INFO         2
#define EPEVENT_PEER_ACPTED       3
#define EPEVENT_PEER_POLLIN       4

/**
 * 回调函数数据，不可以在多线程中使用
 */
typedef struct epevent_data_t * epevent_data;

/**
 * epapi_on_epevent_cb 回调函数返回值
 */
#define EPCB_EXPECT_NEXT       1
#define EPCB_EXPECT_BREAK      0
#define EPCB_EXPECT_END     (-1)

/**
 * 事件回调函数，返回值必须是：
 *    EPCB_EXPECT_NEXT   (1)
 *    EPCB_EXPECT_BREAK  (0)
 *    EPCB_EXPECT_END  (-1)
 */
typedef int (* epapi_on_epevent_cb) (int msgid, int expect, epevent_data epdata, void *arg);


typedef struct epevent_data_t
{
    int epollfd;
    int connfd;

    union {
        struct {
            char hbuf[NI_MAXHOST];
            char sbuf[NI_MAXSERV];

            struct sockaddr in_addr;
            socklen_t in_len;
        };

        struct {
            char msg[NI_MAXHOST + NI_MAXSERV];
            int status;
        };
    };
} epevent_data_t;


/**
 * epapi_on_epevent_cb
 *
 * returns:
 *   1: all is ok
 *   0: break the nearest loop
 *  -1: error and exit.
 */
#define EPCB_EXPECT_NEXT       1
#define EPCB_EXPECT_BREAK      0
#define EPCB_EXPECT_END     (-1)


typedef struct pollin_data_t
{
    int epollfd;
    int connfd;

    void *arg;
} pollin_data_t;


__attribute__((used))
static int epapi_create_and_bind (const char *node, const char *port, char *errmsg, ssize_t msgsize)
{
    int err;

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    int listenfd = -1;

    *errmsg = 0;

    bzero(&hints, sizeof (hints));

    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    err = getaddrinfo(node, port, &hints, &result);
    if (err != 0) {
        snprintf(errmsg, msgsize, "getaddrinfo error(%d): %s", err, gai_strerror(err));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listenfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (listenfd == -1) {
            continue;
        }

        err = bind(listenfd, rp->ai_addr, rp->ai_addrlen);

        if (err == 0) {
            /* We managed to bind successfully! */
            break;
        }

        close(listenfd);
    }

    if (rp == NULL) {
        snprintf(errmsg, msgsize, "socket error(%d): %s", errno, strerror(err));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    freeaddrinfo(result);

    return listenfd;
}


__attribute__((used))
static int epapi_set_nonblock (int listenfd, char *errmsg, ssize_t msgsize)
{
    int oldopt;

    *errmsg = 0;

    oldopt = fcntl(listenfd, F_GETFL, 0);
    if (oldopt == -1) {
        snprintf(errmsg, msgsize, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    if (fcntl(listenfd, F_SETFL, oldopt | O_NONBLOCK) == -1) {
        snprintf(errmsg, msgsize, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    return oldopt;
}


/**
 * oneshotflag = 0 or EPOLLONETSHOT
 **/
__attribute__((used))
static int epapi_add_epoll (int epollfd, int fd, int oneshotflag, char *errmsg, ssize_t msgsize)
{
    struct epoll_event event = {0};

    *errmsg = 0;

    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | oneshotflag;

    if (epoll_ctl (epollfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        snprintf(errmsg, msgsize, "epoll_ctl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    return 0;
}


__attribute__((used))
static int epapi_modify_epoll (int epollfd, int fd, int oneshotflag, char *errmsg, ssize_t msgsize)
{
    struct epoll_event event = {0};

    *errmsg = 0;

    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | oneshotflag;

    if (epoll_ctl (epollfd, EPOLL_CTL_MOD, fd, &event) == -1) {
        snprintf(errmsg, msgsize, "epoll_ctl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    return 0;
}


__attribute__((used))
static int epapi_init_epoll_event (int listenfd, int backlog, char *errmsg, ssize_t msgsize)
{
    int epollfd;

    *errmsg = 0;

    /**
     * 如果要增大 SOMAXCONN, 需要修改内核参数:
     * backlog = SOMAXCONN
     *   $ sudo echo 2048 > /proc/sys/net/core/somaxconn
     *   $ sudo vi /etc/sysctl.conf:
     *   net.core.somaxconn=2048
     *   $ sysctl -p
     **/

    if (listen (listenfd, backlog) == -1) {
        snprintf(errmsg, msgsize, "listen error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    epollfd = epoll_create1 (EPOLL_CLOEXEC);
    if (epollfd == -1) {
        snprintf(errmsg, msgsize, "epoll_create1 error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    if (epapi_add_epoll (epollfd, listenfd, 0, errmsg, msgsize) == -1) {
        close(epollfd);
        return -1;
    }

    return epollfd;
}


/**
 * epapi_pollin_handler
 *
 */
__attribute__((used))
static void * epapi_pollin_handler (void* threadarg)
{
    int err, size;

    pollin_data_t *pdata = (pollin_data_t *) threadarg;

    // 用户定义的数据
    // void *arg = pdata->arg;

    // 子线程接收socket上的数据并重置事件
    int connfd = pdata->connfd;

    // 事件表描述符从arg参数得来
    int epollfd = pdata->epollfd;

    free(pdata);

    printf("start thread connfd: %d\n", connfd);

    char buf[EPAPI_BUFSIZE];

    while (1) {
        // 接收数据
        size = recv(connfd, buf, EPAPI_BUFSIZE, 0);

        if (size == 0) {
            /**
             * 数据接收完毕
             * close the descriptor will make epoll remove it
             *   from the set of descriptors which are monitored.
             **/
            close(connfd);

            printf("* close sock: %d\n", connfd);

            break;
        } else if (size < 0) {
            if (errno == EAGAIN) {
                // 并非网络出错，而是可以再次注册事件
                err = epapi_modify_epoll(epollfd, connfd, EPOLLONESHOT, buf, sizeof(buf));

                if (err == -1) {
                    printf("epapi_modify_epoll error: %s\n", buf);
                    close(connfd);
                }

                break;
            }
        } else {
            buf[size] = 0;

            printf("buf: %s\n", buf);

            // 采用睡眠是为了在1s内若有新数据到来则该线程继续处理，否则线程退出
            // sleep(1);
        }
    }

    printf("* thread exit.\n");

    return 0;
}


/**
 * epapi_on_epevent_interactive
 *   默认回调函数
 */
__attribute__((used))
static int epapi_on_epevent_interactive (int msgid, int expect, epevent_data_t *epdata, void *arg)
{
    switch (msgid) {
    case EPEVENT_MSG_ERROR:
        do {
            switch (epdata->status) {
            case EPOLL_WAIT_TIMEOUT:
                printf("EPOLL_WAIT_TIMEOUT");
                break;

            case EPOLL_WAIT_EINTR:
                printf("EPOLL_WAIT_TIMEOUT");
                break;

            case EPOLL_WAIT_ERROR:
                printf("EPOLL_WAIT_ERROR: %s", epdata->msg);
                break;

            case EPOLL_WAIT_OK:
                printf("EPOLL_WAIT_OK");
                break;

            case EPOLL_EVENT_NOTREADY:
                printf("EPOLL_EVENT_NOTREADY");
                break;

            case EPOLL_ACPT_EAGAIN:
                printf("EPOLL_ACPT_EAGAIN");
                break;

            case EPOLL_ACPT_EWOULDBLOCK:
                printf("EPOLL_ACPT_EWOULDBLOCK");
                break;

            case EPOLL_ACPT_ERROR:
                printf("EPOLL_ACPT_ERROR: %s", epdata->msg);
                return 0;

            case EPOLL_NAMEINFO_ERROR:
                printf("EPOLL_NAMEINFO_ERROR: %s", epdata->msg);
                break;

            case EPOLL_SETNONBLK_ERROR:
                printf("EPOLL_SETNONBLK_ERROR: %s", epdata->msg);
                break;

            case EPOLL_ADDONESHOT_ERROR:
                printf("EPOLL_ADDONESHOT_ERROR: %s", epdata->msg);
                break;

            case EPOLL_ERROR_UNEXPECT:
                printf("EPOLL_ERROR_UNEXPECT");
                break;
            }
        } while(0);

        return expect;

    case EPEVENT_PEER_ADDRIN:
        printf("EPEVENT_PEER_ADDRIN\n");
        break;

    case EPEVENT_PEER_INFO:
        printf("EPEVENT_PEER_INFO\n");
        break;

    case EPEVENT_PEER_ACPTED:
        printf("EPEVENT_PEER_ACPTED\n");
        break;

    case EPEVENT_PEER_POLLIN:
        do {
            printf("EPEVENT_PEER_POLLIN\n");

            // 客户端有数据发送的事件发生
            pthread_t thread;

            pollin_data_t *pdata = (pollin_data_t *) malloc(sizeof(*pdata));

            pdata->arg = arg;
            pdata->epollfd = epdata->epollfd;
            pdata->connfd = epdata->connfd;

            // 调用工作者线程处理数据
            if (pthread_create(&thread, NULL, epapi_pollin_handler, (void*) pdata) == 0) {
                pthread_join(thread, NULL);
            } else {
                printf("pthread_create error(%d): %s\n", errno, strerror(errno));
                expect = EPCB_EXPECT_END;
            }
            break;
        } while(0);
    }

    return expect;
}


__attribute__((used))
static int epapi_loop_epoll_events (int epollfd, int listenfd,
    struct epoll_event *events, int maxevents, int timeout_ms,
    epapi_on_epevent_cb epapi_on_epevent, void *arg)
{
    int i, numfds, ret, sockfd;

    struct epoll_event *ev;

    epevent_data_t  epdata = {0};

    epdata.epollfd = epollfd;
    epdata.connfd = -1;

    while (1) {
        *epdata.msg = 0;

        //  epoll_pwait: 2.6.19 and later
        numfds = epoll_pwait (epollfd, events, maxevents, timeout_ms, 0);

        if (numfds <= 0) {
            if (numfds == 0) {
                epdata.status = EPOLL_WAIT_TIMEOUT;

                if (epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_NEXT, &epdata, arg) == EPCB_EXPECT_NEXT) {
                    // should continue
                    continue;
                } else {
                    break;
                }
            } else if (errno == EINTR) {
                epdata.status = EPOLL_WAIT_EINTR;

                if (epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_NEXT, &epdata, arg) == EPCB_EXPECT_NEXT) {
                    // should continue
                    continue;
                } else {
                    break;
                }
            } else {
                epdata.status = EPOLL_WAIT_ERROR;
                snprintf(epdata.msg, sizeof(epdata.msg), "epoll_pwait error(%d): %s", errno, strerror(errno));

                if (epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_BREAK, &epdata, arg) != EPCB_EXPECT_NEXT) {
                    // should break
                    break;
                } else {
                    // should not continue
                    continue;
                }
            }
        }

        epdata.status = EPOLL_WAIT_OK;

        if (epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_NEXT, &epdata, arg) != EPCB_EXPECT_NEXT) {
            // should not break
            break;
        }

        for (i = 0; i < numfds; i++) {
            ev = &events[i];

            sockfd = ev->data.fd;

            *epdata.msg = 0;

            if ( (ev->events & EPOLLERR) || (ev->events & EPOLLHUP) || (! (ev->events & EPOLLIN)) ) {
                // ev->data.fd = -1;
                close(sockfd);

                /**
                 * epoll error has occured on this socket or the socket is not ready for reading
                 */
                epdata.status = EPOLL_EVENT_NOTREADY;

                ret = epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_NEXT, &epdata, arg);
                if (ret == EPCB_EXPECT_NEXT) {
                    continue;
                } else if (ret == EPCB_EXPECT_BREAK) {
                    break;
                } else {
                    goto onerror_exit;
                }
            }

            if (listenfd == sockfd) {
                /**
                 * We have a notification on the listening socket, which means one or more incoming connections.
                 */
                epdata.in_len = sizeof(epdata.in_addr);
                epdata.connfd = accept(listenfd, &epdata.in_addr, &epdata.in_len);

                if (epdata.connfd == -1) {
                    if (errno == EAGAIN) {
                        /** We have processed all incoming connections */
                        epdata.status = EPOLL_ACPT_EAGAIN;
                    } else if (errno == EWOULDBLOCK) {
                        /** We have processed all incoming connections */
                        epdata.status = EPOLL_ACPT_EWOULDBLOCK;
                    } else {
                        epdata.status = EPOLL_ACPT_ERROR;
                        snprintf(epdata.msg, sizeof(epdata.msg), "accept error(%d): %s", errno, strerror(errno));
                    }

                    ret = epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_BREAK, &epdata, arg);
                    if (ret == EPCB_EXPECT_BREAK || ret == EPCB_EXPECT_NEXT) {
                        break;
                    } else {
                        goto onerror_exit;
                    }
                }

                assert(epdata.connfd != -1);
                ret = epapi_on_epevent(EPEVENT_PEER_ADDRIN, EPCB_EXPECT_NEXT, &epdata, arg);

                if (ret == EPCB_EXPECT_BREAK) {
                    /** decline the coming client */
                    close(epdata.connfd);
                    break;
                } else if (ret == EPCB_EXPECT_END) {
                    close(epdata.connfd);
                    goto onerror_exit;
                }

                /**
                 * http://man7.org/linux/man-pages/man3/getnameinfo.3.html
                 */
                if ( getnameinfo( &epdata.in_addr, epdata.in_len,
                        epdata.hbuf, sizeof(epdata.hbuf),
                        epdata.sbuf, sizeof(epdata.sbuf),
                        NI_NUMERICHOST | NI_NUMERICSERV ) == 0 ) {
                    ret = epapi_on_epevent(EPEVENT_PEER_INFO, EPCB_EXPECT_NEXT, &epdata, arg);
                } else {
                    epdata.status = EPOLL_NAMEINFO_ERROR;
                    snprintf(epdata.msg, sizeof(epdata.msg), "getnameinfo error(%d): %s", errno, strerror(errno));
                    ret = epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_BREAK, &epdata, arg);
                }

                if (ret == EPCB_EXPECT_BREAK) {
                    /** decline the coming client */
                    close(epdata.connfd);
                    break;
                } else if (ret == EPCB_EXPECT_END) {
                    close(epdata.connfd);
                    goto onerror_exit;
                }

                /**
                 * Make the incoming socket nonblock and add it to the list of fds to monitor.
                 */
                if (epapi_set_nonblock(epdata.connfd, epdata.msg, sizeof(epdata.msg)) == -1) {
                    close(epdata.connfd);

                    epdata.status = EPOLL_SETNONBLK_ERROR;
                    epdata.connfd = -1;

                    if (epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_BREAK, &epdata, arg) == EPCB_EXPECT_END) {
                        goto onerror_exit;
                    }

                    break;
                }

                // 新的客户连接置为 EPOLLONESHOT 事件
                if (epapi_add_epoll(epollfd, epdata.connfd, EPOLLONESHOT, epdata.msg, sizeof(epdata.msg))  == -1) {
                    close(epdata.connfd);

                    epdata.status = EPOLL_ADDONESHOT_ERROR;
                    epdata.connfd = -1;

                    if (epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_END, &epdata, arg) == EPCB_EXPECT_END) {
                        goto onerror_exit;
                    }

                    break;
                }

                epdata.status = EPEVENT_MSG_SUCCESS;

                ret = epapi_on_epevent(EPEVENT_PEER_ACPTED, EPCB_EXPECT_NEXT, &epdata, arg);

                if (ret == EPCB_EXPECT_NEXT) {
                    // ok, do nothing
                } else if (ret == EPCB_EXPECT_BREAK) {
                    close(epdata.connfd);
                    break;
                } else if (ret == EPCB_EXPECT_END) {
                    close(epdata.connfd);
                    goto onerror_exit;
                }
            } else if (ev->events & EPOLLIN) {
                epdata.epollfd = epollfd;
                epdata.connfd = sockfd;

                ret = epapi_on_epevent(EPEVENT_PEER_POLLIN, EPCB_EXPECT_NEXT, &epdata, arg);

                if (ret == EPCB_EXPECT_NEXT) {
                    // ok, do nothing
                } else if (ret == EPCB_EXPECT_BREAK) {
                    // decline client
                    close(sockfd);
                    break;
                } else if (ret == EPCB_EXPECT_END) {
                    close(sockfd);
                    goto onerror_exit;
                }
            } else {
                /** something wrong, can go on */
                epdata.status = EPOLL_ERROR_UNEXPECT;

                ret = epapi_on_epevent(EPEVENT_MSG_ERROR, EPCB_EXPECT_NEXT, &epdata, arg);

                if (ret == EPCB_EXPECT_NEXT) {
                    // ok, do nothing
                } else if (ret == EPCB_EXPECT_BREAK) {
                    break;
                } else if (ret == EPCB_EXPECT_END) {
                    goto onerror_exit;
                }
            }
        }
    }

    return 0;

onerror_exit:
    return (-1);
}


#if defined(__cplusplus)
}
#endif

#endif /* EPOLL_API_H_INCLUDED */
