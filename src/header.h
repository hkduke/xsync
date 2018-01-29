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
 * header.h
 *   common linux header files
 *
 * author: master@pepstack.com
 *
 * create: 2014-08-01
 * update: 2017-01-09
 */
#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED

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

#endif /* HEADER_H_INCLUDED */
