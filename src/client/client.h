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

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "../xsync-config.h"
#include "../xsync-error.h"


#if defined(__cplusplus)
extern "C" {
#endif

#define APP_NAME              XSYNC_CLIENT_APPNAME
#define APP_VERSION           XSYNC_CLIENT_VERSION

#define LOGGER_CATEGORY_NAME  APP_NAME

#define XS_DEFAULT_TasksPrethreadMin   8
#define XS_DEFAULT_NumberThreadsMin    2


#include "client_api.h"


/**
 * print usage for app
 *
 * ------ color output ------
 * 30: black
 * 31: red
 * 32: green
 * 33: yellow
 * 34: blue
 * 35: purple
 * 36: cyan
 * 37: white
 * --------------------------
 * "\033[31m RED   \033[0m"
 * "\033[32m GREEN \033[0m"
 * "\033[33m YELLOW \033[0m"
 */
__attribute__((used))
static void print_usage(void)
{
    printf("\033[47;35m* %s, Version: %s, Build: %s %s\033[0m\n",
        APP_NAME, APP_VERSION, __DATE__, __TIME__);
    printf("\033[35mUsage:\033[0m %s [Options]\n", APP_NAME);
    printf("\t\033[35m extremely synchronize files among servers.\033[0m\n");
    printf("\033[35mOptions:\033[0m\n"
        "\t-h, --help                      \033[35m display help messages\033[0m\n"
        "\t-V, --version                   \033[35m print version information\033[0m\n"
        "\n"
        "\t-C, --config=PATHFILE           \033[35m specify path file to conf. '../conf/%s.conf' (default)\033[0m\n"
        "\t-W, --force-watch               \033[35m force to use watch path as an override of config(-C, --config).\033[0m\n"
        "\t                                  \033[35m if enable watch path by '--use-watch', config=PATHFILE will be ignored.\033[0m\n"
        "\t                                  \033[35m NOTE: watch/ folder lives alway in the sibling directory of config PATHFILE, for example:\033[0m\n"
        "\t                                  \033[35m client_home/\033[0m\n"
        "\t                                        \033[35m |\033[0m\n"
        "\t                                        \033[35m +--- CLIENTID    (client id file)\033[0m\n"
        "\t                                        \033[35m |\033[0m\n"
        "\t                                        \033[35m +--- bin/%s-%s\033[0m\n"
        "\t                                        \033[35m |\033[0m\n"
        "\t                                        \033[35m +--- conf/%s.conf    (default config file)\033[0m\n"
        "\t                                        \033[35m |\033[0m\n"
        "\t                                        \033[35m +--- watch/\033[0m\n"
        "\t                                        \033[35m       |\033[0m\n"
        "\t                                        \033[35m       +--- pathlinkA/ -> A    (symlink to path A)\033[0m\n"
        "\t                                        \033[35m       +--- pathlinkB/ -> B    (symlink to path A)\033[0m\n"
        "\t                                        \033[35m       ...\033[0m\n"
        "\n"
        "\t-O, --log4c-rcpath=PATH         \033[35m specify path of log4crc file. '../conf/' (default)\033[0m\n"
        "\t-P, --priority=<PRIORITY>       \033[35m overwrite priority in log4crc, available PRIORITY:\033[0m\n"
        "\t                                      \033[35m 'fatal'\033[0m\n"
        "\t                                      \033[35m 'error' - used in stable release stage\033[0m\n"
        "\t                                      \033[35m 'warn'\033[0m\n"
        "\t                                      \033[35m 'info'  - used in release stage\033[0m\n"
        "\t                                      \033[35m 'debug' - used only in devel\033[0m\n"
        "\t                                      \033[35m 'trace' - show all details\033[0m\n"
        "\n"
        "\t-A, --appender=<APPENDER>       \033[35m overwrite appender in log4crc, available APPENDER:\033[0m\n"
        "\t                                      \033[35m 'default' - using appender specified in log4crc\033[0m\n"
        "\t                                      \033[35m 'stdout' - using appender stdout\033[0m\n"
        "\t                                      \033[35m 'stderr' - using appender stderr\033[0m\n"
        "\t                                      \033[35m 'syslog' - using appender syslog\033[0m\n"
        "\n"
        "\t-t, --threads=<THREADS>         \033[35m specify number of threads. THREADS can also be:\033[0m\n"
        "\t                                      \033[35m  0 - using minimum threads\033[0m\n"
        "\t                                      \033[35m -1 - using maximum threads\033[0m\n"
        "\n"
        "\t-q, --queues=<QUEUES>           \033[35m specify total queues for all threads. QUEUES can also be:\033[0m\n"
        "\t                                      \033[35m  0 - using available minimum queues\033[0m\n"
        "\t                                      \033[35m -1 - using available maximum queues\033[0m\n"
        "\n"
        "\t-I, --replace-clientid=<CLIENTID>\033[35m CAUTION: replace clientid in file CLIENTID\033[0m\n"
        "\n"
        "\t-D, --daemon                \033[35m run as daemon process\033[0m\n"
        "\t-K, --kill                  \033[35m kill all processes for this program\033[0m\n"
        "\t-L, --list                  \033[35m list of pids for this program\033[0m\n"
        "\n"
        "\t-m, --md5=FILE              \033[35m md5sum on given FILE\033[0m\n"
        "\t-r, --regexp=PATTERN        \033[35m use pattern for matching on <express>\033[0m\n"
        "\n"
        "\033[47;35m* COPYRIGHT (c) 2014-2020 PEPSTACK.COM, ALL RIGHTS RESERVED.\033[0m\n",
        APP_NAME, APP_NAME, APP_VERSION, APP_NAME);
}


__attribute__((used))
static int validate_arg_threads (int arg_threads)
{
    int threads = 0;

    if (arg_threads == 0) {
        threads = XSYNC_CLIENT_THREADS_MIN;
    } else if (arg_threads == -1) {
        threads = XSYNC_CLIENT_THREADS_MAX;
    } else if (arg_threads > XSYNC_CLIENT_THREADS_MAX) {
        fprintf(stderr, "\033[1;31m[error]\033[0m too many threads(%d) > %d\033[0m\n",
            arg_threads, XSYNC_CLIENT_THREADS_MAX);
        exit(XS_E_PARAM);
    } else if (arg_threads < XSYNC_CLIENT_THREADS_MIN) {
        fprintf(stderr, "\033[1;31m[error]\033[0m too less threads(%d) < %d\033[0m\n",
            arg_threads, XSYNC_CLIENT_THREADS_MIN);
        exit(XS_E_PARAM);
    } else {
        threads = arg_threads;
    }

    return threads;
}


__attribute__((used))
static int validate_arg_queues (int arg_queues, int threads)
{
    int queues = 0;

    if (threads) {
        if (arg_queues == 0) {
            queues = threads * XSYNC_TASKS_PERTHREAD_MIN;
        } else if (arg_queues == -1) {
            queues = threads * XSYNC_TASKS_PERTHREAD_MAX;
        } else if (arg_queues < threads * XSYNC_TASKS_PERTHREAD_MIN) {
            queues = threads * XSYNC_TASKS_PERTHREAD_MIN;
        } else if (arg_queues > threads * XSYNC_TASKS_PERTHREAD_MAX) {
            queues = threads * XSYNC_TASKS_PERTHREAD_MAX;
        } else {
            queues = arg_queues;
        }
    } else {
        if (arg_queues != 0 && arg_queues != -1) {
            if (arg_queues > XSYNC_CLIENT_QUEUES_MAX) {
                fprintf(stderr, "\033[1;31m[error]\033[0m too many queues(%d > %d)\033[0m\n",
                    arg_queues, XSYNC_CLIENT_QUEUES_MAX);
                exit(XS_E_PARAM);
            }

            if (arg_queues < XSYNC_CLIENT_THREADS_MIN * XSYNC_TASKS_PERTHREAD_MIN) {
                fprintf(stderr, "\033[1;31m[error]\033[0m too less queues(%d < %d)\033[0m\n",
                    arg_queues, XSYNC_CLIENT_THREADS_MIN * XSYNC_TASKS_PERTHREAD_MIN);
                exit(XS_E_PARAM);
            }

            queues = arg_queues;
        }
    }

    return queues;
}


__attribute__((used))
static void validate_client_config (XS_client client)
{
    if (client->threads > XSYNC_CLIENT_THREADS_MAX) {
        LOGGER_WARN("too many THREADS(%d) expected. coerce THREADS=%d", client->threads, XSYNC_CLIENT_THREADS_MAX);
        client->threads = XSYNC_CLIENT_THREADS_MAX;
    } else if (client->threads < XS_DEFAULT_NumberThreadsMin) {
        LOGGER_WARN("too less THREADS(%d) expected. coerce THREADS=%d", client->threads, XS_DEFAULT_NumberThreadsMin);
        client->threads = XS_DEFAULT_NumberThreadsMin;
    }

    if (client->queues > XSYNC_CLIENT_QUEUES_MAX) {
        LOGGER_WARN("too many QUEUES(%d) expected. coerce QUEUES=%d", client->threads, XSYNC_CLIENT_QUEUES_MAX);
        client->queues = XSYNC_CLIENT_QUEUES_MAX;
    } else if (client->queues < client->threads * XS_DEFAULT_TasksPrethreadMin) {
        LOGGER_WARN("too less QUEUES(%d) expected. coerce QUEUES=%d", client->queues, client->threads * XS_DEFAULT_TasksPrethreadMin);
        client->queues = client->threads * XS_DEFAULT_TasksPrethreadMin;
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_H_INCLUDED */
