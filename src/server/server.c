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


void run_server_shell ();


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
    run_server_shell();

    exit(-1);
}


void print_status_msg (int status, const char *msg)
{
    printf("%s", msg);
}


void run_server_shell ()
{
#define XSSRVAPP    csh_green_msg("["APP_NAME"] ")

    int epollfd, listenfd;

    struct epoll_event *events;

    char msg[256];

    char port[6];
    char magic[10];

    const char *result;

    int backlog = SOMAXCONN;
    int maxevents = 1024;
    int timeout_ms = 1000;

    result = getinputline(XSSRVAPP CSH_CYAN_MSG("Input listen port ["XSYNC_PORT_DEFAULT"]: "), msg, sizeof(msg));
    if (result) {
        strncpy(port, result, sizeof(port) - 1);
    } else {
        strncpy(port, XSYNC_PORT_DEFAULT, sizeof(port) - 1);
    }
    port[sizeof(port) - 1] = '\0';

    result = getinputline(XSSRVAPP CSH_CYAN_MSG("Input server magic ["XSYNC_MAGIC_DEFAULT"]: "), msg, sizeof(msg));
    if (result) {
        strncpy(magic, result, sizeof(magic) - 1);
    } else {
        strncpy(magic, XSYNC_MAGIC_DEFAULT, sizeof(magic) - 1);
    }
    magic[sizeof(magic) - 1] = '\0';

    result = getinputline(XSSRVAPP CSH_CYAN_MSG("Is that options correct? [Y for yes | N for no]: "), msg, sizeof(msg));
    if (yes_or_no(result, 0) == 0) {
        getinputline(XSSRVAPP CSH_CYAN_MSG("User cancelded.\n"), 0, 0);
        exit(0);
    }

    snprintf(msg, sizeof(msg), XSSRVAPP CSH_CYAN_MSG("Create server {0.0.0.0:%s#%s} and bind ...\n"), port, magic);
    getinputline(msg, 0, 0);

    listenfd = create_and_bind(NULL, port, msg, sizeof(msg));
    if (listenfd == -1) {
        fprintf(stderr, CSH_RED_MSG("%s.\n"), msg);
        exit(-1);
    }
    getinputline(XSSRVAPP CSH_GREEN_MSG("bind socket ok.\n"), 0, 0);

    int old_sockopt = set_socket_nonblock(listenfd, msg, sizeof(msg));
    if (old_sockopt == -1) {
        close(listenfd);
        fprintf(stderr, CSH_RED_MSG("%s.\n"), msg);
        exit(-1);
    }
    getinputline(XSSRVAPP CSH_GREEN_MSG("set nonblock ok.\n"), 0, 0);

    epollfd = init_epoll_event(listenfd, SOMAXCONN, msg, sizeof(msg));
    if (epollfd == -1) {
        fcntl(listenfd, F_SETFL, old_sockopt);
        close(listenfd);
        fprintf(stderr, CSH_RED_MSG("%s.\n"), msg);
        exit(-1);
    }
    snprintf(msg, sizeof(msg), XSSRVAPP CSH_GREEN_MSG("create epoll ok. (backlog=%d)\n"), backlog);
    getinputline(msg, 0, 0);

    events = (struct epoll_event *) calloc(maxevents, sizeof(struct epoll_event));
    if (! events) {
        fprintf(stderr, CSH_RED_MSG("create events error: out of memory.\n"));
        fcntl(listenfd, F_SETFL, old_sockopt);
        close(listenfd);
        close(epollfd);
        exit(-1);
    }

    snprintf(msg, sizeof(msg), XSSRVAPP CSH_GREEN_MSG("loop epoll events. (events=%d timeout=%d msec)\n"), maxevents, timeout_ms);
    getinputline(msg, 0, 0);

    loop_epoll_events(epollfd, listenfd, events, maxevents, timeout_ms, print_status_msg, msg, sizeof(msg));

    free(events);

    fcntl(listenfd, F_SETFL, old_sockopt);
    close(listenfd);
    close(epollfd);

#undef XSSRVAPP
}
