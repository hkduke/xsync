/***********************************************************************
* COPYRIGHT (C) 2018 PEPSTACK, PEPSTACK.COM
*
* THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES
* ARISING FROM THE USE OF THIS SOFTWARE.
*
* PERMISSION IS GRANTED TO ANYONE TO USE THIS SOFTWARE FOR ANY PURPOSE,
* INCLUDING COMMERCIAL APPLICATIONS, AND TO ALTER IT AND REDISTRIBUTE IT
* FREELY, SUBJECT TO THE FOLLOWING RESTRICTIONS:
*
*  THE ORIGIN OF THIS SOFTWARE MUST NOT BE MISREPRESENTED; YOU MUST NOT
*  CLAIM THAT YOU WROTE THE ORIGINAL SOFTWARE. IF YOU USE THIS SOFTWARE
*  IN A PRODUCT, AN ACKNOWLEDGMENT IN THE PRODUCT DOCUMENTATION WOULD
*  BE APPRECIATED BUT IS NOT REQUIRED.
*
*  ALTERED SOURCE VERSIONS MUST BE PLAINLY MARKED AS SUCH, AND MUST NOT
*  BE MISREPRESENTED AS BEING THE ORIGINAL SOFTWARE.
*
*  THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
***********************************************************************/

/**
 * @file: epollet.h
 *
 *  A frame using epoll() on linux with maximum functionality including:
 *   - Asynchronous I/O
 *   - Edge-Triggered mode
 *   - Handling signals, timers, and file/network I/O with the same syscall
 *   - Multi-threaded event loop
 *
 * originally taken from:
 *   https://github.com/mikedilger/epoll_demo/blob/master/src/xframedb.c
 *
 * NOTES:
 *   USE EPOLLONESHOT in epoll_ctl(), and re-arm with EPOLL_CTL_MOD after
 *    the event is handled (this avoids a 2nd thread handling the 2nd event
 *    before the 1st event is handled-- serializes events per fd -- which is
 *    typically expected).
 *
 * @author: master@pepstack.com
 *
 * @version: 0.4.0
 *
 * @create: 2018-09-04
 *
 * @update: 2018-11-01 16:12:09
 *
 *
 *----------------------------------------------------------------------
 * reference:
 *
 *  - https://stackoverflow.com/questions/13568858/epoll-wait-always-sets-epollout-bit
 *  - http://it165.net/os/html/201308/5868.html
 *  - https://www.cnblogs.com/fnlingnzb-learner/p/5835573.html
 *----------------------------------------------------------------------
 */

#ifndef EPOLLET_H_INCLUDED
#define EPOLLET_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <pthread.h>


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct epollet_event_t
{
    int epollfd;
    int clientfd;

    struct sockaddr in_addr;
    socklen_t in_len;

    struct epoll_event epev;

    union {
        struct {
            char hbuf[NI_MAXHOST];
            char sbuf[NI_MAXSERV];
        };

        char msg[NI_MAXHOST + NI_MAXSERV];
    };

    // user-specific argument
    void *arg;
} epollet_event_t;


/**
 * 事件回调函数，返回值:
 *   1 - 用户已经处理
 *   0 - 用户要求放弃
 */
typedef int (* epollet_event_cb) (struct epollet_event_t *);


typedef struct epollet_conf_t
{
    int epollfd;
    int listenfd;

    int old_flags;

    int numevents;
    struct epoll_event *events;

    epollet_event_cb epcb_trace;
    epollet_event_cb epcb_warn;
    epollet_event_cb epcb_error;
    epollet_event_cb epcb_new_peer;
    epollet_event_cb epcb_accept;
    epollet_event_cb epcb_reject;
    epollet_event_cb epcb_pollin;
    epollet_event_cb epcb_pollout;
} epollet_conf_t;


