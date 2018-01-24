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
* common.h
*   common api
*
* Last Updated: 2017-01-09
* Last Updated: 2018-01-21
*/
#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "xsync-config.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#include "header.h"
#include "logger.h"

/* common */
#include "common/refobject.h"
#include "common/byteorder.h"
#include "common/readconf.h"
#include "common/threadpool.h"

/* socket api */
#include "sockapi.h"
//#include "sslapi.h"

/* inotify api */
#include "inotiapi.h"


/* memory helper api */
__attribute__((used)) __attribute__((malloc))
static inline void * mem_alloc (int num, size_t size)
{
    void * pv = calloc(num, size);
    if (! pv) {
        perror("calloc");
        exit(-1);
    }
    return pv;
}


__attribute__((used))
static inline void mem_free (void **ppv)
{
    if (ppv) {
        void * pv = *ppv;
        if (pv) {
            free(pv);
        }
        *ppv = 0;
    }
}


/**
* The compiler tries to warn you that you lose bits when casting from void *
*   to int. It doesn't know that the void * is actually an int cast, so the
*   lost bits are meaningless.
*
* A double cast would solve this (int)(uintptr_t)t->key. It first casts void *
*   to uintptr_t (same size, no warning), then uintptr_t to int (number to
*   number, no warning). Need to include <stdint.h> to have the uintptr_t type
*   (an integral type with the same size as a pointer).
*/
#define pv_cast_to_int(pv)    ((int) (uintptr_t) (void*) (pv))
#define int_cast_to_pv(ival)    ((void*) (uintptr_t) (int) (ival))


/**
 * trim specified character in given string
 */
__attribute__((used))
static inline char * trim (char * s, char c)
{
    return (*s==0)?s:(((*s!=c)?(((trim(s+1,c)-1)==s)?s:(*(trim(s+1,c)-1)=*s,*s=c,trim(s+1,c))):trim(s+1,c)));
}


/**
 * trim specified characters in given string
 */
__attribute__((used))
static inline char * trims (char *s, char *chrs)
{
    char *p = chrs;
    while (*p != 0) {
        s = trim(s, *p++);
    }
    return s;
}


static inline char * strrplchr (char *s, char c, char d)
{
    char * p = s;
    while (*p) {
        if (*p == c) {
            *p = d;
        }
        ++p;
    }
    return s;
}


#ifndef _MSC_VER
__attribute__((used))
static inline char * strlwr(char * str)
{
    if (! str) {
        return str;
    } else {
        char *p = str;
        while (*p != 0) {
            if (*p >= 'A' && *p <= 'Z') {
                *p = (*p) + 0x20;
            }
            p++;
        }
        return str;
    }
}


__attribute__((used))
static inline char * strupr(char * str)
{
    char * p = str;

    for (; *str != 0; str++) {
        *str = toupper(*str);
    }

    return p;
}
#endif /* _MSC_VER */


__attribute__((used))
static int isdir(const char *path)
{
    struct stat sb;
    int rc;
    rc = stat(path, &sb);
    if (rc == 0 && (sb.st_mode & S_IFDIR)) {
        /* is dir */
        return 1;
    } else {
        /* not dir */
        return 0;
    }
}


/**
 * getpwd
 *   Get absolute path (end with '/') for current process.
 *
 * last: 2017-01-09
 */
__attribute__((used))
static int getpwd (char *path, int size)
{
    ssize_t r;
    char * p;

    r = readlink("/proc/self/exe", path, size);
    if (r < 0) {
        strncpy(path, strerror(errno), size - 1);
        path[size-1] = 0;
        return 0;
    }

    if (r >= size) {
        snprintf(path, size, "insufficent buffer");
        return 0;
    }
    path[r] = '\0';

    p = strrchr(path, '/');
    if (! p) {
        snprintf(path, size, "invalid link");
        return 0;
    }
    *p = 0;

    return p - path;
}


