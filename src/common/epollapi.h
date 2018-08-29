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
 * @file: epollapi.h
 *   epoll ET non-block api
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.1
 *
 * @create: 2018-02-07
 *
 * @update: 2018-08-17 22:08:39
 *
 *
 *----------------------------------------------------------------------
 * reference:
 *
 *  - linux socket:
 *      https://linux.die.net/man/3/getaddrinfo
 *
 *  - epoll man:
 *      https://linux.die.net/man/4/epoll
 *
 *  - epoll with thread
 *      http://www.cnblogs.com/haippy/archive/2012/01/09/2317269.html
 *
 *  - epoll with ipv6
 *      http://www.cnblogs.com/haippy/archive/2012/01/09/2317269.html
 *----------------------------------------------------------------------
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


/**
 * epapi epevent id
 */
#define EPEVT_EPOLLIN_EDGE_TRIG          0

#define EPEVT_ERROR_EPOLL_WAIT           1
#define EPEVT_ERROR_EPOLL_EVENT          2

#define EPEVT_ACCEPT_EAGAIN              3
#define EPEVT_ACCEPT_EWOULDBLOCK         4
#define EPEVT_ERROR_ACCEPT               5

#define EPEVT_PEER_NAMEINFO              6
#define EPEVT_ERROR_PEERNAME             7

#define EPEVT_ERROR_SETNONBLOCK          8
#define EPEVT_ERROR_EPOLL_ADD_ONESHOT    9

#define EPEVT_PEER_CLOSED               10


#define epapi_on_epevent(evtid, epmsg, arg)  (epmsg->msg_cb_handlers[evtid])(epmsg, arg)


/**
 * 回调函数数据，不可以在多线程中使用
 */
typedef struct epevent_msg_t * epevent_msg;


/**
 * 事件回调函数，返回值:
 *   1 - 用户已经处理
 *   0 - 用户要求放弃
 */
typedef int (* epevent_msg_cb) (epevent_msg epmsg, void *arg);


typedef struct epevent_msg_t
{
    int epollfd;
    int connfd;

    struct sockaddr in_addr;
    socklen_t in_len;

    union {
        struct {
            char hbuf[NI_MAXHOST];
            char sbuf[NI_MAXSERV];
        };

        struct {
            char msgbuf[NI_MAXHOST + NI_MAXSERV];
        };
    };

    epevent_msg_cb msg_cb_handlers[EPEVT_PEER_CLOSED + 1];
} epevent_msg_t;


