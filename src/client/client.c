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
 * @file: client.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.2
 *
 * @create: 2018-01-24
 *
 * @update: 2018-08-29 17:33:10
 */

#include "client.h"
#include "server_conn.h"

void run_interactive (int threads);

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
        run_interactive(1);
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


void run_forever (xs_appopts_t *opts)
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

/*
static void * thread_func_ok (void *arg)
{
    int64_t id = 1;
    int c = 0;

    char msg[256];

    int tid = pthread_self();

    printf("thread#%lld start ...\n", (long long) tid);

    do {
        int sfd;
        size_t len;

        xs_server_opts *serv = (xs_server_opts *) arg;

        printf("thread#%d start connect ...\n", tid);

        sfd = opensocket_v2(serv->host, serv->sport, serv->sockopts.timeosec, &serv->sockopts, msg);

        if (sfd == -1) {
            LOGGER_ERROR("opensocket_v2 failed: %s", msg);
        } else {
            LOGGER_INFO("opensocket_v2 success: %s:%s", serv->host, serv->sport);
        }

        // 休息 6 秒
        sleep(6);

        printf("session start...\n");

        int total = 0;

        for (;;) {
            len = snprintf(msg, sizeof(msg), "**** client thread#%lld say hello: %lld ****", (long long) tid, (long long) id++);
            msg[len] = 0;

            printf("sendind: %s\n", msg);

            int err = sendlen(sfd, msg, len + 1);
            if (err != len + 1) {
                printf("**** sendlen error(%d): %s\n", errno, strerror(errno));
                break;
            }

            total += (len + 1);
            printf("%d bytes sent\n", total);

            ////////////// test ///////////////////
            // 发送完毕，休息
            //close(sfd);

            //sleep(20);

            ///////////////////////////////////////
            //if (write(sfd, msg, len + 1) != len + 1) {
            //    perror("write");
            //    close(sfd);
            //    break;
            //} else {
            //    printf("%s\n", msg);
            //    total += (len + 1);
            //}

            if (total > 1073741824) {
                // 1 GB
                printf("total %d bytes send.\n", total);
                break;
            }
        }
        printf("session end.\n");

        // 断开连接
        //
        close(sfd);
    } while(c++ < 10);

    printf("thread#%d end.\n", tid);
    return ((void*) 0);
}
*/

static void * thread_func (void *arg)
{
    int tid = pthread_self();

    XS_RESULT xres;

    char clientid[] = "xsync-test";

    XS_server_conn xcon = 0;

    printf("thread#%lld start ...\n", (long long) tid);

    do {
        char msg[256];
        int len, total = 0;

        xres = XS_server_conn_create((xs_server_opts *) arg, clientid, &xcon);

        if (xres != XS_SUCCESS) {
            printf("failed to connect server.\n");
            break;
        }

        sleep(60);

        XS_server_conn_release(&xcon);
    } while(0);

    printf("thread#%d end.\n", tid);
    return ((void*) 0);
}


void run_interactive (int threads)
{
#define XSCLIAPP    csh_green_msg("["APP_NAME"] ")

    xs_server_opts  srvopts;
    bzero(&srvopts, sizeof(srvopts));

    strcpy(srvopts.host, "localhost");
    strcpy(srvopts.sport, XSYNC_PORT_DEFAULT);

    LOGGER_INFO("interactive client [%s] start ...", APP_NAME);

    char msg[256];

    const char *result;

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input server ip [localhost]: "), msg, sizeof(msg));
    if (result) {
        strncpy(srvopts.host, result, sizeof(srvopts.host) - 1);
    }
    srvopts.host[sizeof(srvopts.host) - 1] = '\0';

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input server port ["XSYNC_PORT_DEFAULT"]: "), msg, sizeof(msg));
    if (result) {
        strncpy(srvopts.sport, result, sizeof(srvopts.sport) - 1);
    }
    srvopts.sport[sizeof(srvopts.sport) - 1] = '\0';
    srvopts.port = atoi(srvopts.sport);

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input server magic ["XSYNC_MAGIC_DEFAULT"]: "), msg, sizeof(msg));
    if (result) {
        srvopts.magic = atoi(msg);
    } else {
        srvopts.magic = atoi(XSYNC_MAGIC_DEFAULT);
    }

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Input client threads [1]: "), msg, sizeof(msg));
    if (result) {
        threads = atoi(msg);
    }

    result = getinputline(XSCLIAPP CSH_CYAN_MSG("Is that options correct? [Y for yes | N for no]: "), msg, sizeof(msg));
    if (yes_or_no(result, 0) == 0) {
        getinputline(XSCLIAPP CSH_CYAN_MSG("User cancelded.\n"), 0, 0);
        exit(0);
    }

    snprintf(msg, sizeof(msg), XSCLIAPP CSH_CYAN_MSG("Connect to server {%s:%d#%d} ...\n"), srvopts.host, srvopts.port, srvopts.magic);
    getinputline(msg, 0, 0);

    do {
        pthread_t * pthreads = (pthread_t *) calloc(threads, sizeof(pthread_t));

        int i, err;
        void *ret;

        for (i = 0; i < threads; i++) {
            err = pthread_create (&pthreads[i], 0, thread_func, (void*) &srvopts);
            assert(err == 0);
        }

        for (i = 0; i < threads; i++) {
            err = pthread_join (pthreads[i], (void**) &ret);
            assert(err == 0);
        }

        free(pthreads);
    } while(0);

    LOGGER_INFO("interactive client [%s] exit.", APP_NAME);

#undef XSCLIAPP
}