__attribute__((used))
static int getfullpath (char * path, char * outpath, size_t size_outpath)
{
    int rc;

    char * p;

    char savedcwd[512];
    char fullpath[512];
    char name[128];

    *name = 0;

    /* save current work dir */
    p = getcwd(savedcwd, sizeof(savedcwd));
    if (! p) {
        snprintf(outpath, size_outpath, "getcwd error(%d): %s", errno, strerror(errno));
        outpath[size_outpath - 1] = 0;
        return (-1);
    }

    if (! isdir(path)) {
        p = strrchr(path, '/');
        if (! p) {
            snprintf(outpath, size_outpath, "invalid path: %s", path);
            outpath[size_outpath - 1] = 0;
            return (-1);
        }
        *p++ = 0;

        rc = snprintf(name, sizeof(name), "%s", p);
        if (rc < 0 || rc >= sizeof(name)) {
            snprintf(outpath, size_outpath, "invalid file name: %s", name);
            outpath[size_outpath - 1] = 0;
            return (-1);
        }
    }

    rc = chdir(path);
    if (rc != 0) {
        snprintf(outpath, size_outpath, "chdir error(%d): %s", errno, strerror(errno));
        outpath[size_outpath - 1] = 0;
        return (-1);
    }

    p = getcwd(fullpath, sizeof(fullpath));
    if (! p) {
        snprintf(outpath, size_outpath, "getcwd error(%d): %s", errno, strerror(errno));
        outpath[size_outpath - 1] = 0;
        chdir(savedcwd);
        return (-1);
    }

    /* restore current work dir */
    rc = chdir(savedcwd);
    if (rc != 0) {
        snprintf(outpath, size_outpath, "chdir error(%d): %s", errno, strerror(errno));
        outpath[size_outpath - 1] = 0;
        return (-1);
    }

    rc = snprintf(outpath, size_outpath, "%s/%s", fullpath, name);
    if (rc <= 0 || rc >= size_outpath) {
        snprintf(outpath, size_outpath, "invalid full path: %s", fullpath);
        outpath[size_outpath - 1] = 0;
        return (-1);
    }

    return 0;
}


__attribute__((used))
static const char* cmd_system(const char * cmd, char *outbuf, ssize_t outsize)
{
    char * result = 0;

    FILE * fp = popen(cmd, "r");
    if (fp) {
        bzero(outbuf, outsize);

        while (fgets(outbuf, outsize, fp) != 0) {
            result = outbuf;
        }

        pclose(fp);
    }

    return result;
}


typedef void (*sighandler_t)(int);

__attribute__((used))
static int pox_system (const char * cmd)
{
    int ret = 0;
    sighandler_t old_handler;

    old_handler = signal(SIGCHLD, SIG_DFL);
    ret = system(cmd);
    if (ret != 0) {
        perror(cmd);
    }
    signal(SIGCHLD, old_handler);
    return ret;
}


__attribute__((used))
static int fileislink (const char * pathfile, char * inbuf, ssize_t inbufsize)
{
    char * buf;

    ssize_t bufsize;
    ssize_t size;

    if (inbuf) {
        bufsize = inbufsize;
        buf = inbuf;
    } else {
        bufsize = PATH_MAX + 1;
        buf = (char *) malloc(bufsize);
    }

    size = readlink(pathfile, buf, bufsize);

    if (buf != inbuf) {
        free(buf);
    }

    if (size == bufsize) {
        if (inbuf) {
            snprintf(inbuf, inbufsize, "buff is too small");
            inbuf[inbufsize - 1] = 0;
        }
        return (-1);
    }

    if (size == -1) {
        /** errno:
         *    http://www.virtsync.com/c-error-codes-include-errno
         */
        if (errno == EINVAL) {
            /* file not a link */
            return 0;
        }

        if (inbuf) {
            snprintf(inbuf, inbufsize, "fileislink error(%d): %s", errno, strerror(errno));
            inbuf[inbufsize - 1] = 0;
        }

        /* fileislink error */
        return (-2);
    }

    /* 1: fileislink ok */
    if (inbuf) {
        inbuf[size] = 0;
    }
    return 1;
}


__attribute__((used))
static const char * getbindir (char * cmd, char * bindir, int size)
{
    char * p = strrchr(cmd, '/');
    *bindir = 0;

    if (p) {
        /* get only abspath to dir */
        char dir[XSYNC_PATHFILE_MAXLEN + 1];
        strcpy(dir, cmd);
        dir[ p - cmd ] = 0;
        realpath(dir, bindir);
    } else {
        /* is slink */
        snprintf(bindir, size, "%s", "/usr/local/bin");
    }

    bindir[size-1] = 0;
    return bindir;
}

