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
 * @file: server.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.8
 *
 * @create: 2018-01-29
 *
 * @update: 2018-10-22 10:56:41
 */

#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED


#if defined(__cplusplus)
extern "C" {
#endif

#define APP_NAME              XSYNC_SERVER_APPNAME
#define APP_VERSION           XSYNC_SERVER_VERSION

#include "server_api.h"

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
    printf("\t\033[35m xsync server (server of extremely synchronizing files).\033[0m\n");
    printf("\033[35mOptions:\033[0m\n"
        "\t-h, --help                   \033[35m display help messages\033[0m\n"
        "\t-V, --version                \033[35m print version information\033[0m\n"
        "\n"
        "\t-C, --config=PATHFILE        \033[35m specify path file to conf. '../conf/%s.conf' (default)\033[0m\n"
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
        "\t-i, --server-id=<ID>         \033[35m specify an unique numberic identifier for server. '1' (default)\033[0m\n"
        "\n"
        "\t-n, --magic=<NUMBER>         \033[35m specify magic number for server. '%s' (default)\033[0m\n"
        "\n"
        "\t-s, --host=<SERVER>          \033[35m specify server ip or hostname to bind. '0.0.0.0' (default)\033[0m\n"
        "\n"
        "\t-p, --port=<PORT>            \033[35m specify port to listen. 8960 (default)\033[0m\n"
        "\n"
        "\t-t, --threads=<THREADS>      \033[35m specify number of threads. %d (default)\033[0m\n"
        "\n"
        "\t-q, --queues=<QUEUES>        \033[35m specify total queues for all threads. %d (default)\033[0m\n"
        "\n"
        "\t-e, --events=<EVENTS>        \033[35m specify maximum number of events for epoll. %d (default)\033[0m\n"
        "\n"
        "\t-m, --somaxconn=<BACKLOG>    \033[35m A kernel parameter provides an upper limit on the value of the\033[0m\n"
        "\t                               \033[35m backlog parameter passed to the listen function. %d (default)\033[0m\n"
        "\n"
        "\t-r, --redis-cluster=<REDIS>  \033[35m redis cluster list of host:port. for example:\033[0m\n"
        "\t                                    \033[35m'127.0.0.1:7001,127.0.0.1:7002,...'\033[0m\n"
        "\n"
        "\t-a, --redis-auth=<PASSWORD>  \033[35m redis cluster password if required.\033[0m\n"
        "\n"
        "\t-D, --daemon                 \033[35m run as daemon process.\033[0m\n"
        "\t-K, --kill                   \033[35m kill all processes for this program.\033[0m\n"
        "\t-L, --list                   \033[35m list of pids for this program.\033[0m\n"
        "\t-I, --interactive            \033[35m run server as interactive mode.\033[0m\n"
        "\n"
        "\033[47;35m* COPYRIGHT (c) 2014-2020 PEPSTACK.COM, ALL RIGHTS RESERVED.\033[0m\n",
        APP_NAME,
        XSYNC_MAGIC_DEFAULT,
        XSYNC_SERVER_THREADS,
        XSYNC_SERVER_QUEUES,
        XSYNC_SERVER_EVENTS,
        XSYNC_SERVER_SOMAXCONN);

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

    int interactive = 0;
    int isdaemon = 0;

    int threads = INT_MAX;
    int queues = INT_MAX;
    int somaxconn = INT_MAX;
    int events = INT_MAX;

    bzero(opts, sizeof(*opts));

    opts->threads = XSYNC_SERVER_THREADS;
    opts->queues = XSYNC_SERVER_QUEUES;
    opts->somaxconn = XSYNC_SERVER_SOMAXCONN;
    opts->maxevents = XSYNC_SERVER_EVENTS;
    opts->timeout_ms = 1000;

    // 默认参数
    strcpy(opts->host, "0.0.0.0");
    strcpy(opts->port, "8960");

    opts->serverid[0] = '1';
    opts->serverid[1] = 0;

    opts->magic = (ub4) atoi(XSYNC_MAGIC_DEFAULT);

    do {
        /**
         * get default real path for xsync-server.conf
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
    } while(0);

    do {
        /**
         * parse command arguments. usning: getopt_long_only
         *    https://blog.csdn.net/pengrui18/article/details/8078813
         */
        int ch, index;

        const struct option lopts[] = {
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'V'},
            {"config", required_argument, 0, 'C'},
            {"log4c-rcpath", required_argument, 0, 'O'},
            {"priority", required_argument, 0, 'P'},
            {"appender", required_argument, 0, 'A'},
            {"server-id", required_argument, 0, 'i'},
            {"magic", required_argument, 0, 'n'},
            {"appender", required_argument, 0, 'A'},
            {"host", required_argument, 0, 's'},
            {"port", required_argument, 0, 'p'},
            {"threads", required_argument, 0, 't'},
            {"queues", required_argument, 0, 'q'},
            {"events", required_argument, 0, 'e'},
            {"somaxconn", required_argument, 0, 'm'},
            {"redis-cluster", required_argument, 0, 'r'},
            {"redis-auth", required_argument, 0, 'a'},
            {"daemon", no_argument, 0, 'D'},
            {"kill", no_argument, 0, 'K'},
            {"list", no_argument, 0, 'L'},
            {"interactive", no_argument, 0, 'I'},
            {0, 0, 0, 0}
        };

        while ((ch = getopt_long_only(argc, argv, "DhIKLVC:O:P:A:s:p:t:q:e:m:r:a:", lopts, &index)) != -1) {
            switch (ch) {
            case '?':
                fprintf(stderr, "\033[1;31m[error]\033[0m option not defined.\n");
                exit(-1);

            case 0:
                // flag 不为 0
                switch (index) {
                case 1:
                case 2:
                default:
                    break;
                }
                break;

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
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid priority: %s\n", optarg);
                    exit(-1);
                }
                break;

            case 'A':
                ret = snprintf(appender, sizeof(appender), "%s", optarg);
                if (ret < 0 || ret >= sizeof(appender)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid appender: \033[31m%s\033[0m\n", optarg);
                    exit(-1);
                }
                break;

            case 'i':
                do {
                    int server_id = atoi(optarg);

                    if (server_id < XS_SERVERID_MINID || server_id > XS_SERVERID_MAXID) {
                        fprintf(stderr, "\033[1;31m[error]\033[0m invalid server-id: \033[31m%s\033[0m\n", optarg);
                        exit(-1);
                    }

                    snprintf(opts->serverid, sizeof(opts->serverid), "%d", server_id);
                } while (0);
                break;

            case 'n':
                opts->magic = (ub4) atoi(optarg);
                break;

            case 's':
                ret = snprintf(opts->host, sizeof(opts->host), "%s", optarg);
                if (ret < 0 || ret >= sizeof(opts->host)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid host: \033[31m%s\033[0m\n", optarg);
                    exit(-1);
                }
                break;

            case 'p':
                ret = snprintf(opts->port, sizeof(opts->port), "%s", optarg);
                if (ret < 0 || ret >= sizeof(opts->port)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid port: \033[31m%s\033[0m\n", optarg);
                    exit(-1);
                }
                break;

            case 't':
                threads = atoi(optarg);
                break;

            case 'q':
                queues = atoi(optarg);
                break;

            case 'e':
                events = atoi(optarg);
                break;

            case 'm':
                somaxconn = atoi(optarg);
                break;

            case 'r':
                ret = snprintf(opts->redis_cluster, sizeof(opts->redis_cluster), "%s", optarg);
                if (ret < 0 || ret >= sizeof(opts->redis_cluster)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid redis cluster: \033[31m%s\033[0m\n", optarg);
                    exit(-1);
                }
                break;

            case 'a':
                ret = snprintf(opts->redis_auth, sizeof(opts->redis_auth), "%s", optarg);
                if (ret < 0 || ret >= sizeof(opts->redis_auth)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid redis auth: \033[31m%s\033[0m\n", optarg);
                    exit(-1);
                }
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
    } while(0);

    fprintf(stdout, "\033[1;34m* Server id          : %s\033[0m\n", opts->serverid);
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
        if (threads > 1024) {
            opts->threads = 1024;
        } else if (threads < 8) {
            opts->threads = 8;
        } else {
            opts->threads = threads;
        }

        fprintf(stdout, "\033[1;33m* Overwritten config : threads=%d queues=%d\033[0m\n\n", opts->threads, opts->queues);
    }

    if (queues != INT_MAX) {
        // 用户只指定了队列覆盖配置文件
        if (queues > 4096) {
            opts->queues = 4096;
        } else if (queues < 256) {
            opts->queues = 256;
        } else {
            opts->queues = queues;
        }

        fprintf(stdout, "\033[1;33m* Overwritten config : threads=? queues=%d\033[0m\n\n", opts->queues);
    }

    if (events != INT_MAX) {
        if (events > 4096) {
            opts->maxevents = 4096;
        } else if (opts->maxevents < 256) {
            opts->maxevents = 256;
        } else {
            opts->maxevents = events;
        }
    }

    if (somaxconn != INT_MAX) {
        if (somaxconn > 4096) {
            opts->somaxconn = 4096;
        } else if (opts->somaxconn < 256) {
            opts->somaxconn = 256;
        } else {
            opts->somaxconn = somaxconn;
        }
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

#endif /* SERVER_H_INCLUDED */