__attribute__((unused)) static inline void close_socket_s (int *sfd)
{
    int fd = *sfd;

    if (fd != -1) {
        *sfd = -1;

        close(fd);
    }
}


__attribute__((unused)) static inline int set_socket_nonblock (int sfd, char *errmsg, ssize_t msgsize)
{
    // Get the current flags on this socket
    int flags = fcntl(sfd, F_GETFL, 0);

    if ( flags == -1 ) {
        snprintf(errmsg, msgsize, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    // Add the non-block flag
    if ( fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1 ) {
        snprintf(errmsg, msgsize, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    return flags;
}


__attribute__((unused)) static int epollin_add (int epollfd, int fd, char *errmsg, ssize_t msgsize)
{
    struct epoll_event ev;

    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        snprintf(errmsg, msgsize, "epoll_ctl fail: %s", strerror(errno));
        errmsg[ msgsize - 1 ] = 0;
        return -1;
    }

    return 0;
}


__attribute__((unused)) static int epollin_mod (int epollfd, int fd, char *errmsg, ssize_t msgsize)
{
    struct epoll_event ev;

    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        snprintf(errmsg, msgsize, "epoll_ctl fail: %s", strerror(errno));
        errmsg[ msgsize - 1 ] = 0;
        return -1;
    }

    return 0;
}


__attribute__((unused)) static int epollout_mod (int epollfd, int fd, char *errmsg, ssize_t msgsize)
{
    struct epoll_event ev;

    ev.data.fd = fd;
    ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;

    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        snprintf(errmsg, msgsize, "epoll_ctl fail: %s", strerror(errno));
        errmsg[ msgsize - 1 ] = 0;
        return -1;
    }

    return 0;
}


__attribute__((unused)) static int create_and_bind (const char *node, const char *port, char *errmsg, ssize_t msgsize)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int err, sfd;

    /**
     * Get an internet address on ( this host ) port, with these hints
     * Clear hints with zeroes ( as required by getaddrinfo ( ) )
     */
    bzero( &hints, sizeof(struct addrinfo) );

    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* Suitable to bind a server onto */

    err = getaddrinfo( node, port, &hints, &result );

    if ( err != 0 ) {
        snprintf(errmsg, msgsize, "getaddrinfo error(%d): %s", err, gai_strerror(err));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    /**
     * For each addrinfo returned, try to open a socket and bind to the
     *   associated address; keep going until it works
     */
    for ( rp = result; rp != NULL; rp = rp->ai_next ) {
        sfd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );

        if ( sfd == -1 ) {
            continue;
        }

        err = bind( sfd, rp->ai_addr, rp->ai_addrlen );

        if ( err == 0 ) {
            /* We managed to bind successfully! */
            break;
        }

        close( sfd );
    }

    /* Fail if nothing worked */
    if ( rp == NULL ) {
        snprintf(errmsg, msgsize, "socket bind error");
        errmsg[msgsize - 1] = '\0';
        return (-1);
    }

    /* Clean up */
    freeaddrinfo( result );

    /* And return our server file descriptor */
    return sfd;
}


