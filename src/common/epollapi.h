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

#define EPEVENT_MSG_ERROR       (-1)

#define EPOLL_WAIT_TIMEOUT      (-2)
#define EPOLL_WAIT_EINTR        (-3)
#define EPOLL_WAIT_ERROR        (-4)
#define EPOLL_WAIT_OK           (-5)
#define EPOLL_EVENT_NOTREADY       (-6)
#define EPOLL_ACPT_ERROR        (-7)
#define EPOLL_ACPT_EWOULDBLOCK  (-8)
#define EPOLL_ACPT_EAGAIN       (-9)
#define EPOLL_NAMEINFO_ERROR    (-10)
#define EPOLL_SETSOCKET_ERROR   (-11)
#define EPOLL_SETSTATUS_ERROR   (-12)
#define EPOLL_ERROR_UNEXPECT  (-13)

#define EPEVENT_PEER_ADDRIN       1
#define EPEVENT_PEER_INFO         2
#define EPEVENT_PEER_ACPTED       3
#define EPEVENT_PEER_POLLIN       4


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


typedef int (* epapi_on_epevent_cb) (int eventid, epevent_data_t *epdata, void *arg);


/**
 * epapi_on_epevent_cb
 *
 * returns:
 *   1: all is ok
 *   0: break the nearest loop
 *  -1: error and exit.
 */
