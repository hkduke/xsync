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


#ifndef HAVE_GETRUSAGE_PROTO
int  getrusage (int, struct rusage *);
#endif


__attribute__((used))
static void print_cpu_time (void)
{
    double user, sys;
    struct rusage myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0) {
        printf("\ngetrusage(RUSAGE_SELF) error.\n");
    }

    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0) {
        printf("\ngetrusage(RUSAGE_CHILDREN) error.\n");
    }

    user = (double) myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec / 1000000.0;
    user += (double) childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec / 1000000.0;
    sys = (double) myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec / 1000000.0;
    sys += (double) childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec / 1000000.0;

    printf("\n* [ user-time = %g, sys-time = %g ]\n", user, sys);
}


#ifndef _SIGNAL_H
typedef void sigfunc(int);

__attribute__((used))
static sigfunc * signal (int signo, sigfunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM) {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;  /* SunOS 4.x */
#endif
    } else {
#ifdef  SA_RESTART
        act.sa_flags |= SA_RESTART;    /* SVR4, 4.4BSD */
#endif
    }

    if (sigaction (signo, &act, &oact) < 0) {
        return (SIG_ERR);
    } else {
        return (oact.sa_handler);
    }
}
#endif


/**
 * memory helper api
 */
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
static int check_file_mode (const char * file, int mode /* R_OK, W_OK */)
{
    if (0 == access(file, mode)) {
        return 0;
    } else {
        perror(file);
        return -1;
    }
}


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

    char savedcwd[PATH_MAX + 1];
    char fullpath[PATH_MAX + 1];
    char name[XSYNC_HOSTNAME_MAXLEN + 1];

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

    /* success */
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
            /** file not a link:
             *    When returned inbuf is undetermined
             */
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


/**
 * returns:
 *
 *   success:
 *     > 0 - length of rpdir
 *
 *   error:
 *     <= 0 - failed anyway
 */
__attribute__((used))
static int realpathdir (const char * file, char * rpdir, size_t size)
{
    char * p;

    p = realpath(file, 0);

    if (p) {
        int n = snprintf(rpdir, size, "%s", p);

        free(p);

        if (n >= size) {
            snprintf(rpdir, size, "insufficent buffer for realpath");
            rpdir[size - 1] = 0;
            return 0;
        }

        if (n < 0) {
            snprintf(rpdir, size, "snprintf error(%d): %s", errno, strerror(errno));
            rpdir[size - 1] = 0;
            return (-1);
        }

        p = strrchr(rpdir, '/');
        if (p) {
            *++p = 0;

            /* success return here */
            return (int)(p - rpdir);
        }

        snprintf(rpdir, size, "invlid path: %s", file);
        rpdir[size - 1] = 0;

        return 0;
    }

    snprintf(rpdir, size, "realpath error(%d): %s", errno, strerror(errno));
    rpdir[size - 1] = 0;

    return (-2);
}


/**
 * http://www.virtsync.com/c-error-codes-include-errno
 */
__attribute__((used))
static int getstartcmd (int argc, char ** argv, char * cmdbuf, ssize_t bufsize, const char * link_name)
{
    int err;
    char *binfile;
    char linkpath[PATH_MAX + 1];

    err = fileislink(argv[0], cmdbuf, bufsize);
    if (err == 1) {
        // 取得链接所在的物理目录: 先取得链接所在目录, 然后取得其路径
        strcpy(cmdbuf, argv[0]);
        binfile = strrchr(cmdbuf, '/');
        *binfile++ = 0;

        err = getfullpath(cmdbuf, linkpath, sizeof(linkpath));
        if (err != 0) {
            return (-1);
        }

        // 执行的链接文件全路径: linkpath
        strcat(linkpath, binfile);

        // 执行的物理文件全路径: cmdbuf
        realpath(argv[0], cmdbuf);
    } else if (err == 0) {
        // 取得文件所在的物理目录
        binfile = strrchr(argv[0], '/');
        binfile++;

        err = realpathdir(argv[0], cmdbuf, bufsize);
        if (err <= 0) {
            return (-1);
        }

        // 链接文件全路径
        snprintf(linkpath, sizeof(linkpath), "%s%s", cmdbuf, link_name);

        // 执行的物理文件全路径
        strcat(cmdbuf, binfile);

        // 创建链接: linkpath -> cmdbuf
        if (symlink(cmdbuf, linkpath) != 0 && errno != EEXIST) {
            // 创建链接失败
            snprintf(cmdbuf, bufsize, "symlink error(%d): %s", errno, strerror(errno));
            return (-1);
        }
    } else {
        // error
        return (-1);
    }

    do {
        // 组合start cmd
        int i;

        char *cmd = cmdbuf;
        ssize_t size = bufsize;

        int offset = snprintf(cmd, size, "%s", linkpath);

        if (offset < 0 || offset >= size) {
            cmdbuf[bufsize - 1] = 0;
            return -2;
        }
        cmd += offset;
        size -= offset;

        for (i = 1; i < argc; ++i) {
            offset = snprintf(cmd, size, " '%s'", argv[i]);

            if (offset < 0 || offset >= size) {
                cmdbuf[bufsize - 1] = 0;
                return -2;
            }
            cmd += offset;
            size -= offset;
        }

        cmdbuf[bufsize - 1] = 0;
    } while(0);

    // 成功返回
    return 0;
}


