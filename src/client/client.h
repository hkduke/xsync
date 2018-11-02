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
 * @version: 0.3.9
 *
 * @create: 2018-01-24
 *
 * @update: 2018-11-01 14:44:46
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
#include "../common/readconf.h"

/**
 * print usage for app
 *
 */
__no_warning_unused(static)
void print_usage(void)
{
    #ifdef NDEBUG
        #undef DEBUG

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
        "\t-C, --config=CFGFILE         \033[35m specify config file. '../conf/xclient.cfg' (default)\033[0m\n"
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
        "\t                                     \033[35m +--- watch/          (can also be symlink)\033[0m\n"
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
        "\t                                    \033[35m 'notice'\033[0m\n"
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
        "\t-s, --sweep-interval=<SECONDS>  \033[35m specify sweep interval in seconds. %d (default)\033[0m\n"
        "\n"
        "\t-k, --kafka                  \033[35m logging event to kafka enabled.\033[0m\n"
        "\n"
        "\t-t, --threads=<THREADS>      \033[35m specify number of threads. %d (default)\033[0m\n"
        "\n"
        "\t-q, --queues=<QUEUES>        \033[35m specify total queues for all threads. %d (default)\033[0m\n"
        "\n"
        "\t-N, --clientid=<CLIENTID>    \033[35m CAUTION: replace clientid in file CLIENTID\033[0m\n"
        "\t-p, --password=<PASSWORD>    \033[35m specify password for the clientid\033[0m\n"
        "\n"
        "\t-D, --daemon                 \033[35m run as daemon process.\033[0m\n"
        "\t-K, --kill                   \033[35m kill all processes for this program.\033[0m\n"
        "\t-L, --list                   \033[35m list of pids for this program.\033[0m\n"
        "\t-I, --interactive            \033[35m run as interactive mode for testing.\033[0m\n"
        "\t-S, --save-config            \033[35m save config file specified by '--config' or '--save-config'.\033[0m\n"
        "\n"
        "\t-m, --md5=FILE               \033[35m md5sum on given file.\033[0m\n"
        "\n"
        "\033[47;35m* COPYRIGHT (c) 2014-2020 PEPSTACK.COM, ALL RIGHTS RESERVED.\033[0m\n",
        APP_NAME, APP_VERSION, APP_NAME,
        XSYNC_SWEEP_INTERVAL_SECONDS, XSYNC_CLIENT_THREADS, XSYNC_CLIENT_QUEUES);

#ifdef DEBUG
    printf("\033[31m**** CAUTION: DEBUG Compiling Mode Only Used in Development Stage ! ****\033[0m\n");
    assert(0 && "DEBUG Compiling Mode Enabled.");
#endif
}


