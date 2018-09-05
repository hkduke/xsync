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
 * sockapi.h
 *
 * create: 2014-08-01
 * update: 2018-02-26
 *
 */
#ifndef SOCKAPI_H_INCLUDED
#define SOCKAPI_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <string.h>
#include <stdio.h>

#include <stdlib.h>
#include <jemalloc/jemalloc.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <pthread.h>  /* link: -pthread */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <semaphore.h>
#include <stdarg.h>

#include <getopt.h>  /* getopt_long */

/*
 * type definitions
 */
#ifndef SOCKET
    typedef int SOCKET;
#endif

#define SOCKAPI_ERROR_SOCKET  (-1)

#define SOCKAPI_SUCCESS          0
#define SOCKAPI_ERROR          (-1)

#define SOCKAPI_ERROR_SOCKOPT  (-2)
#define SOCKAPI_ERROR_FDNOSET  (-3)
#define SOCKAPI_ERROR_TIMEOUT  (-4)
#define SOCKAPI_ERROR_SELECT   (-5)


#ifndef SOCKAPI_BUFSIZE
#  define SOCKAPI_BUFSIZE      8192
#endif


struct sockconn_opts
{
    int timeosec;
    int nowait;

    int keepidle;
    int keepinterval;
    int keepcount;

    int nodelay;
};


__attribute__((unused))
static void sockconn_opts_init_default ( struct sockconn_opts * opts )
{
    opts->timeosec = 3;
    opts->nowait = 1;

    opts->keepidle = 3;
    opts->keepinterval = 3;
    opts->keepcount = 3;

    /* nodelay = 1 disable Nagle, use TCP_CORK to enable Nagle */
    opts->nodelay = 1;
}


__attribute__((unused))
static int connect_nowait (int sd, const struct sockaddr *name, int namelen, int timeosec)
{
    int err;
    socklen_t len = (socklen_t) sizeof(err);

    // 设置为非阻塞模式
    unsigned long nonblock = 1;

#ifdef _MSC_VER
    err = ioctlsocket(sd, FIONBIO, &nonblock);
    // Windows: WSAGetLastError
#else
    err = ioctl(sd, FIONBIO, &nonblock);
    // Linux: strerror(errno)
#endif

    if (err != 0) {
        return (-1);
    }

    // 恢复为阻塞模式
    nonblock = 0;

    err = connect(sd, name, namelen);
    if (err == 0) {
        // 连接成功
#ifdef _MSC_VER
        err = ioctlsocket(sd, FIONBIO, &nonblock);
#else
        err = ioctl(sd, FIONBIO, &nonblock);
#endif
        // 0: success; -1: error
        return err;
    } else {
        struct timeval tm;
        fd_set set;

        tm.tv_sec  = timeosec;
        tm.tv_usec = 0;

        FD_ZERO(&set);
        FD_SET(sd, &set);

        err = select(sd + 1, 0, &set, 0, &tm);
        if (err > 0) {
            if (FD_ISSET(sd, &set)) {
                // 下面的一句一定要，主要针对防火墙
                if (getsockopt(sd, SOL_SOCKET, SO_ERROR, (char *) &err, &len) != 0) {
                #ifdef _MSC_VER
                    ioctlsocket(sd, FIONBIO, &nonblock);
                    // WSAGetLastError()
                #else
                    ioctl(sd, FIONBIO, &nonblock);
                    // strerror(errno)
                #endif
                    return SOCKAPI_ERROR_SOCKOPT;
                }

                if (err == 0) {
                    // 连接成功
                #ifdef _MSC_VER
                    err = ioctlsocket(sd, FIONBIO, &nonblock);
                    // WSAGetLastError()
                #else
                    err = ioctl(sd, FIONBIO, &nonblock);
                    // strerror(errno)
                #endif

                    // 0: success; -1: error
                    return err;
                }
            }

        #ifdef _MSC_VER
            ioctlsocket(sd, FIONBIO, &nonblock);
        #else
            ioctl(sd, FIONBIO, &nonblock);
        #endif

            return SOCKAPI_ERROR_FDNOSET;
        } else if (err == 0) {
            // 超时
        #ifdef _MSC_VER
            ioctlsocket(sd, FIONBIO, &nonblock);
        #else
            ioctl(sd, FIONBIO, &nonblock);
        #endif

            return SOCKAPI_ERROR_TIMEOUT;
        } else {
            // 错误
        #ifdef _MSC_VER
            ioctlsocket(sd, FIONBIO, &nonblock);
        #else
            ioctl(sd, FIONBIO, &nonblock);
        #endif

            return SOCKAPI_ERROR_SELECT;
        }
    }
}


