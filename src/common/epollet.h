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
 * @version: 0.1.5
 *
 * @create: 2018-09-04
 *
 * @update: 2018-09-06 10:29:04
 *
 *
 *----------------------------------------------------------------------
 * reference:
 *
 *  - https://stackoverflow.com/questions/13568858/epoll-wait-always-sets-epollout-bit
 *  - http://it165.net/os/html/201308/5868.html
 *
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

#define EPET_MSG_MAXLEN  255


/**
 * epollet event id
 */
#define EPEVT_TRACE               0
#define EPEVT_WARN                1
#define EPEVT_FATAL               2
#define EPEVT_PEER_OPEN           3
#define EPEVT_PEER_CLOSE          4
#define EPEVT_ACCEPT              5
#define EPEVT_REJECT              6
#define EPEVT_POLLIN              7
#define EPEVT_COUNT              (EPEVT_POLLIN + 1)


#define EPOLLET_EVENT_CB(evtid, epmsg)  (epmsg->msg_cbs[evtid])(epmsg, epmsg->userarg)


/**
 * 回调函数数据，不可以在多线程中使用
 */
typedef struct epollet_msg_t * epollet_msg;


/**
 * 事件回调函数，返回值:
 *   1 - 用户已经处理
 *   0 - 用户要求放弃
 */
typedef int (* epollet_msg_cb) (epollet_msg epmsg, void *arg);

#define EPET_BUF_MAXSIZE    (NI_MAXHOST + NI_MAXSERV)


typedef struct epollet_msg_t
{
    int epollfd;
    int clientfd;

    struct epoll_event pollin_event;

    // 用户指定的参数
    void *userarg;

    struct sockaddr in_addr;
    socklen_t in_len;

    union {
        struct {
            char hbuf[NI_MAXHOST];
            char sbuf[NI_MAXSERV + 1];
        };

        struct {
            char buf[EPET_BUF_MAXSIZE + 1];
        };
    };

    epollet_msg_cb msg_cbs[EPEVT_COUNT];
} epollet_msg_t;


typedef struct epollet_server_t
{
    int timerfd;
    int sigfd;

    int listenfd;
    int epollfd;
    int old_flags;

    int maxevents;
    struct epoll_event *events;

    char msg[EPET_MSG_MAXLEN + 1];
} epollet_server_t;


/**
 * 回调函数原型
 */
static inline int epcb_trace(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_TRACE(%d): %s", epmsg->clientfd, epmsg->buf);
    return 1;
}

static inline int epcb_warn(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_WARN(%d): %s", epmsg->clientfd, epmsg->buf);
    return 1;
}

static inline int epcb_fatal(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_FATAL(%d): %s", epmsg->clientfd, epmsg->buf);

    /* 1=忽略错误, 0=退出服务 */
    return 0;
}

static inline int epcb_peer_open(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_PEER_OPEN(%d): %s:%s", epmsg->clientfd, epmsg->hbuf, epmsg->sbuf);

    /* 1=接受新客户连接, 0=拒绝新客户连接 */
    return 1;
}

static inline int epcb_accepted(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_ACCEPT(%d)", epmsg->clientfd);
    return 0;
}

static inline int epcb_pollin(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_POLLIN(%d): TODO", epmsg->clientfd);

    // -1: 没有实现, 使用默认实现
    // 0: 暂时无法提供服务
    // 1: 接受请求

    return -1;
}

static inline int epcb_peer_close(epollet_msg epmsg, void *arg)
{
    printf("EPEVT_PEER_CLOSE(%d): %s", epmsg->clientfd, epmsg->buf);
    return 0;
}


__attribute__((unused))
static int create_and_bind (const char *node, const char *port, char *errmsg, ssize_t msglen)
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
        snprintf(errmsg, msglen, "getaddrinfo error(%d): %s", err, gai_strerror(err));
        errmsg[msglen] = '\0';
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
        snprintf(errmsg, msglen, "socket bind error");
        errmsg[msglen] = '\0';
        return (-1);
    }

    /* Clean up */
    freeaddrinfo( result );

    /* And return our server file descriptor */
    return sfd;
}