__no_warning_unused(static)
void xs_appopts_initiate (int argc, char *argv[], xs_appopts_t *opts)
{
    int ret;

    char buffer[XSYNC_BUFSIZE];

    /* 程序路径长度不能 > 255 */
    char config[FILENAME_MAXLEN + 1];
    char log4crc[FILENAME_MAXLEN + 1];
    char save_config[FILENAME_MAXLEN + 1];

    char priority[20] = { "debug" };
    char appender[60] = { "stdout" };

    int threads = 0;
    int queues = 0;

    bzero(opts, sizeof(*opts));

    opts->threads = XSYNC_CLIENT_THREADS;
    opts->queues = XSYNC_CLIENT_QUEUES;

    opts->sweep_interval = XSYNC_SWEEP_INTERVAL_SECONDS;

    *save_config = 0;

    /* command arguments */
    const struct option lopts[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {"config", required_argument, 0, 'C'},
        {"from-watch", no_argument, 0, 'W'},
        {"log4c-rcpath", required_argument, 0, 'O'},
        {"priority", required_argument, 0, 'P'},
        {"appender", required_argument, 0, 'A'},
        {"sweep-interval", required_argument, 0, 's'},
        {"kafka", optional_argument, 0, 'k'},
        {"threads", required_argument, 0, 't'},
        {"queues", required_argument, 0, 'q'},
        {"clientid", required_argument, 0, 'N'},
        {"password", required_argument, 0, 'p'},
        {"daemon", no_argument, 0, 'D'},
        {"kill", no_argument, 0, 'K'},
        {"list", no_argument, 0, 'L'},
        {"interactive", no_argument, 0, 'I'},
        {"save-config", optional_argument , 0, 'S'},
        {"md5", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    /**
     * get default real path for xsync-client.conf
     */
    ret = realpathdir(argv[0], buffer, sizeof(buffer));
    if (ret <= 0 || ret >= sizeof(opts->apphome)/2) {
        fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buffer);
        exit(-1);
    }

    if (strrchr(buffer, '/') == strchr(buffer, '/')) {
        fprintf(stderr, "\033[1;31m[error]\033[0m cannot run under root path: %s\n", buffer);
        exit(-1);
    }

    opts->apphome_len = ret;
    memcpy(opts->apphome, buffer, opts->apphome_len + 1);

    *strrchr(buffer, '/') = 0;
    *(strrchr(buffer, '/') + 1) = 0;

    ret = snprintf(config, sizeof(config), "%sconf/xclient.cfg", buffer);
    if (ret < 20 || ret >= sizeof(config)) {
        fprintf(stderr, "\033[1;31m[error]\033[0m invalid config path: %s\n", buffer);
        exit(-1);
    }

    ret = snprintf(log4crc, sizeof(log4crc), "LOG4C_RCPATH=%sconf/", buffer);
    if (ret < 20 || ret >= sizeof(log4crc)) {
        fprintf(stderr, "\033[1;31m[error]\033[0m invalid log4c path: %s\n", buffer);
        exit(-1);
    }

    /* parse command arguments */
    while ((ret = getopt_long(argc, argv, "hVC:WO:k::P:A:t:q:s:N:p:DKLS::Im:", lopts, 0)) != EOF) {
        switch (ret) {
        case 'D':
            opts->isdaemon = 1;
            break;

        case 'h':
            print_usage();
            exit(0);
            break;

        case 'C':
            /* overwrite default config file */
            ret = snprintf(config, sizeof(config), "%s", optarg);
            if (ret < 20 || ret >= sizeof(config)) {
                fprintf(stderr, "\033[1;31m[error]\033[0m invalid config file: %s\n", optarg);
                exit(-1);
            }

            if (getfullpath(config, buffer, sizeof(buffer)) != 0) {
                fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buffer);
                exit(-1);
            } else {
                ret = snprintf(config, sizeof(config), "%s", buffer);
                if (ret < 20 || ret >= sizeof(config)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid config file: %s\n", buffer);
                    exit(-1);
                }
            }
            break;

        case 'k':
            if (optarg) {
                fprintf(stderr, "\033[1;31m[warn]\033[0mNOTE: optional argument for --kafka='%s' not implemented. using kafka_config() in event-task.lua instead.\n", optarg);
                exit(1);
            }

            opts->kafka = 1;
            break;

        case 'S':
            if (optarg) {
                /* overwrite default config file */
                ret = snprintf(save_config, sizeof(save_config), "%s", optarg);
                if (ret < 20 || ret >= sizeof(save_config)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid config file: %s\n", optarg);
                    exit(-1);
                }

                if (getfullpath(save_config, buffer, sizeof(buffer)) != 0) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buffer);
                    exit(-1);
                } else {
                    ret = snprintf(save_config, sizeof(save_config), "%s", buffer);
                    if (ret < 20 || ret >= sizeof(save_config)) {
                        fprintf(stderr, "\033[1;31m[error]\033[0m invalid config file: %s\n", buffer);
                        exit(-1);
                    }
                }
            } else {
                save_config[0] = '-';
                save_config[1] = 0;
            }
            break;

        case 'W':
            opts->from_watch = 1;
            break;

        case 'O':
            /* overwrite default log4crc file */
            ret = snprintf(log4crc, sizeof(log4crc), "%s", optarg);
            if (ret < 0 || ret >= sizeof(log4crc)) {
                fprintf(stderr, "\033[1;31m[error]\033[0m specified invalid log4c path: \033[31m%s\033[0m\n", optarg);
                exit(-1);
            }

            if (getfullpath(log4crc, buffer, sizeof(buffer)) != 0) {
                fprintf(stderr, "\033[1;31m[error]\033[0m %s\n", buffer);
                exit(-1);
            } else {
                ret = snprintf(log4crc, sizeof(log4crc), "LOG4C_RCPATH=%s", buffer);
                if (ret < 10 || ret >= sizeof(log4crc)) {
                    fprintf(stderr, "\033[1;31m[error]\033[0m invalid log4c path: %s\n", buffer);
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

        case 's':
            opts->sweep_interval = atoi(optarg);
            break;

        case 'N':
            strncpy(opts->clientid, optarg, XSYNC_CLIENTID_MAXLEN);
            opts->clientid[XSYNC_CLIENTID_MAXLEN] = '\0';
            break;

        case 'p':
            strncpy(opts->password, optarg, XSYNC_PASSWORD_MAXLEN);
            opts->password[XSYNC_PASSWORD_MAXLEN] = '\0';
            break;

        case 'm':
            ret = check_file_mode(optarg, R_OK);

            if (ret) {
                fprintf(stderr, "\033[1;31m[error: md5sum]\033[0m file not found: %s\n\n", optarg);
            } else {
                ret = md5sum_file(optarg, buffer, sizeof(buffer));

                if (ret == 0) {
                    fprintf(stdout, "\033[1;32m[success: md5sum]\033[0m %s (%s)\n", buffer, optarg);
                } else {
                    fprintf(stderr, "\033[1;31m[error: md5sum]\033[0m file: %s\n", optarg);
                }
            }
            exit(ret);
            break;

        case 'I':
            opts->interactive = 1;
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

    fprintf(stdout, "\033[1;34m* default log4c path  : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[1;34m* default config file : %s\033[0m\n\n", config);

    if (opts->interactive) {
        if (opts->isdaemon) {
            opts->isdaemon = 0;
            fprintf(stdout, "\033[1;31m* daemon mode ignored due to specify '-T,--test'.\033[0m\n");
        }

        if (strcmp(appender, "stdout")) {
            fprintf(stdout, "\033[1;31m* appender ('%s') redesignated to 'stdout' due to specify '-T,--test'.\033[0m\n", appender);
            strcpy(appender, "stdout");
        }
    }

    if (threads != 0) {
        // 用户指定了线程数, 覆盖配置文件
        opts->threads = threads;
    }

    if (queues != 0) {
        // 用户只指定了队列数, 覆盖配置文件
        opts->queues = queues;
    }

    if (save_config[0] == '-') {
        memcpy(save_config, config, sizeof(save_config));
    }
    memcpy(opts->save_config, save_config, sizeof(save_config));

    if (opts->from_watch) {
        // 强迫从 watch 目录自动配置
        memcpy(buffer, opts->apphome, opts->apphome_len + 1);

        *strrchr(buffer, '/') = 0;
        *(strrchr(buffer, '/') + 1) = 0;

        strcat(buffer, "watch");

        if (! isdir(buffer)) {
            fprintf(stderr, "\033[1;31m[error] NOT a directory:\033[0m %s\n\n", buffer);
            exit(-1);
        }

        if (0 != access(buffer, F_OK|R_OK|X_OK)) {
            fprintf(stderr, "\033[1;31m[error] watch error(%d): %s.\033[0m (%s)\n\n", errno, strerror(errno), buffer);
            exit(-1);
        }

        snprintf(config, sizeof(config), "%s", buffer);
        fprintf(stdout, "\033[1;36m* force using watch  : %s\033[0m\n", config);
    }

    // 设置log4c: common_util.h
    config_log4crc(APP_NAME, log4crc, priority, appender, sizeof(appender), buffer, sizeof(buffer));

    // 得到启动命令
    ret = getstartcmd(argc, argv, buffer, sizeof(buffer), APP_NAME);
    if (ret) {
        fprintf(stderr, "\033[1;31m[error: getstartcmd]\033[0m %s\n\n", buffer);
        exit(-1);
    }

    // 输出选项
    ret = strlen(buffer) + 1;
    opts->startcmd = (char *) malloc(ret + 1);
    memcpy(opts->startcmd, buffer, ret);
    opts->startcmd[ret] = 0;

    memcpy(opts->config, config, sizeof(config));
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
