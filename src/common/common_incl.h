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
 * @file: common_incl.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.1
 *
 * @create: 2018-01-09
 *
 * @update: 2018-09-21 16:39:52
 */

#ifndef COMMON_INCL_H_INCLUDED
#define COMMON_INCL_H_INCLUDED

#include <assert.h>  /* assert */
#include <string.h>  /* memset */
#include <stdio.h>   /* printf, perror */
#include <limits.h>  /* realpath */
#include <stdbool.h> /* memset */
#include <ctype.h>

/**
 * MUST include stdlib before jemalloc
 * link: LDFLAGS ?= /usr/local/lib/libjemalloc.a -lpthread
 */
#include <stdlib.h>
#include <jemalloc/jemalloc.h>

#include <unistd.h>          /* close */
#include <fcntl.h>           /* open */
#include <errno.h>           /* errno */
#include <signal.h>

#include <pthread.h>         /* link: -pthread */
#include <sys/types.h>
#include <sys/wait.h>        /* waitpid */
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/sendfile.h>    /* sendfile */
#include <sys/ioctl.h>       /* ioctl, FIONREAD */

#include <dirent.h>          /* open, opendir, closedir, rewinddir, seekdir, telldir, scandir */

#include <time.h>

#include <netinet/in.h>      /* sockaddr_in */
#include <netinet/tcp.h>

#include <arpa/inet.h>       /* inet_addr */
#include <netdb.h>
#include <semaphore.h>
#include <stdarg.h>

#include <getopt.h>          /* getopt_long */
#include <regex.h>           /* regex_t */

/* sqlite3 should be installed first */
#include <sqlite3.h>

/* openssl should be installed first */
#ifdef OPENSSL_NO_MD5
  #undef OPENSSL_NO_MD5
#endif
#ifndef OPENSSL_NO_MD5
  #include <openssl/md5.h>
#endif


#ifndef PATHFILE_MAXLEN
#  define PATHFILE_MAXLEN  PATH_MAX
#endif

#ifndef HOSTNAME_MAXLEN
#  define HOSTNAME_MAXLEN  127
#endif

#ifndef ERRORMSG_MAXLEN
#  define ERRORMSG_MAXLEN  255
#endif

#ifndef BUFFER_MAXSIZE
#  define BUFFER_MAXSIZE  4096
#endif

#endif /* COMMON_INCL_H_INCLUDED */