__attribute__((unused)) static int epollet_create_epoll (int sfd, int backlog, char *errmsg, ssize_t msgsize)
{
    int epollfd;

    /**
     * 如果要增大 SOMAXCONN, 需要修改内核参数:
     * backlog = SOMAXCONN
     *   $ sudo echo 2048 > /proc/sys/net/core/somaxconn
     *   $ sudo vi /etc/sysctl.conf:
     *   net.core.somaxconn=2048
     *   $ sysctl -p
     */
    if (listen (sfd, backlog) == -1) {
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

    /**
     * Make up the event structure with our socket and
     *   mark it for read ( EPOLLIN ) and edge triggered ( EPOLLET )
     */
    if (epollin_add (epollfd, sfd, errmsg, msgsize) == -1) {
        close(epollfd);
        return -1;
    }

    return epollfd;
}


__attribute__((unused)) static int epollet_conf_init (struct epollet_conf_t *epconf,
    const char *host, const char *port, int somaxconn, int maxevents, char *errmsg, ssize_t msgsize)
{
    int listenfd, epollfd, old_flags;

    struct epoll_event *events;

    bzero(epconf, sizeof(*epconf));

    /* Setup a server socket */
    listenfd = create_and_bind(host, port, errmsg, msgsize);
    if (listenfd == -1) {
        return (-1);
    }

    /* Make server socket non-blocking */
    old_flags = set_socket_nonblock(listenfd, errmsg, msgsize);
    if (old_flags == -1) {
        close(listenfd);
        return (-1);
    }

    /* Mark server socket as a listener, with maximum backlog queue */
    epollfd = epollet_create_epoll(listenfd, somaxconn, errmsg, msgsize);
    if (epollfd == -1) {
        fcntl(listenfd, F_SETFL, old_flags);
        close(listenfd);
        return (-1);
    }

    events = (struct epoll_event *) calloc(maxevents, sizeof(struct epoll_event));
    if (! events) {
        snprintf(errmsg, msgsize, "memory overflow when alloc events(=%d)", maxevents);
        errmsg[msgsize - 1] = '\0';

        fcntl(listenfd, F_SETFL, old_flags);
        close(epollfd);
        close(listenfd);
        return (-1);
    }

    epconf->listenfd = listenfd;
    epconf->epollfd = epollfd;
    epconf->old_flags = old_flags;

    epconf->events = events;
    epconf->numevents = maxevents;

    snprintf(errmsg, msgsize, "success");
    errmsg[msgsize - 1] = '\0';

    return 0;
}


__attribute__((unused)) static void epollet_conf_uninit (epollet_conf_t *epconf)
{
    fcntl(epconf->listenfd, F_SETFL, epconf->old_flags);

    close(epconf->listenfd);
    close(epconf->epollfd);

    free(epconf->events);

    epconf->epollfd = -1;
    epconf->listenfd = -1;
    epconf->events = 0;
}


__attribute__((unused)) static inline int ON_EPCB_TRACE(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_trace) {
        return epconf->epcb_trace(event);
    } else {
        // 不能使用线程池, 必须在主线程中
        printf("epcb_trace: %s\n", event->msg);

        return 0;
    }
}


__attribute__((unused)) static inline int ON_EPCB_WARN(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_warn) {
        return epconf->epcb_warn(event);
    } else {
        // 不能使用线程池, 必须在主线程中
        printf("epcb_warn: %s\n", event->msg);

        return 0;
    }
}


__attribute__((unused)) static inline int ON_EPCB_ERROR(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_error) {
        return epconf->epcb_error(event);
    } else {
        // 不能使用线程池, 必须在主线程中
        printf("epcb_error: %s\n", event->msg);

        // 返回值:
        //   1: 继续不退出
        //   0: 退出进程 (总是应该返回 0)
        return 0;
    }
}


__attribute__((unused)) static inline int ON_EPCB_NEW_PEER(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_new_peer) {
        return epconf->epcb_new_peer(event);
    } else {
        // 不能使用线程池, 必须在主线程中
        printf("new peer: [%s:%s]\n", event->hbuf, event->sbuf);

        // 返回值:
        //   1: 接受
        //   0: 拒绝
        return 1;
    }
}


__attribute__((unused)) static inline int ON_EPCB_ACCEPT(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_accept) {
        return epconf->epcb_accept(event);
    } else {
        // 不能使用线程池, 必须在主线程中
        printf("accept peer: [%s:%s]\n", event->hbuf, event->sbuf);

        // 不关心返回值
        return 1;
    }
}


__attribute__((unused)) static inline int ON_EPCB_REJECT(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_reject) {
        return epconf->epcb_reject(event);
    } else {
        // 不能使用线程池, 必须在主线程中
        printf("reject peer: [%s:%s]\n", event->hbuf, event->sbuf);

        // 不关心返回值
        return 0;
    }
}