__attribute__((unused))
static SOCKET opensocket (const char *server, int port, int timeo_sec, int nowait, int *err)
{
    SOCKET               sockfd;
    struct sockaddr_in   saddr;
    struct hostent      *hp;
    unsigned int        iaddr;

    hp = 0;

    /* parse saddr host */
    if (inet_addr (server) == INADDR_NONE) {
        hp = gethostbyname (server);
        if (hp == 0) {
            return SOCKAPI_ERROR_SOCKET;
        }
        saddr.sin_addr.s_addr = *((unsigned long*) hp->h_addr);
    } else {
        iaddr = inet_addr (server);
        saddr.sin_addr.s_addr = iaddr;
    }

    saddr.sin_family = AF_INET;  /* PF_INET */
    saddr.sin_port = htons ((u_short) port);

    /* create a new socket and attempt to connect to saddr */
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  /* PF_INET */
    if (sockfd == SOCKAPI_ERROR_SOCKET) {
        return SOCKAPI_ERROR_SOCKET;
    }

    if (nowait) {
        /* make a connection to the saddr, no matter what local port used */
        *err = connect_nowait(sockfd, (struct sockaddr *) &saddr, (int) sizeof(saddr), timeo_sec);
    } else {
        if (timeo_sec) {
            struct timeval timeo = {0};
            socklen_t len = (socklen_t) sizeof(timeo);
            timeo.tv_sec = timeo_sec;

            if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len) == -1) {
                return SOCKAPI_ERROR_SOCKET;
            }
        }

        *err = connect(sockfd, (struct sockaddr *) &saddr, (int) sizeof(saddr));
    }

    if (*err == 0) {
        /* all is ok */
        return sockfd;
    }

    close(sockfd);

    /**
     * errno = EINPROGRESS
     *   The socket is nonblocking and the connection cannot be
     *     completed immediately.
     */
    return SOCKAPI_ERROR_SOCKET;
}


/**
 * For Linux Client, there are 4 parameters to keep alive for TCP:
 *   tcp_keepalive_probes - detecting times
 *   tcp_keepalive_time - send detecting pkg after last pkg
 *   tcp_keepalive_intvl - interval between detecting pkgs
 *   tcp_retries2 -
 * On Linux OS, use "echo" to update these params:
 *   echo "30" > /proc/sys/net/ipv4/tcp_keepalive_time
 *   echo "6" > /proc/sys/net/ipv4/tcp_keepalive_intvl
 *   echo "3" > /proc/sys/net/ipv4/tcp_keepalive_probes
 *   echo "3" > /proc/sys/net/ipv4/tcp_retries2
 * tcp_keepalive_time and tcp_keepalive_intvl is by seconds
 *   if keep them after system reboot, must add to: /etc/sysctl.conf file
 */
__attribute__((unused))
static int setsockalive (SOCKET sd, int keepIdle, int keepInterval, int keepCount)
{
    int keepAlive = 1;

    socklen_t len = (socklen_t) sizeof(keepAlive);

    /* enable keep alive */
    setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));

    /* tcp idle time, test time internal, test times */
    if (keepIdle > 0) {
        setsockopt(sd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    }
    if (keepInterval > 0) {
        setsockopt(sd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    }
    if (keepCount > 0) {
        setsockopt(sd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
    }

    /* Check the status again */
    if (getsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, &len) < 0) {
        return SOCKAPI_ERROR_SOCKET;
    }

    return keepAlive;
}


__attribute__((unused))
static int setsocktimeo (SOCKET sd, int sendTimeoutSec, int recvTimeoutSec)
{
    if (sendTimeoutSec > 0) {
        struct timeval sndtime = {sendTimeoutSec, 0};

        if (SOCKAPI_SUCCESS != setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char*) &sndtime, sizeof(struct timeval))) {
            return SOCKAPI_ERROR;
        }
    }

    if (recvTimeoutSec > 0) {
        struct timeval rcvtime = {recvTimeoutSec, 0};

        if (SOCKAPI_SUCCESS != setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*) &rcvtime, sizeof(struct timeval))) {
            return SOCKAPI_ERROR;
        }
    }

    return SOCKAPI_SUCCESS;
}


