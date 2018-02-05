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


int on_timer_event (mul_event_hdl eventhdl, void *eventarg, void *timerarg)
{
    mul_event_t *event = mul_handle_cast_event(eventhdl);
    printf("    => event_%lld: on_counter=%lld hash=%d\n",
        (long long) event->eventid, (long long) event->on_counter, event->hash);

    //mul_timer_remove_event(eventhdl);

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

    err = mul_timer_init(mul_timeunit_sec, 1, 10, sigalarm_handler, 0, 0);
    assert(! err);

    /*
    // 添加首次20秒激发，以后间隔3秒激发的定时器
    mul_timer_set_event(20, 3, MULTIMER_EVENT_INFINITE, on_timer_event, 0);

    // 添加首次30秒激发，以后间隔5秒激发的定时器
    mul_timer_set_event(30, 5, MULTIMER_EVENT_INFINITE, on_timer_event, 0);

    // 添加首次35秒激发，以后间隔1秒激发的定时器，但是只激发一次
    mul_timer_set_event(30, 1, MULTIMER_EVENT_ONCEOFF, on_timer_event, 0);

    // 添加一次性定时器，仅在第10秒激发一次，忽略 MULTIMER_EVENT_INFINITE
    mul_timer_set_event(10, 0, MULTIMER_EVENT_INFINITE, on_timer_event, 0);
    */

    for (int i = 0; i < 5; i++) {
        mul_timer_set_event(0, 10, 10, on_timer_event, 0, MULTIMER_EVENT_CB_BLOCK);
    }

    mul_timer_start();

    while (1)
    {
        sleep(100);
        pause();
    }

    err = mul_timer_destroy();
    assert(! err);

    printf("TODO: %s-%s end.\n", APP_NAME, APP_VERSION);
    return (XS_ERROR);
}