__attribute__((used))
static void config_log4crc (const char * catname, char * log4crc, char * priority, char * appender, size_t sizeappd, char * buff, ssize_t sizebuf)
{
    int i, ret;

    struct stat sb;

    char result[512];

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
    ret = snprintf(buff, sizebuf,
            "grep '<category name=\"%s\" priority=\"' '%s' | sed -r 's/.* priority=\"(.*)\" appender=\".*/\\1/'",
            catname, log4crc_file);
    if (ret < 0 || ret >= sizebuf) {
        fprintf(stderr, "\033[31m[error]\033[0m insufficent buff");
        exit(-1);
    }

    if (cmd_system(buff, result, sizeof(result))) {
        ret = snprintf(old_priority, sizeof(old_priority), "%s", trims(result, " \n"));
        if (ret < 0 || ret >= sizeof(old_priority)) {
            fprintf(stderr, "\033[31m[error]\033[0m insufficent buffer for priority");
            exit(-1);
        }
    }

    /* get old appender from log4crc */
    ret = snprintf(buff, sizebuf,
            "grep '<category name=\"%s\" priority=\"' '%s' | sed -r 's/.* appender=\"(.*)\" .*/\\1/'",
            catname, log4crc_file);
    if (ret < 0 || ret >= sizebuf) {
        fprintf(stderr, "\033[31m[error]\033[0m insufficent buff");
        exit(-1);
    }

    if (cmd_system(buff, result, sizeof(result))) {
        ret = snprintf(old_appender, sizeof(old_appender), "%s", trims(result, " \n"));
        if (ret < 0 || ret >= sizeof(old_appender)) {
            fprintf(stderr, "\033[31m[error]\033[0m insufficent buffer for appender\n");
            exit(-1);
        }
    }

    /* make up old category */
    ret = snprintf(old_category, sizeof(old_category),
            "<category name=\"%s\" priority=\"%s\" appender=\"%s\" \\/>",
            catname, old_priority, old_appender);
    if (ret < 0 || ret >= sizeof(old_category)) {
        perror("\033[31m[error]\033[0m insufficent buffer for category");
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
        ret = snprintf(appender, sizeappd, "%s-appender", catname);
        if (ret < 0 || ret >= sizeappd) {
            perror("\033[31m[error]\033[0m insufficent buffer for appender");
            exit(-1);
        }
    }

    /* make up new category */
    ret = snprintf(category, sizeof(category),
            "<category name=\"%s\" priority=\"%s\" appender=\"%s\" \\/>",
            catname, priority, appender);
    if (ret < 0 || ret >= sizeof(category)) {
        perror("\033[31m[error]\033[0m insufficent buffer for category");
        exit(-1);
    }

    if (strcmp(category, old_category)) {
        ret = snprintf(buff, sizebuf, "sed -i 's/%s/%s/g' '%s'", old_category, category, log4crc_file);
        if (ret < 0 || ret >= sizeof(category)) {
            perror("\033[31m[error]\033[0m insufficent buff");
            exit(-1);
        }

        ret = pox_system(buff);
        if (ret != 0) {
            perror("pox_system");
            exit(-1);
        }
    }

    if (default_appender) {
        /* get and check logdir from log4crc */
        ret = snprintf(buff, sizebuf,
                "grep '<appender name=\"%s-appender\" logdir=\"' '%s' | sed -r 's/.* logdir=\"(.*)\".*/\\1/'",
                catname, log4crc_file);
        if (ret < 0 || ret >= sizebuf) {
            perror("\033[31m[error]\033[0m insufficent buff");
            exit(-1);
        }

        if (cmd_system(buff, result, sizeof(result))) {
            ret = snprintf(buff, sizebuf, "%s", trims(result, " \n"));
            if (ret <= 0 || ret >= sizebuf) {
                perror("\033[31m[error]\033[0m insufficent buff");
                exit(-1);
            }
        }

        fprintf(stdout, "* [log4crc]   logdir : \033[35m%s\033[0m\n", buff);

        if (stat(buff, &sb) == -1 && errno == ENOENT) {
            fprintf(stderr, "\033[31m* [error] stat() - %s:\033[0m %s\n", strerror(errno), buff);
            exit(-1);
        }

        if (access(buff, F_OK | R_OK | W_OK) != 0) {
            fprintf(stderr, "\033[31m* [error] access() - %s:\033[0m %s\n", strerror(errno), buff);
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


/**
 * MUST equal with:
 *   $ md5sum $file
 *   $ openssl md5 $file
 */
__attribute__((used))
static int md5sum_file (const char * filename, char * buf, size_t bufsize)
{
    FILE * fd;

    fd = fopen(filename, "rb");
    if ( ! fd ) {
        return (-1);
    } else {
        MD5_CTX  ctx;
        int err = 1;

        MD5_Init(&ctx);

        for (;;) {
            size_t len = fread(buf, 1, bufsize, fd);

            if (len == 0) {
                err = ferror(fd);

                if (! err) {
                    unsigned char md5[16];

                    MD5_Final(md5, &ctx);

                    for (len = 0; len < sizeof(md5); ++len) {
                        snprintf(buf + len * 2, 3, "%02x", md5[len]);
                    }
                }

                buf[32] = 0;

                fclose(fd);
                break;
            }

            MD5_Update(&ctx , (const void *) buf, len);
        }

        return err;
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* COMMON_H_INCLUDED */