__attribute__((unused))
static inline int set_socket_nonblock (int sfd, char *errmsg, ssize_t msglen)
{
    int flags;

    /* Get the current flags on this socket */
    flags = fcntl(sfd, F_GETFL, 0);
    if ( flags == -1 ) {
        snprintf(errmsg, msglen, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = '\0';
        return -1;
    }

    /* Add the non-block flag */
    if ( fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1 ) {
        snprintf(errmsg, msglen, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = '\0';
        return -1;
    }

    return flags;
}


__attribute__((unused))
static inline int epollet_ctl_add (int epollfd, int infd, char *errmsg, ssize_t msglen)
{
    struct epoll_event event = {0};

    event.data.fd = infd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event) == -1) {
        snprintf(errmsg, msglen, "epoll_ctl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = 0;
        return (-1);
    }

    return 0;
}


__attribute__((unused))
static inline int epollet_ctl_mod (int epollfd, struct epoll_event *epevent, char *errmsg, ssize_t msglen)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, epevent->data.fd, epevent) == -1) {
        snprintf(errmsg, msglen, "epoll_ctl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = 0;
        return (-1);
    }

    return 0;
}


__attribute__((unused))
static inline int epollet_ctl_del (int epollfd, struct epoll_event *epevent, char *errmsg, ssize_t msglen)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, epevent->data.fd, epevent) == -1) {
        snprintf(errmsg, msglen, "epoll_ctl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = 0;
        return (-1);
    }

    return 0;
}


__attribute__((unused))
static inline void cleanup_socket (int *fd_addr)
{
    int fd = *fd_addr;

    *fd_addr = -1;

    if (fd != -1) {
        close(fd);
    }
}


/**
 * 读光缓冲区
 * We have data on the infd waiting to be read. Read and display it.
 * We must read whatever data is available completely, as we are running
 *  in edge-triggered mode and won't get a notification again for the same data.
 */
__attribute__((unused))
static inline int readlen_next (int fd, char * buf, int len, int *next)
{
    ssize_t count;

    // 已读字节
    int offset = 0;

    // 剩余未读字节
    int remain = len - offset;

    *next = 1;

    while (remain > 0) {
        count = read(fd, buf + offset, remain);
        if (count == -1) {
            if (errno != EAGAIN) {
                perror("read");
                return (-1);
            }

            /**
             * if (errno == EAGAIN) that means we have read all data.
             */
            *next = 0;
            break;
        } else if (count == 0) {
            /* End of file. The remote has closed the connection. */
            printf("remote has closed the connection.\n");
            return (-2);
        } else {
            offset += count;
            remain = len - offset;
        }
    }

    return offset;
}


