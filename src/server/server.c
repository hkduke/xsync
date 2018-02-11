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

#include "server.h"

void run_interactive (xs_appopts_t *opts);

void run_forever (xs_appopts_t *opts);


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
    //TODO:
}


/***********************************************************************
 * epoll refer:
 *
 *   1) http://blog.csdn.net/liuxuejiang158blog/article/details/12422471
 *   2) http://www.cnblogs.com/haippy/archive/2012/01/09/2317269.html
 *
 *
 * Run Commands:
 * 1) Debug:
 *     $ ../target/server-client-0.0.1 -Ptrace -Astdout
 *
 **********************************************************************/
int main (int argc, char *argv[])
{
    xs_appopts_t opts;

    xs_appopts_initiate(argc, argv, &opts);

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

    if (opts.interactive) {
        opts.threads = appopts_validate_threads(opts.threads);
        opts.queues = appopts_validate_queues(opts.threads, opts.queues);

        LOGGER_INFO("threads=%d queues=%d", opts.threads, opts.queues);

        run_interactive(&opts);

        xs_appopts_finalize(&opts);

        LOGGER_FATAL("%s (v%s) shutdown !", APP_NAME, APP_VERSION);
        LOGGER_FINI();

        exit(0);
    } else {
        run_forever(&opts);
    }

    /** 客户端异常退出 */
onerror_exit:

    xs_appopts_finalize(&opts);

    LOGGER_FATAL("%s (v%s) shutdown !", APP_NAME, APP_VERSION);
    LOGGER_FINI();

    return (XS_ERROR);
}


void run_interactive (xs_appopts_t *opts)
{
#define XSSRVAPP    csh_green_msg("["APP_NAME"] ")
    XS_RESULT ret;
    XS_server server;

    opts->timeout_ms = 1000;
    opts->maxclients = 1024;
    opts->threads = 16;
    opts->queues = 256;

    strcpy(opts->port, "8960");

    opts->somaxconn = 128;
    opts->maxevents = 1024;

    getinputline(XSSRVAPP CSH_GREEN_MSG("XS_server_create ...\n"), 0, 0);

    ret = XS_server_create(opts, &server);

    if (ret == XS_SUCCESS) {
        getinputline(XSSRVAPP CSH_GREEN_MSG("XS_server_bootstrap ...\n"), 0, 0);

        XS_server_bootstrap(server);

        XS_server_release(&server);
    }
#undef XSSRVAPP
}


void run_forever (xs_appopts_t *opts)
{
    XS_server server;

    if (XS_server_create(opts, &server) == XS_SUCCESS) {

#ifdef DEBUG
        //TODO:XS_server_conf_save_xml(server, "/tmp/xsync-server-conf_DEBUG.xml");
        //TODO:XS_server_conf_save_ini(server, "/tmp/xsync-server-conf_DEBUG.ini");
#endif

        /** 启动客户端服务程序, 永远运行 */
        XS_server_bootstrap(server);

        /** 使用完毕, 释放客户端 */
        XS_server_release(&server);
    }
}
