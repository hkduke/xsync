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
 * client.c
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-24
 * update: 2018-02-08
 *
 */

#include "client.h"


void run_client_forever (clientapp_opts *opts);

/**
 * ${app} --shell
 */
void run_client_shell (clientapp_opts *opts);


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
void exit_handler (int exitCode, void *ppData)
{
    char *startcmd = *((char **) ppData);

    //sem_unlink(semaphore);
    printf("\n* %s-%s exit(%d).\n", APP_NAME, APP_VERSION, exitCode);

    if (startcmd) {
        *((char **) ppData) = 0;

        if (exitCode == 10 || exitCode == 11) {
            //time_t t = time(0);
            //printf("\n\n* %s* isynclog-client restart: %s\n\n", ctime(&t), (char*) startcmd);
            //code = pox_system(startcmd);
        }

        free(startcmd);
    }
}


/***********************************************************************
 * client application
 *
 * Run Commands:
 * 1) Debug:
 *     $ ../target/xsync-client-0.0.1 -Ptrace -Astdout
 *
 **********************************************************************/
int main (int argc, char *argv[])
{
    clientapp_opts opts;

    clientapp_opts_initiate(argc, argv, &opts);

    void sig_chld(int);
    void sig_int(int);

    LOGGER_INIT();
    LOGGER_INFO("%s-%s startup %s", APP_NAME, APP_VERSION, (opts.isdaemon? "as daemon..." : "..."));
    LOGGER_INFO("startcmd={%s}", opts.startcmd);

    /**
     * 注册退出函数
     */
    on_exit(exit_handler, (void *)((char**) &opts.startcmd));

    if (opts.isdaemon) {
        /**
         * int daemon(int nochdir, int noclose);
         *
         * see also:
         *   http://man7.org/linux/man-pages/man3/daemon.3.html
         */
        if ( daemon(1, 1) ) {
            LOGGER_ERROR("daemon error(%d): %s", errno, strerror(errno));
            goto onerror_exit;
        } else {
            LOGGER_INFO("daemon ok. pid=%d", getpid());
        }
    }

    /**
     * 注册信号处理函数
     */
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

    if (opts.isshell) {
        opts.threads = clientapp_validate_threads(opts.threads);
        opts.queues = clientapp_validate_queues(opts.threads, opts.queues);

        LOGGER_INFO("threads=%d queues=%d", opts.threads, opts.queues);

        run_client_shell(&opts);

        clientapp_opts_cleanup(&opts);

        LOGGER_FATAL("%s (v%s) shutdown !", APP_NAME, APP_VERSION);
        LOGGER_FINI();

        exit(0);
    } else {
        run_client_forever(&opts);
    }

    /** 客户端异常退出 */
onerror_exit:

    clientapp_opts_cleanup(&opts);

    LOGGER_FATAL("%s (v%s) shutdown !", APP_NAME, APP_VERSION);
    LOGGER_FINI();

    return (XS_ERROR);
}


void run_client_forever (clientapp_opts *opts)
{
    XS_client client;

    if (XS_client_create(opts, &client) == XS_SUCCESS) {

#ifdef DEBUG
        XS_client_conf_save_xml(client, "/tmp/xsync-client-conf_DEBUG.xml");
        XS_client_conf_save_ini(client, "/tmp/xsync-client-conf_DEBUG.ini");
#endif

        /** 启动客户端服务程序, 永远运行 */
        XS_client_bootstrap(client);

        /** 使用完毕, 释放客户端 */
        XS_client_release(&client);
    }
}


void run_client_shell (clientapp_opts *opts)
{
#define XSCLIAPP    csh_green_msg("["APP_NAME"] ")

    LOGGER_INFO("interactive shell [%s] start ...", APP_NAME);

    char host[XSYNC_HOSTNAME_MAXLEN + 1];
    char port[6];
    char magic[10];

    char msg[256];

    const char *result;

    int portno = 8960;

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input server ip [localhost]: "), msg, sizeof(msg));
    if (result) {
        strncpy(host, result, sizeof(host) - 1);
    } else {
        strncpy(host, "localhost", strlen("localhost") + 1);
    }
    host[sizeof(host) - 1] = '\0';

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input server port ["XSYNC_PORT_DEFAULT"]: "), msg, sizeof(msg));
    if (result) {
        strncpy(port, result, sizeof(port) - 1);
    } else {
        strncpy(port, XSYNC_PORT_DEFAULT, sizeof(port) - 1);
    }
    port[sizeof(port) - 1] = '\0';
    portno = atoi(port);

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input server magic ["XSYNC_MAGIC_DEFAULT"]: "), msg, sizeof(msg));
    if (result) {
        strncpy(magic, result, sizeof(magic) - 1);
    } else {
        strncpy(magic, XSYNC_MAGIC_DEFAULT, sizeof(magic) - 1);
    }
    magic[sizeof(magic) - 1] = '\0';

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Is that options correct? [Y for yes | N for no]: "), msg, sizeof(msg));
    if (yes_or_no(result, 0) == 0) {
        getinputline(XSCLIAPP CSH_CYAN_MSG("User cancelded.\n"), 0, 0);
        exit(0);
    }

    snprintf(msg, sizeof(msg), XSCLIAPP CSH_CYAN_MSG("Connect to server {%s:%d#%s} ...\n"), host, portno, magic);
    getinputline(msg, 0, 0);

    ////////////////////////////////////////////////////////////////////
    struct addrinfo hints;
    struct addrinfo *res, *rp;
    int sfd, s, id;
    size_t len;

    id = 1;
    do {
        /* Obtain address(es) matching host/port */
        memset(&hints, 0, sizeof(struct addrinfo));

        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
        hints.ai_flags = 0;
        hints.ai_protocol = 0;          /* Any protocol */

        s = getaddrinfo(host, port, &hints, &res);

        if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
        }

        /**
         * getaddrinfo() returns a list of address structures.
         *
         * Try each address until we successfully connect(2).
         * If socket(2) (or connect(2)) fails, we (close the socket and)
         *   try the next address.
         **/

        for (rp = res; rp != NULL; rp = rp->ai_next) {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            if (sfd == -1) {
                continue;
            }

            if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                printf("socket ok.\n");
                break;
            }

            close(sfd);
        }

        if (rp == NULL) {
            /* No address succeeded */
            fprintf(stderr, "Could not connect\n");
            exit(-1);
        }

        /* No longer needed */
        freeaddrinfo(res);

        len = snprintf(msg, sizeof(msg), "*************** hello: %d ****************\n", id);
        msg[len] = 0;

        if (write(sfd, msg, len + 1) != len + 1) {
            perror("write");
            close(sfd);
            exit(-1);
        } else {
            printf("send msg:%s\n", msg);
        }

        close(sfd);
    } while(id++ < 1000000000);

    LOGGER_INFO("interactive shell [%s] exit.", APP_NAME);

#undef XSCLIAPP
}