__attribute__((unused))
static int epollet_create_epoll (int sfd, int backlog, char *errmsg, ssize_t msglen)
{
    int epollfd;

    /**
     * 如果要增大 SOMAXCONN, 需要修改内核参数:
     * backlog = SOMAXCONN
     *   $ sudo echo 2048 > /proc/sys/net/core/somaxconn
     *   $ sudo vi /etc/sysctl.conf:
     *   net.core.somaxconn=2048
     *   $ sysctl -p
     **/

    if (listen (sfd, backlog) == -1) {
        snprintf(errmsg, msglen, "listen error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = '\0';
        return -1;
    }

    epollfd = epoll_create1 (EPOLL_CLOEXEC);
    if (epollfd == -1) {
        snprintf(errmsg, msglen, "epoll_create1 error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = '\0';
        return -1;
    }

    /**
     * Make up the event structure with our socket and
     *   mark it for read ( EPOLLIN ) and edge triggered ( EPOLLET )
     */
    if (epollet_ctl_add(epollfd, sfd, errmsg, msglen) == -1) {
        close(epollfd);
        return -1;
    }

    return epollfd;
}


__attribute__((unused))
static int epollet_server_initiate (epollet_server_t *epserver, const char *host, const char *port, int somaxconn, int maxevents)
{
    int listenfd, epollfd, old_flags;
    struct epoll_event *events;

    char *errmsg = epserver->msg;

    bzero(epserver, sizeof(*epserver));

    /* Setup a server socket */
    listenfd = create_and_bind(host, port, errmsg, EPET_MSG_MAXLEN);
    if (listenfd == -1) {
        return (-1);
    }

    /* Make server socket non-blocking */
    old_flags = set_socket_nonblock(listenfd, errmsg, EPET_MSG_MAXLEN);
    if (old_flags == -1) {
        close(listenfd);
        return (-1);
    }

    /* Mark server socket as a listener, with maximum backlog queue */
    epollfd = epollet_create_epoll(listenfd, somaxconn, errmsg, EPET_MSG_MAXLEN);
    if (epollfd == -1) {
        fcntl(listenfd, F_SETFL, old_flags);
        close(listenfd);
        return (-1);
    }

    events = (struct epoll_event *) calloc(maxevents, sizeof(struct epoll_event));
    if (! events) {
        snprintf(errmsg, EPET_MSG_MAXLEN, "memory overflow when alloc events(=%d)", maxevents);
        errmsg[EPET_MSG_MAXLEN] = '\0';

        fcntl(listenfd, F_SETFL, old_flags);
        close(epollfd);
        close(listenfd);
        return (-1);
    }

    epserver->listenfd = listenfd;
    epserver->epollfd = epollfd;
    epserver->old_flags = old_flags;

    epserver->events = events;
    epserver->maxevents = maxevents;

    snprintf(errmsg, EPET_MSG_MAXLEN, "success");
    errmsg[EPET_MSG_MAXLEN] = '\0';

    return 0;
}


__attribute__((unused))
static void epollet_server_finalize (epollet_server_t *epserver)
{
    fcntl(epserver->listenfd, F_SETFL, epserver->old_flags);

    close(epserver->listenfd);
    close(epserver->epollfd);

    free(epserver->events);

    epserver->epollfd = -1;
    epserver->listenfd = -1;
    epserver->events = 0;
}


__attribute__((unused))
static void epollet_server_loop_events (epollet_server_t *epserver, epollet_msg epmsg)
{
    int epollfd = epserver->epollfd;
    int listenfd = epserver->listenfd;

    int maxevents = epserver->maxevents;
    struct epoll_event *events = epserver->events;

    struct epoll_event *epevent;

    int nfd, i, err;

    char *msg = epserver->msg;

    epmsg->epollfd = epollfd;
    epmsg->clientfd = -1;

    while (1) {
        /**
         * Wait for an event. -1 = wait forever
         * ( 0 would mean return immediately, otherwise value is in ms )
         */
        nfd = epoll_wait(epollfd, events, maxevents, -1);
        if (nfd == 0) {
            snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "epoll_wait: no file descriptor ready");
            epmsg->buf[EPET_BUF_MAXSIZE] = 0;

            EPOLLET_EVENT_CB(EPEVT_TRACE, epmsg);

            continue;
        }

        if (nfd == -1) {
            snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "epoll_wait error(%d): %s", errno, strerror(errno));
            epmsg->buf[EPET_BUF_MAXSIZE] = 0;

            if (EPOLLET_EVENT_CB(EPEVT_FATAL, epmsg)) {
                continue;
            }

            goto exit_fatal_error;
        }

        /**
         * loop-for-each-event
         *   For each event that the epoll instance just gave us
         */
        for (i = 0; i < nfd; i++) {
            epevent = &events[i];
            epmsg->clientfd = epevent->data.fd;

            /**
             * If error, hangup, or NOT ready for read
             */
            if ((epevent->events & EPOLLERR) ||
                (epevent->events & EPOLLHUP) ||
                (!(epevent->events & EPOLLIN))) {
                /**
                 * An error has occured on this fd, or the socket is not
                 *  ready for reading ( why were we notified then? )
                 */
                snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "error has occured on fd or socket not ready for reading");
                epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                cleanup_socket(&epevent->data.fd);

                continue;
            }

            if ( listenfd == epevent->data.fd ) {
                /**
                 * If the event is on our listening socket we have a notification on
                 * the listening socket, which means one (or more) incoming connections
                 */
                while (1) {
                    /**
                     * accept new client ?
                     *   Grab an incoming connection socket and its address
                     */
                    epmsg->in_len = sizeof(epmsg->in_addr);
                    epmsg->clientfd = accept(listenfd, &epmsg->in_addr, &epmsg->in_len);

                    if (epmsg->clientfd == -1) {
                        // Nothing was waiting

                        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
                            // We have processed all incoming connections
                            snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "accept again(%d): %s", errno, strerror(errno));
                            epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                            EPOLLET_EVENT_CB(EPEVT_TRACE, epmsg);

                        } else {
                            // unexpected error
                            snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "accept error(%d): %s", errno, strerror(errno));
                            epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                            EPOLLET_EVENT_CB(EPEVT_WARN, epmsg);
                        }

                        break;
                    }

                    /* Translate that sockets address to host:port */
                    err = getnameinfo(&epmsg->in_addr, epmsg->in_len,
                                epmsg->hbuf, sizeof(epmsg->hbuf),
                                epmsg->sbuf, sizeof(epmsg->sbuf),
                                NI_NUMERICHOST | NI_NUMERICSERV);

                    if (err == 0) {
                        // success
                        if (! EPOLLET_EVENT_CB(EPEVT_PEER_OPEN, epmsg)) {
                            // client rejected

                            EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                            cleanup_socket(&epmsg->clientfd);

                            continue;
                        }
                    } else {
                        // error
                        snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "getnameinfo error(%d): %s", err, strerror(err));
                        epmsg->buf[EPET_BUF_MAXSIZE] = 0;
                        EPOLLET_EVENT_CB(EPEVT_WARN, epmsg);

                        EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                        cleanup_socket(&epmsg->clientfd);

                        continue;
                    }

                    /**
                     * Make the incoming socket non-blocking (required for epoll edge-triggered mode)
                     */
                    err = set_socket_nonblock(epmsg->clientfd, msg, EPET_MSG_MAXLEN);
                    if ( err == -1 ) {
                        snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "set_socket_nonblock error: %s", msg);
                        epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                        if (EPOLLET_EVENT_CB(EPEVT_FATAL, epmsg)) {

                            EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                            cleanup_socket(&epmsg->clientfd);

                            continue;
                        }

                        EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                        cleanup_socket(&epmsg->clientfd);

                        goto exit_fatal_error;
                    }

                    /**
                     * Start watching for events to READ ( EPOLLIN ) on this socket in
                     *  edge triggered mode ( EPOLLET ) on the same epoll instance we
                     *  are already using
                     */
                    if (epollet_ctl_add(epollfd, epmsg->clientfd, msg, EPET_MSG_MAXLEN) == -1) {
                        snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "epollet_ctl_add error: %s", msg);
                        epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                        if (EPOLLET_EVENT_CB(EPEVT_FATAL, epmsg)) {

                            EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                            cleanup_socket(&epmsg->clientfd);

                            continue;
                        }

                        EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                        cleanup_socket(&epmsg->clientfd);

                        goto exit_fatal_error;
                    }

                    /* here we accept client */
                    EPOLLET_EVENT_CB(EPEVT_ACCEPT, epmsg);
                }

                /**
                 * Re-arm the listening socket
                 */
                if (epollet_ctl_mod(epollfd, epevent, msg, EPET_MSG_MAXLEN) == -1) {
                    snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "epollet_ctl_mod error: %s", msg);
                    epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                    EPOLLET_EVENT_CB(EPEVT_FATAL, epmsg);

                    goto exit_fatal_error;
                }

                /* next event please */
                continue;
            } else if (epevent->events & EPOLLIN) {
                /**
                 * The event was on a client socket:
                 * We have data on the fd waiting to be read. Take action.
                 * We must read whatever data is available completely,
                 * as we are running in edge-triggered mode and we won't
                 * get a notification again for the same data.
                 */
                *epmsg->buf = 0;

                epmsg->pollin_event = *epevent;

                err = EPOLLET_EVENT_CB(EPEVT_POLLIN, epmsg);

                if (err == 0) {
                    /* 暂时无法提供服务 */
                    EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                    cleanup_socket(&epmsg->clientfd);
                } else if (err == -1) {
                    /* 没有实现, 使用默认实现 */
                    int next = 1;
                    int cb = 0;

                    while (next && (cb = readlen_next(epmsg->clientfd, msg, EPET_MSG_MAXLEN + 1, &next)) >= 0) {
                        if (cb > 0) {
                            msg[cb] = 0;

                            snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "read %d bytes", cb);
                            epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                            EPOLLET_EVENT_CB(EPEVT_TRACE, epmsg);
                        }
                    }

                    if (cb < 0) {
                        /**
                         * Closing the descriptor will make epoll remove it from the set of
                         *  descriptors which are monitored.
                         */
                        snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "peer close: read error(%d)", cb);
                        epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                        EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                        cleanup_socket(&epmsg->clientfd);

                        // goto: loop-for-each-event
                        continue;
                    }

                    cb = snprintf(msg, EPET_MSG_MAXLEN, "SUCCESS");
                    msg[cb] = 0;

                    if (write(epmsg->clientfd, msg, cb + 1) == -1) {
                        snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "peer close: write error(%d): %s", errno, strerror(errno));
                        epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                        EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                        cleanup_socket(&epmsg->clientfd);

                        // goto: loop-for-each-event
                        continue;
                    }

                    /**
                     * Re-arm the socket
                     */
                    if (epollet_ctl_mod(epollfd, epevent, msg, EPET_MSG_MAXLEN) == -1) {
                        snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "epollet_ctl_mod error: %s", msg);
                        epmsg->buf[EPET_BUF_MAXSIZE] = 0;

                        EPOLLET_EVENT_CB(EPEVT_FATAL, epmsg);

                        EPOLLET_EVENT_CB(EPEVT_PEER_CLOSE, epmsg);

                        cleanup_socket(&epmsg->clientfd);

                        goto exit_fatal_error;
                    }
                }
            }
        }
    }

exit_fatal_error:

    snprintf(epmsg->buf, EPET_BUF_MAXSIZE, "exit_fatal_error");
    epmsg->buf[EPET_BUF_MAXSIZE] = 0;

    EPOLLET_EVENT_CB(EPEVT_FATAL, epmsg);
}


#if defined(__cplusplus)
}
#endif

#endif /* EPOLLET_H_INCLUDED */
