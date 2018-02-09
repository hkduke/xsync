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

#include "sockapi.h"

#include <sys/epoll.h>

#include <assert.h>


#define SOCK_BUFSIZE    4096


typedef struct epfd_pair_t
{
    int epollfd;
    int sockfd;
} epfd_pair_t;


__attribute__((used))
static int create_and_bind (const char *node, const char *port, char *errmsg, ssize_t msgsize)
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
static int set_socket_nonblock (int listenfd, char *errmsg, ssize_t msgsize)
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
static int add_epoll_fd (int epollfd, int fd, int oneshotflag, char *errmsg, ssize_t msgsize)
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
static int modify_epoll_fd (int epollfd, int fd, int oneshotflag, char *errmsg, ssize_t msgsize)
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
static int init_epoll_event (int listenfd, int backlog, char *errmsg, ssize_t msgsize)
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

    if (add_epoll_fd (epollfd, listenfd, 0, errmsg, msgsize) == -1) {
        close(epollfd);
        return -1;
    }

    return epollfd;
}


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

    char buf[SOCK_BUFSIZE];

    while (1) {
        // 接收数据
        size = recv(sockfd, buf, SOCK_BUFSIZE, 0);

        if (size == 0) {
            /**
             * close the descriptor will make epoll remove it
             *   from the set of descriptors which are monitored.
             **/
            close(sockfd);
            printf("close sock: %d\n", sockfd);
            break;
        } else if (size < 0) {
            if (errno == EAGAIN) {
                // 并非网络出错，而是可以再次注册事件
                size = modify_epoll_fd (epollfd, sockfd, EPOLLONESHOT, buf, sizeof(buf));
                assert(size == 0);

                printf("modify_epoll_fd = EPOLLONESHOT.\n");
                break;
            }
        } else {
            buf[size] = 0;

            printf("buf: %s\n", buf);

            // 采用睡眠是为了在1s内若有新数据到来则该线程继续处理，否则线程退出
            sleep(1);
        }
    }

    printf("thread exit.\n");
    return NULL;
}


__attribute__((used))
static int loop_epoll_events (int epollfd, int listenfd, struct epoll_event *events, int maxevents, int timeout_ms,
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
            status_msg_cb(0, "epoll_pwait ok.\n");
        }


        for (i = 0; i < ret; i++) {
            ev = events;

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

                connfd = accept (listenfd, &in_addr, &in_len);

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
                err = set_socket_nonblock(connfd, msg, msgsize);
                if (err == -1) {
                    exit(-1);
                }

                // 新的客户连接置为 EPOLLONESHOT 事件
                err = add_epoll_fd (epollfd, connfd, EPOLLONESHOT, msg, msgsize);

                assert(err == 0);
                status_msg_cb(err, msg);
            } else if (ev->events & EPOLLIN) {
                // 客户端有数据发送的事件发生
                pthread_t thread;

                epfd_pair_t *fdpair = (epfd_pair_t *) malloc(sizeof(*fdpair));

                fdpair->epollfd = epollfd;
                fdpair->sockfd = sockfd;

                // 调用工作者线程处理数据
                pthread_create(&thread, NULL, epoll_conn_handler, (void*) fdpair);

                status_msg_cb(0, "epoll_conn_handler handle client...\n");
            } else {
                status_msg_cb(-1, "something wrong.\n");
                printf("something wrong.\n");
            }
        }
    }

    return 0;
}


#if defined(__cplusplus)
}
#endif

#endif /* EPOLL_API_H_INCLUDED */
