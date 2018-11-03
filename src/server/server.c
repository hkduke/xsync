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
 * @file: server.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.4.1
 *
 * @create: 2018-01-29
 *
 * @update: 2018-11-03 11:06:04
 */

#include "server.h"

void run_interactive (xs_appopts_t *opts);

void run_service (xs_appopts_t *opts);


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
 *
 *   $ xsync-server --redis-cluster='127.0.0.1:7001,127.0.0.1:7002,127.0.0.1:7003,127.0.0.1:7004,127.0.0.1:7005,127.0.0.1:7006,127.0.0.1:7007,127.0.0.1:7008,127.0.0.1:7009' --redis-auth='PepSt@ck'
 *
 *   $ xsync-server --redis-cluster='localhost:7001-7009'
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
        run_interactive(&opts);
    } else {
        run_service(&opts);
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

    getinputline(XSSRVAPP CSH_GREEN_MSG("XS_server_create ...\n"), 0, 0);

    ret = XS_server_create(opts, &server);

    if (ret == XS_SUCCESS) {
        getinputline(XSSRVAPP CSH_GREEN_MSG("XS_server_bootstrap ...\n"), 0, 0);

        XS_server_bootstrap(server);

        XS_server_release(&server);
    }
#undef XSSRVAPP
}


void run_service (xs_appopts_t *opts)
{
    XS_server server;

    if (XS_server_create(opts, &server) == XS_SUCCESS) {
        /** 启动客户端服务程序, 永远运行 */
        XS_server_bootstrap(server);

        /** 使用完毕, 释放客户端 */
        XS_server_release(&server);
    }
}
