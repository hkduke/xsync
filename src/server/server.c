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

#include "../common/mul_timer.h"

/**
 * https://linux.die.net/man/4/epoll
 *
 */
#include <sys/epoll.h>


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


struct fds
{
    //文件描述符结构体，用作传递给子线程的参数
    int epollfd;
    int sockfd;
};


int setnonblocking (int fd)
{
    //设置文件描述符为非阻塞
    int old_option = fcntl (fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;

    fcntl(fd, F_SETFL, new_option);

    return old_option;
}


void addfd (int epollfd, int fd, bool oneshot)
{
    //为文件描述符添加事件
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;

    if (oneshot) {
        // 采用EPOLLONETSHOT事件
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

    setnonblocking(fd);
}


void reset_oneshot (int epollfd, int fd)
{
    //重置事件
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    epoll_ctl (epollfd, EPOLL_CTL_MOD, fd, &event);
}

#define BUFFER_SIZE      1024
#define MAX_EVENT_NUMBER 2048


void * worker(void* arg)
{
    //工作者线程(子线程)接收socket上的数据并重置事件
    int sockfd = ((struct fds *) arg)->sockfd;

    //事件表描述符从arg参数(结构体fds)得来
    int epollfd = ((struct fds*) arg)->epollfd;

    free(arg);

    printf("start new thread to receive data on fd: %d\n", sockfd);

    char buf[BUFFER_SIZE];

    memset(buf, 0, BUFFER_SIZE);

    while (1) {
        //接收数据
        int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);

        if (ret == 0) {
            //关闭连接
            close(sockfd);
            printf("close sock: %d\n", sockfd);
            break;
        } else if (ret < 0) {
            if (errno == EAGAIN) {
                //并非网络出错，而是可以再次注册事件
                reset_oneshot(epollfd, sockfd);
                printf("reset epollfd\n");
                break;
            }
        } else {
            printf("buf: %s\n", buf);

            //采用睡眠是为了在5s内若有新数据到来则该线程继续处理，否则线程退出
            sleep(5);
        }
    }

    printf("thread exit on fd: %d\n", sockfd);

    return 0;
}


/**
 * epoll refer:
 *
 *   1) http://blog.csdn.net/liuxuejiang158blog/article/details/12422471
 *   2) http://www.cnblogs.com/haippy/archive/2012/01/09/2317269.html
 *
 * Run Commands:
 * 1) Debug:
 *     $ ../target/server-client-0.0.1 -Ptrace -Astdout
 *
 */
int main (int argc, char *argv[])
{
    const char* ip = "127.0.0.1";

    int port = 8960;

    int ret = 0;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));

    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);

    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create1(EPOLL_CLOEXEC);

    assert(epollfd != -1);

    //不能将监听端口listenfd设置为EPOLLONESHOT否则会丢失客户连接
    addfd(epollfd, listenfd, false);

    while (1) {
        //等待事件发生: 1000 = 1 秒
        //  epoll_pwait: 2.6.19 and later
        int ret = epoll_pwait(epollfd, events, MAX_EVENT_NUMBER, 1000, 0);

        if (ret <= 0) {
            if (ret == 0 || errno == EINTR) {
                //监听被打断
                printf("epoll_pwait: %s\n", strerror(errno));
                continue;
            }

            printf("epoll error\n");
            break;
        } else {
            printf("epoll_pwait ok: %d\n", ret);
        }

        if(ret<=0){
            if(ret==0||errno==EINTR)//监听被打断
                continue;
            return -1;
        }

        for (int i = 0; i < ret; i++) {
            int sockfd = events[i].data.fd;

            if (sockfd == listenfd) {
                //监听端口
                struct sockaddr_in client_address;

                socklen_t addrlen = sizeof(client_address);

                int connfd = accept(listenfd, (struct sockaddr*) &client_address, &addrlen);

                //新的客户连接置为EPOLLONESHOT事件
                addfd(epollfd, connfd, true);
            } else if (events[i].events&EPOLLIN) {
                //客户端有数据发送的事件发生
                pthread_t thread;
                struct fds *new_fds = malloc(sizeof(*new_fds));
                new_fds->epollfd = epollfd;
                new_fds->sockfd = sockfd;

                //调用工作者线程处理数据
                pthread_create(&thread, NULL, worker, (void*) new_fds);
            } else {
                printf("something wrong.\n");
            }
        }
    }

    close(listenfd);
    printf("server exit.\n");

    return (-1);
}
