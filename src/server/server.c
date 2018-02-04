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


int on_timer_event (mul_event_handle eventhdl, void *eventarg)
{
    printf(" ==> on_timer_event_%lld\n", (long long) * eventhdl);

    //mul_wheel_timer_remove_event(eventhdl);

    return 0;
}


/**
 * server main entry
 *
 * Run Commands:
 * 1) Debug:
 *     $ ../target/server-client-0.0.1 -Ptrace -Astdout
 *
 */
int main (int argc, char *argv[])
{
    int err;

    printf("TODO: %s-%s start ...\n", APP_NAME, APP_VERSION);

    err = mul_wheel_timer_init(mwt_timeunit_sec, 1, 10, 0);
    assert(! err);

    // 添加首次20秒激发，以后间隔3秒激发的定时器
    mul_wheel_timer_set_event(20, 3, MUL_WHEEL_TIMER_EVENT_INFINITE, on_timer_event, 0);

    // 添加首次30秒激发，以后间隔5秒激发的定时器
    mul_wheel_timer_set_event(30, 5, MUL_WHEEL_TIMER_EVENT_INFINITE, on_timer_event, 0);

    // 添加首次35秒激发，以后间隔1秒激发的定时器，但是只激发一次
    mul_wheel_timer_set_event(30, 1, MUL_WHEEL_TIMER_EVENT_ONEOFF, on_timer_event, 0);

    // 添加一次性定时器，仅在第10秒激发一次，忽略 MUL_WHEEL_TIMER_EVENT_INFINITE
    mul_wheel_timer_set_event(10, 0, MUL_WHEEL_TIMER_EVENT_INFINITE, on_timer_event, 0);

    mul_wheel_timer_start();

    while (1)
    {
        pause();
    }

    err = mul_wheel_timer_destroy();
    assert(! err);

    printf("TODO: %s-%s end.\n", APP_NAME, APP_VERSION);
    return (XS_ERROR);
}
