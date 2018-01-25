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

#include "client.h"


void sig_chld (int signo)
{
    pid_t    pid;
    int      stat;

    /* must call waitpid() */
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        /* UNUSED:
         * printf ("[%d] child process terminated", pid);
         */
    }
    return;
}


/**
 * kill -15 pid
 */
void sig_term (int signo)
{
    void print_cpu_time(void);
    print_cpu_time();
    exit(signo);
}


/**
 * kill -2 pid
 */
void sig_int (int signo)
{
    void print_cpu_time(void);
    print_cpu_time();
    exit(signo);
}


/**
 * 程序退出时调用: exit_handler
 */
void exit_handler (int exitcode, void * startcmd)
{
    //sem_unlink(semaphore);
    printf("\n* %s-%s exit(%d).\n", APP_NAME, APP_VERSION, exitcode);

    if (startcmd) {
        if (exitcode == 10 || exitcode == 11) {
            //time_t t = time(0);
            //printf("\n\n* %s* isynclog-client restart: %s\n\n", ctime(&t), (char*) startcmd);
            //code = pox_system(startcmd);
        }

        free(startcmd);
    }
}


/**
 * client main entry
 *
 * Run Commands:
 * 1) Debug:
 *     $ ../target/xsync-client-0.0.1 -Ptrace -Astdout
 *
 */
int main (int argc, char * argv [])
{
    int ret;

    char buff[XSYNC_IO_BUFSIZE];

    char config[XSYNC_PATHFILE_MAXLEN + 1];
    char log4crc[XSYNC_PATHFILE_MAXLEN + 1];

    char priority[20] = {0};
    char appender[60] = {0};

    int force_watch = 0;

    int isdaemon = 0;

    __attribute__((unused)) int verbose = 0;

    /* command arguments */
    const struct option lopts[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {"verbose", no_argument, 0, 'v'},
        {"config", required_argument, 0, 'C'},
        {"force-watch", no_argument, 0, 'W'},
        {"log4c-rcpath", required_argument, 0, 'O'},
        {"priority", required_argument, 0, 'P'},
        {"appender", required_argument, 0, 'A'},
        {"daemon", no_argument, 0, 'D'},
        {"kill", no_argument, 0, 'K'},
        {"list", no_argument, 0, 'L'},
        {"md5", required_argument, 0, 'm'},
        {"regexp", required_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    void sig_chld(int);
    void sig_int(int);

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

    ret = snprintf(config, sizeof(config), "%sconf/%s.conf", buff, APP_NAME);
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
    while ((ret = getopt_long(argc, argv, "DKLhVvC:WO:P:A:m:r:", lopts, 0)) != EOF) {
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

        case 'K':
            exit(0);
            break;

        case 'L':
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

        case 'r':
            exit(0);
            break;

        case 'V':
            fprintf(stdout, "\033[1;35m%s, Version: %s, Build: %s %s\033[0m\n\n", APP_NAME, APP_VERSION, __DATE__, __TIME__);
            exit(0);
            break;

        case 'v':
            verbose = 1;
            break;
        }
    }

    fprintf(stdout, "\033[1;34m* Default conf file  : %s\033[0m\n", config);
    fprintf(stdout, "\033[1;34m* Default log4c path : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));
    fprintf(stdout, "\033[1;32m* Using conf file    : %s\033[0m\n", config);
    fprintf(stdout, "\033[1;32m* Using log4c path   : %s\033[0m\n", log4crc + sizeof("LOG4C_RCPATH"));

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

    LOGGER_INIT();
    LOGGER_INFO("%s-%s startup %s", APP_NAME, APP_VERSION, (isdaemon? "as daemon..." : "..."));
    LOGGER_INFO("startcmd={%s}", buff);

    if (isdaemon) {
        /**
         * int daemon(int nochdir, int noclose);
         *
         * see also:
         *   http://man7.org/linux/man-pages/man3/daemon.3.html
         */
        ret = daemon(1, 1);

        if (ret) {
            LOGGER_ERROR("daemon error(%d): %s", errno, strerror(errno));
            goto exit_onerror;
        } else {
            LOGGER_INFO("daemon ok. pid=%d", getpid());
        }
    }

    do {
        char * startcmd;
        ret = strlen(buff);

        startcmd = (char *) malloc(ret + 1);
        memcpy(startcmd, buff, ret);
        startcmd[ret] = 0;

        /* 注册退出函数 */
        on_exit(exit_handler, startcmd);

        /* SIGCHLD */
        if (signal(SIGCHLD, sig_chld) == SIG_ERR) {
            LOGGER_FATAL("signal(SIGCHLD) error(%d): %s", errno, strerror(errno));
            exit(-1);
        }

        if (signal(SIGINT, sig_int) == SIG_ERR) {
            LOGGER_FATAL("signal(SIGINT) error(%d): %s", errno, strerror(errno));
            exit(-1);
        }

        if (signal(SIGTERM, sig_term) == SIG_ERR) {
            LOGGER_FATAL("signal(SIGTERM) error(%d): %s", errno, strerror(errno));
            exit(-1);
        }

        /**
         * signal(SIGPIPE, SIG_IGN);
         *
         * Unix supports the principle of piping, which allows processes to send data
         * to other processes without the need for creating temporary files. When a
         * pipe is broken, the process writing to it is sent the SIGPIPE signal.
         * The default reaction to this signal for a process is to terminate.
         * see also:
         *   - http://blog.csdn.net/sukhoi27smk/article/details/43760605
         */
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
            LOGGER_FATAL("signal(SIGPIPE) error(%d): %s", errno, strerror(errno));
            exit(-1);
        }
    } while (0);

    // 启动客户端服务程序, 永远运行
    do {
        XS_client client = 0;

        ret = XS_client_create(config, force_watch, &client);

        if (ret == 0) {

            XS_client_listening_events(client);

            XS_client_release(&client);
        }
    } while(0);

exit_onerror:

    LOGGER_FATAL("%s (v%s) shutdown !", APP_NAME, APP_VERSION);
    LOGGER_FINI();

    return (ret);
}