// http://www.virtsync.com/c-error-codes-include-errno
__attribute__((used))
static int getstartcmd (int argc, char ** argv, char * cmdbuf, ssize_t bufsize)
{
    int err;
    char * file;

    err = fileislink(argv[0], cmdbuf, bufsize);
    if (err == 1) {
        printf("file is link: %s\n", cmdbuf);
    } else if (err == 0) {
        printf("file not link: %s\n", argv[0]);
    } else {
        printf("%d - %s\n", err, cmdbuf);
        return (-1);
    }

    file = realpath(argv[0], 0);
    printf("realpath for (%s) is: %s\n", argv[0], file);
    free(file);

    return 0;
}


__attribute__((used))
static void config_log4crc (const char * catname, char * log4crc, char * priority, char * appender, size_t size_appender)
{
    int i, ret;

    struct stat sb;

    char cmd[512];
    char result[256];

    char old_priority[30] = "info";
    char old_appender[60] = "stdout";
    char old_category[256] = {0};

    char category[256] = {0};

    int default_appender = 0;

    int endpos = strlen(log4crc);

    const char * log4crc_file = strchr(log4crc, '/');

    strcat(log4crc, "log4crc");

    if (stat(log4crc_file, &sb) == -1 && errno == ENOENT) {
        perror(log4crc_file);
        exit(-1);
    }

    if (access(log4crc_file, F_OK | R_OK | W_OK) != 0) {
        perror(log4crc_file);
        exit(-1);
    }

    /* get old priority from log4crc */
    ret = snprintf(cmd, sizeof(cmd),
            "grep '<category name=\"%s\" priority=\"' '%s' | sed -r 's/.* priority=\"(.*)\" appender=\".*/\\1/'",
            catname, log4crc_file);
    if (ret < 0 || ret >= sizeof(cmd)) {
        fprintf(stderr, "\033[31m[error]\033[0m insufficent string buffer for cmd");
        exit(-1);
    }

    if (cmd_system(cmd, result, sizeof(result))) {
        ret = snprintf(old_priority, sizeof(old_priority), "%s", trims(result, " \n"));
        if (ret < 0 || ret >= sizeof(old_priority)) {
            fprintf(stderr, "\033[31m[error]\033[0m insufficent string buffer for priority");
            exit(-1);
        }
    }

    /* get old appender from log4crc */
    ret = snprintf(cmd, sizeof(cmd),
            "grep '<category name=\"%s\" priority=\"' '%s' | sed -r 's/.* appender=\"(.*)\" .*/\\1/'",
            catname, log4crc_file);
    if (ret < 0 || ret >= sizeof(cmd)) {
        fprintf(stderr, "\033[31m[error]\033[0m insufficent string buffer for cmd");
        exit(-1);
    }

    if (cmd_system(cmd, result, sizeof(result))) {
        ret = snprintf(old_appender, sizeof(old_appender), "%s", trims(result, " \n"));
        if (ret < 0 || ret >= sizeof(old_appender)) {
            fprintf(stderr, "\033[31m[error]\033[0m insufficent string buffer for appender\n");
            exit(-1);
        }
    }

    /* make up old category */
    ret = snprintf(old_category, sizeof(old_category),
            "<category name=\"%s\" priority=\"%s\" appender=\"%s\" \\/>",
            catname, old_priority, old_appender);
    if (ret < 0 || ret >= sizeof(old_category)) {
        perror("\033[31m[error]\033[0m insufficent string buffer for category");
        exit(-1);
    }

    /* update log4crc with priority and appender */

    if (*priority) {
        const char * priorities[] = {
            "fatal",
            "error",
            "warn",
            "info",
            "debug",
            "trace",
            0
        };

        strlwr(priority);

        i = 0;
        while (priorities[i]) {
            if (! strcmp(priority, priorities[i])) {
                break;
            }
            i++;
        }

        if (! priorities[i]) {
            fprintf(stderr, "\033[31m[error]\033[0m unknown priority: \033[31m%s\033[0m\n * should be one of the following:\n", priority);
            i = 0;
            while (priorities[i]) {
                fprintf(stderr, " - \033[32m%s\033[0m\n", priorities[i++]);
            }
            exit(-1);
        }
    } else {
        strcpy(priority, "debug");
    }

    if (*appender) {
        const char * appenders[] = {
            "default",
            "stdout",
            "stderr",
            "syslog",
            0
        };

        strlwr(appender);

        i = 0;
        while (appenders[i]) {
            if (! strcmp(appender, appenders[i])) {
                break;
            }
            i++;
        }

        if (! appenders[i]) {
            fprintf(stderr, "\033[31m[error]\033[0m unknown appender: \033[31m%s\033[0m\n * should be one of the following:\n", appender);
            i = 0;
            while (appenders[i]) {
                fprintf(stderr, " - \033[32m%s\033[0m\n", appenders[i++]);
            }
            exit(-1);
        }

        if (i == 0) {
            default_appender = 1;
        }
    } else {
        default_appender = 1;
    }

    if (default_appender) {
        ret = snprintf(appender, size_appender, "%s-appender", catname);
        if (ret < 0 || ret >= size_appender) {
            perror("\033[31m[error]\033[0m insufficent string buffer for appender");
            exit(-1);
        }
    }

    /* make up new category */
    ret = snprintf(category, sizeof(category),
            "<category name=\"%s\" priority=\"%s\" appender=\"%s\" \\/>",
            catname, priority, appender);
    if (ret < 0 || ret >= sizeof(category)) {
        perror("\033[31m[error]\033[0m insufficent string buffer for category");
        exit(-1);
    }

    if (strcmp(category, old_category)) {
        ret = snprintf(cmd, sizeof(cmd), "sed -i 's/%s/%s/g' '%s'", old_category, category, log4crc_file);
        if (ret < 0 || ret >= sizeof(category)) {
            perror("\033[31m[error]\033[0m insufficent string buffer for cmd");
            exit(-1);
        }

        ret = pox_system(cmd);
        if (ret != 0) {
            perror("pox_system");
            exit(-1);
        }
    }

    if (default_appender) {
        /* get and check logdir from log4crc */
        ret = snprintf(cmd, sizeof(cmd),
                "grep '<appender name=\"%s-appender\" logdir=\"' '%s' | sed -r 's/.* logdir=\"(.*)\".*/\\1/'",
                catname, log4crc_file);
        if (ret < 0 || ret >= sizeof(cmd)) {
            perror("\033[31m[error]\033[0m insufficent string buffer for cmd");
            exit(-1);
        }

        if (cmd_system(cmd, result, sizeof(result))) {
            ret = snprintf(cmd, sizeof(cmd), "%s", trims(result, " \n"));
            if (ret <= 0 || ret >= sizeof(cmd)) {
                perror("\033[31m[error]\033[0m insufficent string buffer for cmd");
                exit(-1);
            }
        }

        fprintf(stdout, "* [log4crc]   logdir : \033[35m%s\033[0m\n", cmd);

        if (stat(cmd, &sb) == -1 && errno == ENOENT) {
            fprintf(stderr, "\033[31m* [error] stat() - %s:\033[0m %s\n", strerror(errno), cmd);
            exit(-1);
        }

        if (access(cmd, F_OK | R_OK | W_OK) != 0) {
            fprintf(stderr, "\033[31m* [error] access() - %s:\033[0m %s\n", strerror(errno), cmd);
            exit(-1);
        }
    }

    fprintf(stdout, "* [log4crc]   rcfile : \033[35m%s\033[0m\n", log4crc_file);
    fprintf(stdout, "* [log4crc] priority : \033[35m%s\033[0m\n", priority);
    fprintf(stdout, "* [log4crc] appender : \033[35m%s\033[0m\n", appender);

    log4crc[endpos] = 0;
    if (0 != putenv(log4crc)) {
        fprintf(stderr, "\033[31m* [error] putenv() - %s:\033[0m %s\n", strerror(errno), log4crc);
        exit(-1);
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* COMMON_H_INCLUDED */
