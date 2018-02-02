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
 * update: 2018-01-22
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
static int setsockalive (SOCKET s, int keepIdle, int keepInterval, int keepCount)
{
    int keepAlive = 1;

    socklen_t len = (socklen_t) sizeof(keepAlive);

    /* enable keep alive */
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));

    /* tcp idle time, test time internal, test times */
    if (keepIdle > 0) {
        setsockopt(s, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    }
    if (keepInterval > 0) {
        setsockopt(s, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    }
    if (keepCount > 0) {
        setsockopt(s, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
    }

    /* Check the status again */
    if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, &len) < 0) {
        return SOCKAPI_ERROR_SOCKET;
    }

    return keepAlive;
}


__attribute__((unused))
static int setsocktimeo (SOCKET sockfd, int sendTimeoutSec, int recvTimeoutSec)
{
    if (sendTimeoutSec > 0) {
        struct timeval sndtime = {sendTimeoutSec, 0};
        if (SOCKAPI_SUCCESS != setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
                                (char*)&sndtime, sizeof(struct timeval))) {
            return SOCKAPI_ERROR;
        }
    }

    if (recvTimeoutSec > 0) {
        struct timeval rcvtime = {recvTimeoutSec, 0};
        if (SOCKAPI_SUCCESS != setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                                (char*)&rcvtime, sizeof(struct timeval))) {
            return SOCKAPI_ERROR;
        }
    }

    return SOCKAPI_SUCCESS;
}


__attribute__((unused))
static int socket_set_conn_opts (int connfd, struct sockconn_opts *opts)
{
    /* set timeouts on current socket connection */
    if (SOCKAPI_SUCCESS != setsocktimeo(connfd, opts->timeosec, opts->timeosec)) {
        printf("error(%d): %s", errno, strerror(errno));
        return SOCKAPI_ERROR;
    }

    /* set keepalive for current socket connection */
    if (1 != setsockalive(connfd, opts->keepidle, opts->keepinterval, opts->keepcount)) {
        printf("error(%d): %s", errno, strerror(errno));
        return SOCKAPI_ERROR;
    }

    /* nodelay=1, TCP_NODELAY disable Nagle, use TCP_CORK to enable Nagle */
    if (opts->nodelay) {
        if (setsockopt (connfd, IPPROTO_TCP, TCP_NODELAY, &opts->nodelay, sizeof(opts->nodelay))) {
            printf("error(%d): %s\n", errno, strerror(errno));
            return SOCKAPI_ERROR;
        }
    } else {
        if (setsockopt (connfd, IPPROTO_TCP, TCP_CORK, &opts->nodelay, sizeof(opts->nodelay))) {
            printf("error(%d): %s\n", errno, strerror(errno));
            return SOCKAPI_ERROR;
        }
    }

    return SOCKAPI_SUCCESS;
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
static inline ssize_t pread_len (int fd, char * buf, size_t len, off_t pos)
{
    ssize_t rc;
    char * pb = buf;

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

#if defined(__cplusplus)
}
#endif

#endif /* SOCKAPI_H_INCLUDED */