__attribute__((unused))
static int socket_set_conn_opts (int connfd, const struct sockconn_opts *opts, char errmsg[256])
{
    /* set timeouts on current socket connection */
    if (SOCKAPI_SUCCESS != setsocktimeo(connfd, opts->timeosec, opts->timeosec)) {
        snprintf(errmsg, 255, "setsocktimeo error: %s", strerror(errno));
        errmsg[255] = 0;

        return SOCKAPI_ERROR;
    }

    /* set keepalive for current socket connection */
    if (1 != setsockalive(connfd, opts->keepidle, opts->keepinterval, opts->keepcount)) {
        snprintf(errmsg, 255, "setsockalive error: %s", strerror(errno));
        errmsg[255] = 0;

        return SOCKAPI_ERROR;
    }

    /* nodelay=1, TCP_NODELAY disable Nagle, use TCP_CORK to enable Nagle */
    if (opts->nodelay) {
        if (setsockopt (connfd, IPPROTO_TCP, TCP_NODELAY, &opts->nodelay, sizeof(opts->nodelay))) {
            snprintf(errmsg, 255, "setsockopt TCP_NODELAY error: %s", strerror(errno));
            errmsg[255] = 0;

            return SOCKAPI_ERROR;
        }
    } else {
        if (setsockopt (connfd, IPPROTO_TCP, TCP_CORK, &opts->nodelay, sizeof(opts->nodelay))) {
            snprintf(errmsg, 255, "setsockopt TCP_CORK error: %s", strerror(errno));
            errmsg[255] = 0;

            return SOCKAPI_ERROR;
        }
    }

    return SOCKAPI_SUCCESS;
}


/**
 * IPv4 and IPv6 supported
 *
 * https://linux.die.net/man/3/gai_strerror
 */
__attribute__((unused))
static SOCKET opensocket_v2 (const char *host, const char *port, int timeo_sec, const struct sockconn_opts *opts, char errmsg[256])
{
    struct addrinfo hints;
    struct addrinfo *res, *rp;

    int err;

    SOCKET sockfd = -1;

    *errmsg = 0;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* We want a TCP socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;            /* Any protocol */

    err = getaddrinfo(host, port, &hints, &res);
    if (err != 0) {
        snprintf(errmsg, 255, "getaddrinfo error: %s", gai_strerror(err));
        errmsg[255] = 0;

        return -1;
    }

    /**
     * getaddrinfo() returns a list of address structures.
     *
     * Try each address until we successfully connect(2).
     * If socket(2) (or connect(2)) fails, we (close the socket and)
     *   try the next address.
     */
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        struct timeval old_timeo = {0};
        socklen_t old_len = (socklen_t) sizeof(old_timeo);
        old_timeo.tv_sec = timeo_sec;

        /**
         * socket()
         *
         *   create an endpoint for communication
         *     https://linux.die.net/man/2/socket
         *   On success, a file descriptor for the new socket is returned.
         *   On error, -1 is returned, and errno is set appropriately.
         */
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sockfd == -1) {
            snprintf(errmsg, 255, "socket error(%d): %s", errno, strerror(errno));
            errmsg[255] = 0;

            continue;
        }

        if (timeo_sec) {
            struct timeval new_timeo = {0};
            socklen_t new_len = (socklen_t) sizeof(new_timeo);
            new_timeo.tv_sec = timeo_sec;

            if (getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &old_timeo, &old_len) == -1) {
                snprintf(errmsg, 255, "getsockopt SO_SNDTIMEO error: %s", strerror(errno));
                errmsg[255] = 0;

                close(sockfd);
                continue;
            }

            if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &new_timeo, new_len) == -1) {
                snprintf(errmsg, 255, "setsockopt SO_SNDTIMEO error: %s", strerror(errno));
                errmsg[255] = 0;

                close(sockfd);
                continue;
            }
        }

        /**
         * connect a socket
         *  - https://linux.die.net/man/3/connect
         *   Upon successful completion, connect() shall return 0;
         *   otherwise, -1 shall be returned and errno set to indicate
         *   the error.
         */
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* success */
            *errmsg = 0;
            break;
        }

        /* error */
        snprintf(errmsg, 255, "connect a socket error(%d): %s", errno, strerror(errno));
        errmsg[255] = 0;

        if (timeo_sec) {
            setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &old_timeo, old_len);
        }

        close(sockfd);
    }

    if (rp == NULL) {
        if (! *errmsg) {
            snprintf(errmsg, 255, "no address succeeded: %s:%s", host, port);
            errmsg[255] = 0;
        }

        freeaddrinfo(res);
        return -1;
    }

    /* No longer needed */
    freeaddrinfo(res);

    if (opts) {
        if (socket_set_conn_opts(sockfd, opts, errmsg) != SOCKAPI_SUCCESS) {
            close(sockfd);
            return -1;
        }
    }

    return sockfd;
}


__attribute__((unused))
static inline int setbuflen (SOCKET s, int rcvlen, int sndlen)
{
    socklen_t rcv, snd;
    socklen_t rcvl = (socklen_t) sizeof(int);
    socklen_t sndl = rcvl;

    /* default size is 8192 */
    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&rcv, &rcvl) == SOCKAPI_ERROR_SOCKET ||
        getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&snd, &sndl) == SOCKAPI_ERROR_SOCKET) {
        return SOCKAPI_ERROR_SOCKET;
    }

    if (rcv < rcvlen) {
        rcv = rcvlen;
        rcvl = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*) &rcv, rcvl);
    }

    if (snd < sndlen){
        snd = sndlen;
        sndl = setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&snd, sndl);
    }

    return SOCKAPI_SUCCESS;
}