__attribute__((used))
static int epapi_on_epevent_example (int eventid, epevent_data_t *epdata, void *arg)
{
    switch (eventid) {
    case EPEVENT_MSG_ERROR:

        switch (epdata->status) {
        case EPOLL_WAIT_TIMEOUT:
            printf("EPOLL_WAIT_TIMEOUT\n");
            return 1;

        case EPOLL_WAIT_EINTR:
            printf("EPOLL_WAIT_TIMEOUT\n");
            return 1;

        case EPOLL_WAIT_ERROR:
            LOGGER_FATAL("EPOLL_WAIT_ERROR: %s\n", epdata->msg);
            return 0;

        case EPOLL_WAIT_OK:
            printf("EPOLL_WAIT_OK\n");
            return 1;

        case EPOLL_EVENT_NOTREADY:
            printf("EPOLL_EVENT_NOTREADY\n");
            return 1;

        case EPOLL_ACPT_EAGAIN:
            printf("EPOLL_ACPT_EAGAIN\n");
            return 0;

        case EPOLL_ACPT_EWOULDBLOCK:
            printf("EPOLL_ACPT_EWOULDBLOCK\n");
            return 0;

        case EPOLL_ACPT_ERROR:
            printf("EPOLL_ACPT_ERROR: %s\n", epdata->msg);
            return 0;

        case EPOLL_NAMEINFO_ERROR:
            printf("EPOLL_NAMEINFO_ERROR: %s\n", epdata->msg);
            // 0 or -1
            return 0;

        case EPOLL_SETSOCKET_ERROR:
            printf("EPOLL_SETSOCKET_ERROR: %s\n", epdata->msg);
            // 0 or -1
            return 0;

        case EPOLL_SETSTATUS_ERROR:
            printf("EPOLL_SETSTATUS_ERROR: %s\n", epdata->msg);
            // 0 or -1
            return -1;

        case EPOLL_ERROR_UNEXPECT:
            printf("EPOLL_ERROR_UNEXPECT\n");
            // go on
            return 1;
        }
        break;

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

    // application error
    return (-1);
}



typedef struct epfd_pair_t
{
    int epollfd;
    int sockfd;
} epfd_pair_t;


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

//DEL
__attribute__((used))
static void * epoll_conn_handler (void* arg)
{
    int size;

    // 子线程接收socket上的数据并重置事件
    int sockfd = ((struct epfd_pair_t *) arg)->sockfd;

    // 事件表描述符从arg参数得来
    int epollfd = ((struct epfd_pair_t *) arg)->epollfd;

    free(arg);

    printf("start new thread to receive data on fd: %d\n", sockfd);

    char buf[EPAPI_BUFSIZE];

    while (1) {
        // 接收数据
        size = recv(sockfd, buf, EPAPI_BUFSIZE, 0);

        if (size == 0) {
            /**
             * 数据接收完毕
             * close the descriptor will make epoll remove it
             *   from the set of descriptors which are monitored.
             **/
            close(sockfd);

            printf("* close sock: %d\n", sockfd);

            break;
        } else if (size < 0) {
            if (errno == EAGAIN) {
                // 并非网络出错，而是可以再次注册事件
                size = epapi_modify_epoll (epollfd, sockfd, EPOLLONESHOT, buf, sizeof(buf));
                assert(size == 0);

                printf("epapi_modify_epoll = EPOLLONESHOT.\n");
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
    return NULL;
}

//DEL
__attribute__((used))
static int loop_epoll_event (int epollfd, int listenfd, struct epoll_event *events, int maxevents, int timeout_ms,
    void (* status_msg_cb)(int status, const char *msg), char *msg, ssize_t msgsize)
{
    int i, err, ret, status, sockfd;

    struct epoll_event *ev;

    *msg = 0;

    while (1) {
        //  epoll_pwait: 2.6.19 and later
        ret = epoll_pwait (epollfd, events, maxevents, timeout_ms, 0);

        if (ret <= 0) {
            status = errno;

            if (ret == 0 || errno == EINTR) {
                snprintf(msg, msgsize, "epoll_pwait returns(%d): %s.\n", errno, strerror(errno));
                msg[msgsize - 1] = '\0';

                status_msg_cb(status, msg);
                continue;
            }

            snprintf(msg, msgsize, "epoll_pwait error(%d): %s.\n", errno, strerror(errno));
            msg[msgsize - 1] = '\0';

            status_msg_cb(status, msg);
            break;
        } else {
            status_msg_cb(ret, "epoll_pwait ok.\n");
        }

        for (i = 0; i < ret; i++) {
            ev = &events[i];

            sockfd = ev->data.fd;

            if ((ev->events & EPOLLERR) || (ev->events & EPOLLHUP) || (! (ev->events & EPOLLIN))) {
                /**
                 * An error has occured on this socket fd, or the socket is not
                 *   ready for reading (why were we notified then?)
                 **/
                snprintf(msg, msgsize, "epoll error has occured on this socket, or the socket is not ready for reading.\n");
                msg[msgsize - 1] = '\0';

                // ev->data.fd = -1;
                close(sockfd);

                status_msg_cb(-1, msg);

                continue;
            }

            if (listenfd == sockfd) {
                /**
                 * We have a notification on the listening socket,
                 *   which means one or more incoming connections.
                 **/
                struct sockaddr in_addr;
                socklen_t in_len;
                int connfd;
                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                in_len = sizeof(in_addr);

                connfd = accept(listenfd, &in_addr, &in_len);

                if (connfd == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        /* We have processed all incoming connections. */
                        break;
                    } else {
                        perror ("accept");
                        break;
                    }
                }

                err = getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);

                if (err == 0) {
                    printf("Accepted connection on (host=%s, port=%s)\n", hbuf, sbuf);

                }

                /* Make the incoming socket nonblock and add it to the list of fds to monitor. */
                err = epapi_set_nonblock(connfd, msg, msgsize);
                if (err == -1) {
                    exit(-1);
                }

                // 新的客户连接置为 EPOLLONESHOT 事件
                err = epapi_add_epoll (epollfd, connfd, EPOLLONESHOT, msg, msgsize);

                assert(err == 0);
                status_msg_cb(err, msg);

            } else if (ev->events & EPOLLIN) {
                status_msg_cb(0, "epoll_conn_handler handle client...\n");

                // 客户端有数据发送的事件发生
                pthread_t thread;

                epfd_pair_t *fdpair = (epfd_pair_t *) malloc(sizeof(*fdpair));

                fdpair->epollfd = epollfd;
                fdpair->sockfd = sockfd;

                // 调用工作者线程处理数据
                pthread_create(&thread, NULL, epoll_conn_handler, (void*) fdpair);

                pthread_join(thread, NULL);
            } else {
                status_msg_cb(-1, "something wrong.\n");
                printf("something wrong.\n");
            }
        }
    }

    return 0;
}


__attribute__((used))
static int epapi_loop_epoll_events (int epollfd, int listenfd, struct epoll_event *events, int maxevents, int timeout_ms,
    int (* epapi_on_epevent)(int eventid, epevent_data_t *epdata, void *arg), void *arg)
{
    int i, err, ret, sockfd;

    epevent_data_t  epdata = {0};

    epdata.epollfd = epollfd;

    struct epoll_event *ev;

    while (1) {
        *epdata.msg = 0;

        //  epoll_pwait: 2.6.19 and later
        ret = epoll_pwait (epollfd, events, maxevents, timeout_ms, 0);

        if (ret <= 0) {
            if (ret == 0) {
                epdata.status = EPOLL_WAIT_TIMEOUT;
                if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == 1) {
                    continue;
                }
            } else if (errno == EINTR) {
                epdata.status = EPOLL_WAIT_EINTR;
                if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == 1) {
                    continue;
                }
            } else {
                // should break
                epdata.status = EPOLL_WAIT_ERROR;
                snprintf(epdata.msg, sizeof(epdata.msg), "epoll_pwait error(%d): %s", errno, strerror(errno));

                if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == 0) {
                    // should break
                    break;
                }

                // should not continue
                continue;
            }
        }

        epdata.status = EPOLL_WAIT_OK;
        epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg);

        for (i = 0; i < ret; i++) {
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

                err = epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg);
                if (err == 1) {
                    continue;
                } else if (err == 0) {
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

                    if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == 0) {
                        break;
                    } else {
                        goto onerror_exit;
                    }
                }

                assert(epdata.connfd != -1);
                err = epapi_on_epevent(EPEVENT_PEER_ADDRIN, &epdata, arg);

                if (err == 0) {
                    /** decline the coming client */
                    close(epdata.connfd);
                    break;
                } else if (err == -1) {
                    close(epdata.connfd);
                    goto onerror_exit;
                }

                /**
                 * http://man7.org/linux/man-pages/man3/getnameinfo.3.html
                 */
                err = getnameinfo(&epdata.in_addr, epdata.in_len,
                    epdata.hbuf, sizeof(epdata.hbuf), epdata.sbuf, sizeof(epdata.sbuf),
                    NI_NUMERICHOST | NI_NUMERICSERV);

                if (err != 0) {
                    epdata.status = EPOLL_NAMEINFO_ERROR;
                    snprintf(epdata.msg, sizeof(epdata.msg), "getnameinfo error(%d): %s", errno, strerror(errno));

                    if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) != 1) {
                        /** decline the coming client */
                        close(epdata.connfd);
                        break;
                    }
                } else if (epapi_on_epevent(EPEVENT_PEER_INFO, &epdata, arg) != 1) {
                    /** decline the coming client */
                    close(epdata.connfd);
                    break;
                }

                /**
                 * Make the incoming socket nonblock and add it to the list of fds to monitor.
                 */
                err = epapi_set_nonblock(epdata.connfd, epdata.msg, sizeof(epdata.msg));
                if (err == -1) {
                    epdata.status = EPOLL_SETSOCKET_ERROR;

                    if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == -1) {
                        close(epdata.connfd);
                        goto onerror_exit;
                    }

                    break;
                }

                // 新的客户连接置为 EPOLLONESHOT 事件
                err = epapi_add_epoll(epollfd, epdata.connfd, EPOLLONESHOT, epdata.msg, sizeof(epdata.msg));
                if (err == -1) {
                    epdata.status = EPOLL_SETSTATUS_ERROR;

                    if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == -1) {
                        close(epdata.connfd);
                        goto onerror_exit;
                    }

                    break;
                }

                epdata.status = err;
                err = epapi_on_epevent(EPEVENT_PEER_ACPTED, &epdata, arg);

                if (err == 0) {
                    close(epdata.connfd);
                    break;
                } else if (err == -1) {
                    close(epdata.connfd);
                    goto onerror_exit;
                }

                assert(err == 1);
            } else if (ev->events & EPOLLIN) {
                epdata.epollfd = epollfd;
                epdata.connfd = sockfd;

                err = epapi_on_epevent(EPEVENT_PEER_POLLIN, &epdata, arg);
                if (err == 0) {
                    // decline client
                    close(sockfd);
                    break;
                } else if (err == -1) {
                    close(sockfd);
                    goto onerror_exit;
                }

                assert(err == 1);

                /*
                epoll_event_cb(EPOLL_STATUS_POLLIN_OK, msg, arg);

                epoll_event_cb(0, "epoll_conn_handler handle client...\n", arg);

                // 客户端有数据发送的事件发生
                pthread_t thread;

                epfd_pair_t *fdpair = (epfd_pair_t *) malloc(sizeof(*fdpair));

                fdpair->epollfd = epollfd;
                fdpair->sockfd = sockfd;

                // 调用工作者线程处理数据
                pthread_create(&thread, NULL, epoll_conn_handler, (void*) fdpair);

                pthread_join(thread, NULL);
                */
            } else {
                /** something wrong, can go on */
                epdata.status = EPOLL_ERROR_UNEXPECT;
                if (epapi_on_epevent(EPEVENT_MSG_ERROR, &epdata, arg) == -1) {
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