__attribute__((unused)) static inline int ON_EPCB_POLLIN(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_pollin) {
        return epconf->epcb_pollin(event);
    } else {
        return 0;
    }
}


__attribute__((unused)) static inline int ON_EPCB_POLLOUT(struct epollet_event_t *event, struct epollet_conf_t *epconf)
{
    if (epconf->epcb_pollout) {
        return epconf->epcb_pollout(event);
    } else {
        return 0;
    }
}


__attribute__((unused)) static int epollet_loop_events (struct epollet_conf_t *epconf, struct epollet_event_t *event)
{
    int epollfd = epconf->epollfd;
    int listenfd = epconf->listenfd;

    struct epoll_event *events = epconf->events;
    int numevents = epconf->numevents;

    int i, err, nfd, clientfd;

    int len =  sizeof(event->msg) - 1;

    ub8 rc_read = 0;

    event->epollfd = epollfd;

    for (;;) {
        // wait for an event:
        //  -1 = wait forever
        //  ( 0 would mean return immediately, otherwise value is in ms )

        nfd = epoll_wait(epollfd, events, numevents, -1);

        if (nfd == 0) {
            snprintf(event->msg, len, "epoll_wait: no file descriptor ready");
            event->msg[len] = 0;

            ON_EPCB_WARN(event, epconf);
            continue;
        }

        if (nfd == -1) {
            snprintf(event->msg, len, "epoll_wait fail: %s", strerror(errno));
            event->msg[len] = 0;

            ON_EPCB_ERROR(event, epconf);

            return (-1);
        }

        // For each event that the epoll instance just gave us

        for (i = 0; i < nfd; i++) {
            // If error, hangup, or NOT ready for read

            int sfd = events[i].data.fd;

            event->clientfd = sfd;

            if (sfd == -1) {
                snprintf(event->msg, len, "epoll_wait: invalid socket");
                event->msg[len] = 0;

                ON_EPCB_WARN(event, epconf);
                continue;
            }

            if (events[i].events & EPOLLERR) {
                snprintf(event->msg, len, "epoll_wait: EPOLLERR");
                event->msg[len] = 0;

                ON_EPCB_WARN(event, epconf);

                close(sfd);
                continue;
            }

            if (events[i].events & EPOLLHUP) {
                snprintf(event->msg, len, "epoll_wait: EPOLLHUP");
                event->msg[len] = 0;

                ON_EPCB_WARN(event, epconf);

                close(sfd);
                continue;
            }

            if ( ! (events[i].events & EPOLLIN || events[i].events & EPOLLOUT) ) {
                snprintf(event->msg, len, "epoll_wait: unexpected events(=%d)", events[i].events);
                event->msg[len] = 0;

                ON_EPCB_WARN(event, epconf);

                close(sfd);
                continue;
            }

            if ( listenfd == events[i].data.fd ) {
                // If the event is on our listening socket we have a notification on
                //  the listening socket, which means one (or more) incoming connections

                while (1) {

                    // accept new client ? Grab an incoming connection socket and its address
                    event->in_len = sizeof(event->in_addr);

                    clientfd = accept(listenfd, (struct sockaddr *) & event->in_addr, & event->in_len);

                    event->clientfd = clientfd;

                    if (clientfd == -1) {
                        // Nothing was waiting

                        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
                            // We have processed all incoming connections

                            snprintf(event->msg, len, "accept(%d): %s", errno, strerror(errno));
                            event->msg[len] = 0;

                            ON_EPCB_TRACE(event, epconf);
                        } else {
                            // unexpected error
                            snprintf(event->msg, len, "accept error(%d): %s", errno, strerror(errno));
                            event->msg[len] = 0;

                            ON_EPCB_WARN(event, epconf);
                        }

                        break;
                    }

                    // translate that sockets address to host:port

                    err = getnameinfo (&event->in_addr, event->in_len,
                        event->hbuf, sizeof event->hbuf,
                        event->sbuf, sizeof event->sbuf,
                        NI_NUMERICHOST | NI_NUMERICSERV);

                    if (err) {
                        // unexpected error
                        snprintf(event->msg, len, "getnameinfo error(%d): %s", err, strerror(err));
                        event->msg[len] = 0;

                        ON_EPCB_WARN(event, epconf);

                        close(clientfd);
                        continue;
                    }

                    // Make the incoming socket non-blocking (required for epoll edge-triggered mode)

                    err = set_socket_nonblock(clientfd, event->msg, sizeof event->msg);
                    if (err == -1) {
                        ON_EPCB_WARN(event, epconf);

                        close(clientfd);
                        continue;
                    }

                    // success accept client?

                    if ( ON_EPCB_NEW_PEER(event, epconf) ) {
                        // Start watching for events to READ ( EPOLLIN ) on this socket in et mode
                        //  (EPOLLET - edge trigger) on the same epoll instance we are already using
                        // 注册用于 read 事件: EPOLLIN

                        if (epollin_add(epollfd, clientfd, event->msg, sizeof event->msg) == -1) {

                            ON_EPCB_REJECT(event, epconf);

                            close(clientfd);
                        } else {
                            // 通知接受新客户

                            ON_EPCB_ACCEPT(event, epconf);
                        }
                    } else {
                        ON_EPCB_REJECT(event, epconf);

                        close(clientfd);
                    }
                }

                // rearm the listening socket
                if (epollin_mod(epollfd, listenfd, event->msg, sizeof event->msg) == -1) {
                    ON_EPCB_ERROR(event, epconf);
                    return (-1);
                }
            } else if (events[i].events & EPOLLIN) {
                // The event was on a client socket:
                // We have data on the fd waiting to be read. Take action.
                // We must read whatever data is available completely, as we are running in
                //  edge-triggered mode and we won't get a notification again for the same data.

                err = ON_EPCB_POLLIN(event, epconf);

                if (! err) {
                    // 读光缓冲区
                    off_t total = 0;
                    int next = 1;
                    int count = 0;

                    while (next && (count = readlen_next(sfd, event->msg, sizeof event->msg, &next)) >= 0) {
                        if (count > 0) {
                            total += count;
                        }
                    }

                    if (! next) {
                        snprintf(event->msg, len, "[%ju] read %d bytes ok.\n", ++rc_read, count);
                        event->msg[len] = 0;

                        ON_EPCB_TRACE(event, epconf);

                    } else if (count < 0) {
                        // Closing the descriptor will make epoll remove it from
                        //  the set of descriptors which are monitored

                        snprintf(event->msg, len, "client close socket(%d)", sfd);
                        event->msg[len] = 0;

                        ON_EPCB_WARN(event, epconf);

                        close(sfd);

                        continue;
                    }

                    // rearm the socket. 注册事件用于 write
                    if (epollout_mod(epollfd, sfd, event->msg, sizeof event->msg) == -1) {

                        ON_EPCB_ERROR(event, epconf);

                        close(sfd);
                        return (-1);
                    }
                }
            } else if (events[i].events & EPOLLOUT) {
                err = ON_EPCB_POLLOUT(event, epconf);

                if (! err) {
                    // 如果回调未实现

                    snprintf(event->msg, sizeof event->msg, "TODO: EPOLLOUT");

                    write(sfd, event->msg, strlen(event->msg));

                    if (epollin_mod(epollfd, sfd, event->msg, sizeof event->msg) == -1) {

                        ON_EPCB_ERROR(event, epconf);

                        close(sfd);

                        return (-1);
                    }
                }
            }
        }
    }

    return 0;
}


#if defined(__cplusplus)
}
#endif

#endif /* EPOLLET_H_INCLUDED */