typedef struct epollin_arg_t
{
    int epollfd;
    int connfd;

    void *arg;
} epollin_arg_t;


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

    if (node && node[0] != '\0') {
        err = getaddrinfo(node, port, &hints, &result);
    } else {
        err = getaddrinfo(0, port, &hints, &result);
    }

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
    int flags;

    *errmsg = 0;

    flags = fcntl(listenfd, F_GETFL, 0);
    if (flags == -1) {
        snprintf(errmsg, msgsize, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    if (fcntl(listenfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        snprintf(errmsg, msgsize, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msgsize - 1] = '\0';
        return -1;
    }

    return flags;
}


/**
 * oneshotflag = 0 or EPOLLONETSHOT
 **/
__attribute__((used))
static int epapi_add_epoll (int epollfd, int fd, int oneshotflag, char *errmsg, ssize_t msgsize)
{
    struct epoll_event event = {0};

    *errmsg = 0;

    /* EPOLLET 边沿触发 */

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
static int epapi_init_epoll (int listenfd, int backlog, char *errmsg, ssize_t msgsize)
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


__attribute__((used))
static int epapi_loop_events (int epollfd, int listenfd, int timeout_ms,
    struct epoll_event *events, int maxevents,
    epevent_msg epmsg, void *arg)
{
    int i, nfd, fd;
    struct epoll_event *ev;

    ssize_t count;
    char buf[4096];

    epmsg->epollfd = epollfd;
    epmsg->connfd = -1;

    while (1) {
        *epmsg->msgbuf = 0;

        // https://linux.die.net/man/2/epoll_pwait
        //  epoll_pwait: 2.6.19 and later
        nfd = epoll_pwait(epollfd, events, maxevents, timeout_ms, 0);

        if (nfd == 0) {
            // no file descriptor became ready during the requested timeout milliseconds.
            continue;
        }

        if (nfd == -1) {
            snprintf(epmsg->msgbuf, sizeof(epmsg->msgbuf), "epoll_pwait error(%d): %s", errno, strerror(errno));
            epapi_on_epevent(EPEVT_ERROR_EPOLL_WAIT, epmsg, arg);
            continue;
        }

        for (i = 0; i < nfd; i++) {
            *epmsg->msgbuf = 0;

            ev = &events[i];
            fd = ev->data.fd;

            if ( (ev->events & EPOLLERR) ||
                 (ev->events & EPOLLHUP) ||
                 (! (ev->events & EPOLLIN)) ) {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */

                ev->data.fd = -1;
                close(fd);

                snprintf(epmsg->msgbuf, sizeof(epmsg->msgbuf), "error fd or the socket not ready for reading");
                epapi_on_epevent(EPEVT_ERROR_EPOLL_EVENT, epmsg, arg);

                continue;
            }

            if (listenfd == fd) {
                /* A new notification on the listening socket,
                 *   which means one or more incoming connections. */
                while(1) {
                    epmsg->in_len = sizeof(epmsg->in_addr);
                    epmsg->connfd = accept(listenfd, &epmsg->in_addr, &epmsg->in_len);

                    if (epmsg->connfd == -1) {
                        if (errno == EAGAIN) {
                            /* We have processed all incoming connections */
                            epapi_on_epevent(EPEVT_ACCEPT_EAGAIN, epmsg, arg);
                        } else if (errno == EWOULDBLOCK) {
                            epapi_on_epevent(EPEVT_ACCEPT_EWOULDBLOCK, epmsg, arg);
                        } else {
                            snprintf(epmsg->msgbuf, sizeof(epmsg->msgbuf), "accept error(%d): %s", errno, strerror(errno));
                            epapi_on_epevent(EPEVT_ERROR_ACCEPT, epmsg, arg);
                        }
                        break;
                    }

                    // http://man7.org/linux/man-pages/man3/getnameinfo.3.html
                    if ( getnameinfo(&epmsg->in_addr, epmsg->in_len,
                            epmsg->hbuf, sizeof(epmsg->hbuf),
                            epmsg->sbuf, sizeof(epmsg->sbuf),
                            NI_NUMERICHOST | NI_NUMERICSERV ) == 0 ) {
                        /* here we got ip:port of peer */
                        epapi_on_epevent(EPEVT_PEER_NAMEINFO, epmsg, arg);
                    } else {
                        snprintf(epmsg->msgbuf, sizeof(epmsg->msgbuf), "getnameinfo error(%d): %s", errno, strerror(errno));
                        epapi_on_epevent(EPEVT_ERROR_PEERNAME, epmsg, arg);
                    }

                    /* Make the incoming socket non-blocking and add it to the list of fds to monitor.*/
                    if (epapi_set_nonblock(epmsg->connfd, epmsg->msgbuf, sizeof(epmsg->msgbuf)) == -1) {
                        epapi_on_epevent(EPEVT_ERROR_SETNONBLOCK, epmsg, arg);

                        close(epmsg->connfd);
                        epmsg->connfd = -1;

                        goto EXIT_ON_ERROR;
                    }

                    /* add EPOLLONESHOT for new client */
                    if (epapi_add_epoll(epollfd, epmsg->connfd, EPOLLONESHOT, epmsg->msgbuf, sizeof(epmsg->msgbuf)) == -1) {
                        epapi_on_epevent(EPEVT_ERROR_EPOLL_ADD_ONESHOT, epmsg, arg);

                        close(epmsg->connfd);
                        epmsg->connfd = -1;

                        goto EXIT_ON_ERROR;
                    }
                }

                continue;
            } else {
                /* We have data on the connfd (fd) waiting to be read. Read and
                 display it. We must read whatever data is available
                 completely, as we are running in edge-triggered mode
                 and won't get a notification again for the same data. */

                epmsg->epollfd = epollfd;
                epmsg->connfd = fd;

                if (epapi_on_epevent(EPEVT_EPOLLIN_EDGE_TRIG, epmsg, arg) == 0) {
                    /* Add default implementation (do nothing) since caller has ignored
                      the event which must be handled */
                    int done = 0;

                    printf("WARN: should never run to this!\n");

                    while (1) {
                        count = read(fd, buf, sizeof(buf));

                        if (count == -1) {
                            /* If errno == EAGAIN, that means we have read all
                               data. So go back to the main loop. */
                            if (errno != EAGAIN) {
                                perror ("read");
                                done = 1;
                            }

                            break;
                        } else if (count == 0) {
                            /* End of file. The remote has closed the connection. */
                            done = 1;
                            break;
                        }

                        /* ignored data in buf */
                    }

                    if (done) {
                        /* Closing the descriptor will make epoll remove it
                           from the set of descriptors which are monitored. */
                        epapi_on_epevent(EPEVT_PEER_CLOSED, epmsg, arg);

                        close(fd);
                        ev->data.fd = -1;
                    }
                }
            }
        }
    }

EXIT_ON_ERROR:

    return (-1);
}

#if defined(__cplusplus)
}
#endif

#endif /* EPOLL_API_H_INCLUDED */
