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


#if defined(__cplusplus)
extern "C" {
#endif

#define APP_NAME              XSYNC_CLIENT_APPNAME
#define APP_VERSION           XSYNC_CLIENT_VERSION

#define XS_DEFAULT_TasksPrethreadMin   8
#define XS_DEFAULT_NumberThreadsMin    2


#include "client_api.h"

#include "../common/common_util.h"

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
__no_warning_unused(static)
void print_usage(void)
{
    #ifdef NDEBUG
        printf("\033[47;35m* %s, NDEBUG version: %s, build: %s %s\033[0m\n",
            APP_NAME, APP_VERSION, __DATE__, __TIME__);

        assert(0 && "RELEASE VERSION SHOULD NOT ENABLE assert() MARCO !");
    #else
        printf("\033[47;35m* %s, DEBUG version: %s, build: %s %s\033[0m\n",
            APP_NAME, APP_VERSION, __DATE__, __TIME__);
    #endif

    printf("\033[35mUsage:\033[0m %s [Options]\n", APP_NAME);
    printf("\t\033[35m extremely synchronize files among servers.\033[0m\n");
    printf("\033[35mOptions:\033[0m\n"
        "\t-h, --help                      \033[35m display help messages\033[0m\n"
        "\t-V, --version                   \033[35m print version information\033[0m\n"
        "\n"
        "\t-C, --config=PATHFILE           \033[35m specify path file to conf. '../conf/%s.conf' (default)\033[0m\n"
        "\t-W, --from-watch                \033[35m config client from watch path no matter if specify config(-C, --config) or not.\033[0m\n"
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

#ifdef DEBUG
    printf("\033[31m**** Caution: DEBUG compiling mode only used in develop stage ! ****\033[0m\n");
    assert(0 && "DEBUG compiling mode enabled.");
#endif
}


__no_warning_unused(static)
int validate_arg_threads (int arg_threads)
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


__no_warning_unused(static)
int validate_arg_queues (int arg_queues, int threads)
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


__no_warning_unused(static)
void clientapp_opts_checkup (clientapp_opts *opts)
{
    if (opts->threads > XSYNC_CLIENT_THREADS_MAX) {
        LOGGER_WARN("too many THREADS(%d) expected. coerce THREADS=%d", opts->threads, XSYNC_CLIENT_THREADS_MAX);
        opts->threads = XSYNC_CLIENT_THREADS_MAX;
    } else if (opts->threads < XS_DEFAULT_NumberThreadsMin) {
        LOGGER_WARN("too less THREADS(%d) expected. coerce THREADS=%d", opts->threads, XS_DEFAULT_NumberThreadsMin);
        opts->threads = XS_DEFAULT_NumberThreadsMin;
    }

    if (opts->queues > XSYNC_CLIENT_QUEUES_MAX) {
        LOGGER_WARN("too many QUEUES(%d) expected. coerce QUEUES=%d", opts->threads, XSYNC_CLIENT_QUEUES_MAX);
        opts->queues = XSYNC_CLIENT_QUEUES_MAX;
    } else if (opts->queues < opts->threads * XS_DEFAULT_TasksPrethreadMin) {
        LOGGER_WARN("too less QUEUES(%d) expected. coerce QUEUES=%d", opts->queues, opts->threads * XS_DEFAULT_TasksPrethreadMin);
        opts->queues = opts->threads * XS_DEFAULT_TasksPrethreadMin;
    }
}


__no_warning_unused(static)
void clientapp_opts_initiate (int argc, char *argv[], clientapp_opts *opts)
{
    int ret;

    char buff[XSYNC_IO_BUFSIZE];

    char config[XSYNC_PATHFILE_MAXLEN + 1];
    char log4crc[XSYNC_PATHFILE_MAXLEN + 1];

    char priority[20] = {0};
    char appender[60] = {0};

    int force_watch = 0;

    int isdaemon = 0;

    int threads = 0;
    int queues = 0;

    bzero(opts, sizeof(*opts));

    /* command arguments */
    const struct option lopts[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {"config", required_argument, 0, 'C'},
        {"from-watch", no_argument, 0, 'W'},
        {"log4c-rcpath", required_argument, 0, 'O'},
        {"priority", required_argument, 0, 'P'},
        {"appender", required_argument, 0, 'A'},
        {"threads", required_argument, 0, 't'},
        {"queues", required_argument, 0, 'q'},
        {"update-clientid", required_argument, 0, 'I'},
        {"daemon", no_argument, 0, 'D'},
        {"kill", no_argument, 0, 'K'},
        {"list", no_argument, 0, 'L'},
        {"md5", required_argument, 0, 'm'},
        {"regexp", required_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    /**
     * get default real path for xsync-client.conf
     */
    ret = realpathdir(argv[0], buff, sizeof(buff));
    if (ret <= 0) {
        fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buff);
        exit(-1);
    }

    if (strrchr(buff, '/') == strchr(buff, '/')) {
        fprintf(stderr, "\033[1;31m[error]\033[0m cannot run under root path: %s\n", buff);
        exit(-1);
    }

    *strrchr(buff, '/') = 0;
    *(strrchr(buff, '/') + 1) = 0;

    ret = snprintf(config, sizeof(config), "%sconf/%s-conf.ini", buff, APP_NAME);
    if (ret < 20 || ret >= sizeof(config)) {
        fprintf(stderr, "\033[1;31m[error]\033[0m invalid conf path: %s\n", buff);
        exit(-1);
    }

    ret = snprintf(log4crc, sizeof(log4crc), "LOG4C_RCPATH=%sconf/", buff);
    if (ret < 20 || ret >= sizeof(log4crc)) {
        fprintf(stderr, "\033[1;31m[error]\033[0m invalid log4c path: %s\n", buff);
        exit(-1);
    }

    /* parse command arguments */
    while ((ret = getopt_long(argc, argv, "DKLhVC:WO:P:A:t:q:I:m:r:", lopts, 0)) != EOF) {
        switch (ret) {
        case 'D':
            isdaemon = 1;
            break;

        case 'h':
            print_usage();
            exit(0);
            break;

        case 'C':
            /* overwrite default config file */
            ret = snprintf(config, sizeof(config), "%s", optarg);
            if (ret < 20 || ret >= sizeof(config)) {
                fprintf(stderr, "\033[1;31m[error]\033[0m specified invalid conf file: %s\n", optarg);
                exit(-1);
            }

            if (getfullpath(config, buff, sizeof(buff)) != 0) {
                fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buff);
                exit(-1);
            } else {
                ret = snprintf(config, sizeof(config), "%s", buff);
                if (ret < 20 || ret >= sizeof(config)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid conf file: %s\n", buff);
                    exit(-1);
                }
            }
            break;

        case 'W':
            force_watch = 1;
            break;

        case 'O':
            /* overwrite default log4crc file */
            ret = snprintf(log4crc, sizeof(log4crc), "%s", optarg);
            if (ret < 0 || ret >= sizeof(log4crc)) {
                fprintf(stderr, "\033[1;31m[error]\033[0m specified invalid log4c path: \033[31m%s\033[0m\n", optarg);
                exit(-1);
            }

            if (getfullpath(log4crc, buff, sizeof(buff)) != 0) {
                fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buff);
                exit(-1);
            } else {
                ret = snprintf(log4crc, sizeof(log4crc), "LOG4C_RCPATH=%s", buff);
                if (ret < 10 || ret >= sizeof(log4crc)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid log4c path: %s\n", buff);
                    exit(-1);
                }
            }
            break;

        case 'P':
            ret = snprintf(priority, sizeof(priority), "%s", optarg);
            if (ret < 0 || ret >= sizeof(priority)) {
                fprintf(stderr, "\033[1;31m[error]\033[0m specified invalid priority: %s\n", optarg);
                exit(-1);
            }
            break;

        case 'A':
            ret = snprintf(appender, sizeof(appender), "%s", optarg);
            if (ret < 0 || ret >= sizeof(appender)) {
                fprintf(stderr, "\033[1;31m[error]\033[0m specified invalid appender: \033[31m%s\033[0m\n", optarg);
                exit(-1);
            }
            break;

        case 't':
            threads = validate_arg_threads(atoi(optarg));
            break;

        case 'q':
            queues = validate_arg_queues(atoi(optarg), threads);
            break;

        case 'I':
            exit(0);
            break;

        case 'm':
            ret = check_file_mode(optarg, R_OK);

            if (ret) {
                fprintf(stderr, "\033[1;31m[error: md5sum]\033[0m file not found: %s\n\n", optarg);
            } else {
                ret = md5sum_file(optarg, buff, sizeof(buff));

                if (ret == 0) {
                    fprintf(stdout, "\033[1;32m[success: md5sum]\033[0m %s (%s)\n", buff, optarg);
                } else {
                    fprintf(stderr, "\033[1;31m[error: md5sum]\033[0m file: %s\n", optarg);
                }
            }
            exit(ret);
            break;

        case 'K':
            exit(0);
            break;

        case 'L':
            exit(0);
            break;

        case 'r':
            exit(0);
            break;

        case 'V':
            fprintf(stdout, "\033[1;35m%s, Version: %s, Build: %s %s\033[0m\n\n", APP_NAME, APP_VERSION, __DATE__, __TIME__);
            exit(0);
            break;
        }
    }

    fprintf(stdout, "\033[1;34m* Default log4c path : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[1;34m* Default config file: %s\033[0m\n\n", config);
    fprintf(stdout, "\033[1;32m* Using log4c path   : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[1;32m* Using config file  : %s\033[0m\n", config);

    if (threads > 0) {
        // 用户指定了线程和队列用于覆盖配置文件
        queues = validate_arg_queues(queues, threads);
        fprintf(stdout, "\033[1;32m* Overwritten config : (threads=%d, queues=%d)\033[0m\n\n", threads, queues);
    }

    if (force_watch) {
        // 强迫从 watch 目录自动配置
        snprintf(buff, sizeof(buff), "%s", config);

        *strrchr(buff, '/') = 0;
        *strrchr(buff, '/') = 0;
        strcat(buff, "/watch");

        if (! isdir(buff)) {
            fprintf(stderr, "\033[1;31m[error] NOT a directory:\033[0m %s\n\n", buff);
            exit(-1);
        }

        if (0 != access(buff, F_OK|R_OK|X_OK)) {
            fprintf(stderr, "\033[1;31m[error] watch error(%d): %s.\033[0m (%s)\n\n", errno, strerror(errno), buff);
            exit(-1);
        }

        snprintf(config, sizeof(config), "%s", buff);
        fprintf(stdout, "\033[1;36m* Force using watch  : %s\033[0m\n", config);
    }

    // 设置log4c
    config_log4crc(APP_NAME, log4crc, priority, appender, sizeof(appender), buff, sizeof(buff));

    // 得到启动命令
    ret = getstartcmd(argc, argv, buff, sizeof(buff), APP_NAME);
    if (ret) {
        fprintf(stderr, "\033[1;31m[error: getstartcmd]\033[0m %s\n\n", buff);
        exit(-1);
    }

    // 输出选项
    ret = strlen(buff) + 1;
    opts->startcmd = (char *) malloc(ret + 1);
    memcpy(opts->startcmd, buff, ret);
    opts->startcmd[ret] = 0;

    opts->isdaemon = isdaemon;
    opts->threads = threads;
    opts->queues = queues;
    opts->force_watch = force_watch;

    memcpy(opts->config, config, XSYNC_PATHFILE_MAXLEN);
    opts->config[XSYNC_PATHFILE_MAXLEN] = 0;
}


__no_warning_unused(static)
void clientapp_opts_cleanup (clientapp_opts *opts)
{
    // TODO:
}


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_H_INCLUDED */
