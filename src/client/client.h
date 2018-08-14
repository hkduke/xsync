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
 * @file: client.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.6
 *
 * @create: 2018-01-24
 *
 * @update: 2018-08-13 17:25:32
 */

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED


#if defined(__cplusplus)
extern "C" {
#endif

#define APP_NAME              XSYNC_CLIENT_APPNAME
#define APP_VERSION           XSYNC_CLIENT_VERSION

#include "client_api.h"

#include "../common/cshell.h"
#include "../common/common_util.h"


/**
 * print usage for app
 *
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
    printf("\t\033[35m xsync client (client of extremely synchronizing files).\033[0m\n");
    printf("\033[35mOptions:\033[0m\n"
        "\t-h, --help                   \033[35m display help messages\033[0m\n"
        "\t-V, --version                \033[35m print version information\033[0m\n"
        "\n"
        "\t-C, --config=PATHFILE        \033[35m specify path file to conf. '../conf/%s.conf' (default)\033[0m\n"
        "\t-W, --from-watch             \033[35m config client from watch path no matter if specify config(-C, --config) or not.\033[0m\n"
        "\t                               \033[35m if enable watch path by '--use-watch', config=PATHFILE will be ignored.\033[0m\n"
        "\t                               \033[35m NOTE: watch/ folder lives alway in the sibling directory of config PATHFILE, for example:\033[0m\n"
        "\t                               \033[35m client_home/\033[0m\n"
        "\t                                     \033[35m |\033[0m\n"
        "\t                                     \033[35m +--- CLIENTID    (client id file)\033[0m\n"
        "\t                                     \033[35m |\033[0m\n"
        "\t                                     \033[35m +--- bin/%s-%s\033[0m\n"
        "\t                                     \033[35m |\033[0m\n"
        "\t                                     \033[35m +--- conf/%s.conf    (default config file)\033[0m\n"
        "\t                                     \033[35m |\033[0m\n"
        "\t                                     \033[35m +--- watch/\033[0m\n"
        "\t                                     \033[35m       |\033[0m\n"
        "\t                                     \033[35m       +--- pathlinkA/ -> A    (symlink to path A)\033[0m\n"
        "\t                                     \033[35m       +--- pathlinkB/ -> B    (symlink to path A)\033[0m\n"
        "\t                                     \033[35m       ...\033[0m\n"
        "\n"
        "\t-O, --log4c-rcpath=PATH      \033[35m specify path of log4crc file. '../conf/' (default)\033[0m\n"
        "\t-P, --priority=<PRIORITY>    \033[35m overwrite priority in log4crc. available PRIORITY:\033[0m\n"
        "\t                                    \033[35m 'fatal'\033[0m\n"
        "\t                                    \033[35m 'error' - used in stable release stage\033[0m\n"
        "\t                                    \033[35m 'warn'\033[0m\n"
        "\t                                    \033[35m 'info'  - used in release stage\033[0m\n"
        "\t                                    \033[35m 'debug' - used only in devel. (default)\033[0m\n"
        "\t                                    \033[35m 'trace' - show all details\033[0m\n"
        "\n"
        "\t-A, --appender=<APPENDER>    \033[35m overwrite appender in log4crc, available APPENDER:\033[0m\n"
        "\t                                    \033[35m 'default' - using appender specified in log4crc\033[0m\n"
        "\t                                    \033[35m 'stdout' - using appender stdout\033[0m\n"
        "\t                                    \033[35m 'stderr' - using appender stderr\033[0m\n"
        "\t                                    \033[35m 'syslog' - using appender syslog\033[0m\n"
        "\n"
        "\t-t, --threads=<THREADS>      \033[35m specify number of threads. %d (default)\033[0m\n"
        "\n"
        "\t-q, --queues=<QUEUES>        \033[35m specify total queues for all threads. %d (default)\033[0m\n"
        "\n"
        "\t-N, --clientid=<CLIENTID>    \033[35m CAUTION: replace clientid in file CLIENTID\033[0m\n"
        "\n"
        "\t-D, --daemon                 \033[35m run as daemon process.\033[0m\n"
        "\t-K, --kill                   \033[35m kill all processes for this program.\033[0m\n"
        "\t-L, --list                   \033[35m list of pids for this program.\033[0m\n"
        "\t-I, --interactive            \033[35m run as interactive mode for testing.\033[0m\n"
        "\n"
        "\t-m, --md5=FILE               \033[35m md5sum on given file.\033[0m\n"
        "\t-r, --regexp=PATTERN         \033[35m use pattern for matching on <express>.\033[0m\n"
        "\n"
        "\033[47;35m* COPYRIGHT (c) 2014-2020 PEPSTACK.COM, ALL RIGHTS RESERVED.\033[0m\n",
        APP_NAME, APP_NAME, APP_VERSION, APP_NAME, XSYNC_CLIENT_THREADS, XSYNC_CLIENT_QUEUES);

#ifdef DEBUG
    printf("\033[31m**** Caution: DEBUG compiling mode only used in develop stage ! ****\033[0m\n");
    assert(0 && "DEBUG compiling mode enabled.");
#endif
}


__no_warning_unused(static)
void xs_appopts_initiate (int argc, char *argv[], xs_appopts_t *opts)
{
    int ret;

    char buff[XSYNC_BUFSIZE];

    char config[XSYNC_PATHFILE_MAXLEN + 1];
    char log4crc[XSYNC_PATHFILE_MAXLEN + 1];

    char priority[20] = { "debug" };
    char appender[60] = { "stdout" };

    int from_watch = 0;

    int interactive = 0;
    int isdaemon = 0;

    int threads = INT_MAX;
    int queues = INT_MAX;

    bzero(opts, sizeof(*opts));

    opts->threads = XSYNC_CLIENT_THREADS;
    opts->queues = XSYNC_CLIENT_QUEUES;

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
        {"clientid", required_argument, 0, 'N'},
        {"daemon", no_argument, 0, 'D'},
        {"kill", no_argument, 0, 'K'},
        {"list", no_argument, 0, 'L'},
        {"interactive", no_argument, 0, 'I'},
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
    while ((ret = getopt_long(argc, argv, "hVC:WO:P:A:t:q:N:DKLIm:r:", lopts, 0)) != EOF) {
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
            from_watch = 1;
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
            threads = atoi(optarg);
            break;

        case 'q':
            queues = atoi(optarg);
            break;

        case 'N':
            /* TODO */
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

        case 'I':
            interactive = 1;
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
        #ifdef NDEBUG
            fprintf(stdout, "\033[1;35m%s, NDEBUG version: %s, build: %s %s\033[0m\n\n", APP_NAME, APP_VERSION, __DATE__, __TIME__);
            assert(0 && "RELEASE VERSION SHOULD NOT ENABLE assert() MARCO !");
        #else
            fprintf(stdout, "\033[1;35m%s, DEBUG version: %s, build: %s %s\033[0m\n\n", APP_NAME, APP_VERSION, __DATE__, __TIME__);
            fprintf(stdout, "\033[31m**** Caution: DEBUG compiling mode only used in develop stage ! ****\033[0m\n");
            assert(0 && "DEBUG compiling mode enabled.");
        #endif
            exit(0);
            break;
        }
    }

    fprintf(stdout, "\033[1;34m* Default log4c path : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[1;34m* Default config file: %s\033[0m\n\n", config);
    fprintf(stdout, "\033[1;32m* Using log4c path   : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[1;32m* Using config file  : %s\033[0m\n\n", config);

    if (interactive) {
        if (isdaemon) {
            isdaemon = 0;
            fprintf(stdout, "\033[1;31m* daemon mode ignored due to specify '-T,--test'.\033[0m\n");
        }

        if (strcmp(appender, "stdout")) {
            fprintf(stdout, "\033[1;31m* appender ('%s') redesignated to 'stdout' due to specify '-T,--test'.\033[0m\n", appender);
            strcpy(appender, "stdout");
        }
    }

    if (threads != INT_MAX) {
        // 用户指定了线程覆盖配置文件, 此时队列也必须覆盖
        if (threads > XSYNC_CLIENT_THREADS_MAX) {
            opts->threads = XSYNC_CLIENT_THREADS_MAX;
        } else if (threads < 1) {
            opts->threads = 1;
        } else {
            opts->threads = threads;
        }

        fprintf(stdout, "\033[1;33m* Overwritten config : threads=%d queues=%d\033[0m\n\n", opts->threads, opts->queues);
    } else if (queues != INT_MAX) {
        // 用户只指定了队列覆盖配置文件
        if (queues > XSYNC_CLIENT_THREADS_MAX * 4) {
            opts->queues = XSYNC_CLIENT_THREADS_MAX * 4;
        } else if (queues < 64) {
            opts->queues = 64;
        } else {
            opts->queues = queues;
        }

        fprintf(stdout, "\033[1;33m* Overwritten config : threads=? queues=%d\033[0m\n\n", opts->queues);
    }

    if (from_watch) {
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
    opts->from_watch = from_watch;
    opts->interactive = interactive;

    memcpy(opts->config, config, XSYNC_PATHFILE_MAXLEN);

    opts->config[XSYNC_PATHFILE_MAXLEN] = 0;
}


__no_warning_unused(static)
void xs_appopts_finalize (xs_appopts_t *opts)
{
    // TODO:
}


#if defined(__cplusplus)
}
#endif

#endif /* CLIENT_H_INCLUDED */