__attribute__((unused))
static inline int recvlen (int fd, char * buf, int len)
{
    int    rc;
    char * pb = buf;

    while (len > 0) {
        rc = recv (fd, pb, len, 0);
        if (rc > 0) {
            pb += rc;
            len -= rc;
        } else if (rc < 0) {
            if (errno != EINTR) {
                /* error returned */
                return -1;
            }
        } else {
            /* rc == 0: socket has closed */
            return 0;
        }
    }

    return (pb - buf);
}


__attribute__((unused))
static inline int sendlen (int fd, const char* msg, int len)
{
    int    rc;
    const char * pb = msg;

    while (len > 0) {
        rc = send(fd, pb, len, 0);
        if (rc > 0) {
            pb += rc;
            len -= rc;
        } else if (rc < 0) {
            if (errno != EINTR) {
                /* error returned */
                return -1;
            }
        } else {
            /* socket closed */
            return 0;
        }
    }

    return (pb - msg);
}


__attribute__((unused))
static inline int send_bulky (SOCKET s, const char* bulk, int bulksize, int sndl /* length per msg to send */)
{
    int snd, ret;
    int left = bulksize;
    int at = 0;

    while (left > 0) {
        snd = left > sndl? sndl : left;
        ret = sendlen(s, &bulk[at], snd);
        if (ret != snd) {
            return ret;
        }
        at += ret;
        left -= ret;
    }

    return at;
}


__attribute__((unused))
static inline int recv_bulky (SOCKET s, char* bulk, int len, int rcvl/* length per msg to recv */)
{
    int  rcv, ret;
    int  left = len;
    int  at = 0;

    while (left > 0) {
        rcv = left > rcvl? rcvl : left;
        ret = recvlen(s, &bulk[at], rcv);
        if (ret != rcv) {
            return ret;
        }
        at += ret;
        left -= ret;
    }

    return at;
}


__attribute__((unused))
static ssize_t pread_len (int fd, unsigned char *buf, size_t len, off_t pos)
{
    ssize_t rc;
    unsigned char *pb = buf;

    while (len != 0 && (rc = pread (fd, (void*) pb, len, pos)) != 0) {
        if (rc == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else {
                /* pread error */
                return SOCKAPI_ERROR;
            }
        }

        len -= rc;
        pos += rc;
        pb += rc;
    }

    return (ssize_t) (pb - buf);
}


__attribute__((unused))
static inline ssize_t sendfile_len (int sockfd, int filefd, size_t len, off_t pos)
{
    ssize_t rc;

    off_t  off = pos;

    while (len != 0 && (rc = sendfile (sockfd, filefd, &off, len)) != 0) {
        if (rc == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else {
                return -1;
            }
        }

        /* rc > 0 */
        len -= rc;
    }

    return (ssize_t) (off - pos);
}


/**********************************************************************
 *                                                                    *
 *                        socket no-block api                         *
 *                                                                    *
 **********************************************************************/
__attribute__((unused))
static inline int socket_set_nonblock (int fd, char *errmsg, ssize_t msglen)
{
    int flags;

    /* Get the current flags on this socket */
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        snprintf(errmsg, msglen, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = '\0';
        return -1;
    }

    /* Add the non-block flag */
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        snprintf(errmsg, msglen, "fcntl error(%d): %s", errno, strerror(errno));
        errmsg[msglen] = '\0';
        return -1;
    }

    return flags;
}


/**
 * 读非阻塞数据
 *   buf - 存放读入的数据
 *   len - 要读的数据字节
 *   返回实际读的字节. 如果返回的字节数 >=0 且 errno == EAGAIN, 则数据已经读完
 *
    char msg[256];
    int next = 1;
    int cbread = 0;

    while (next && (cbread = nb_readlen_next(sockfd, msg, sizeof(msg), &next)) >= 0) {
        if (cbread > 0) {
            msg[cbread] = 0;

            LOGGER_DEBUG("read: %s", msg);
        }
    }

    if (cbread < 0) {
        LOGGER_ERROR("read (%d)", cbread);
        close(sockfd);
        sockfd = -1;
    }
 *
 */
__attribute__((unused))
static inline int nb_readlen_next (int fd, char * buf, int len, int *next)
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


#if defined(__cplusplus)
}
#endif

#endif /* SOCKAPI_H_INCLUDED */
