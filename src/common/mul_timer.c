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
* 2. Altered source versions must be plainly marked as such, and must not
*   be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/***********************************************************************
 * mul_timer.c
 *   multiply wheelz timer for linux and windows
 *
 *  多定时器的工业级实现, 支持时间单位自定义:
 *      Linux:   微秒级
 *      Windows: 毫秒级
 *
 * refer:
 *   http://blog.csdn.net/zhangxinrun/article/details/5914191
 *   https://linux.die.net/man/2/setitimer
 *
 * author: master@pepstack.com
 *
 * create: 2018-02-03 11:00
 * update: 2018-02-04
 **********************************************************************/

#include "mul_timer.h"


struct mul_timer_t multimer_singleton = {0};


struct mul_timer_t * get_multimer_singleton ()
{
    struct mul_timer_t *mtr = &multimer_singleton;

    printf("\033[32m* mul_timer=%p\033[0m\n", mtr);

    return mtr;
}


#ifdef _LINUX_GNUC
void sigalarm_handler (int signo)
{
    int err;
    bigint_t on_counter;

    mul_timer_t *ptimer = get_multimer_singleton();

    err = threadlock_trylock(&ptimer->lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MULTIMER_PRINT == 1
        printf("[mul]\033[33m lock error: %lld\033[0m\n", (long long) ptimer->counter);
    #endif
        return;
    }

    /** 当前激发的计数器 */
    on_counter = ptimer->counter++;

    /**
     * 激发事件：此事件在锁定状态下调用用户提供的回调函数：
     *   on_timer_event
     *
     * !! 因此不可以在on_timer_event中执行长时间的操作 !!
     */
    mul_timer_fire_event(ptimer, on_counter, ptimer->lpParameter);

    threadlock_unlock(&ptimer->lock);

#if MULTIMER_PRINT == 1
    printf(" * alarm: %llu\n", (unsigned long long) on_counter);
#endif
}
#endif


#ifdef _WINDOWS_MSVC
void __stdcall win_sigalarm_handler_example (PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    if (TimerOrWaitFired) {
        // TimerOrWaitFired:
        //   If this parameter is TRUE, the wait timed out.
        //   If this parameter is FALSE, the wait event has been signaled.
        // This parameter is always TRUE for timer callbacks.

        int err;
        bigint_t on_counter;

        mul_timer_t *ptimer = get_multimer_singleton();

        // The wait timed out
        err = threadlock_trylock(&ptimer->lock);
        if (err) {
            /** 多线程锁定失败 */
        #if MULTIMER_PRINT == 1
            printf("[mul]\033[33m lock error: %lld\033[0m\n", (long long) ptimer->counter);
        #endif
            return;
        }

        /** 当前激发的计数器 */
        on_counter = ptimer->counter++;

        /**
         * 激发事件：此事件在锁定状态下调用用户提供的回调函数：
         *   on_timer_event
         *
         * !! 因此不可以在on_timer_event中执行长时间的操作 !!
         */
        mul_timer_fire_event(ptimer, on_counter, lpParameter);

        threadlock_unlock(&ptimer->lock);

    #if MULTIMER_PRINT == 1
        printf(" * alarm: %llu\n", (unsigned long long) on_counter);
    #endif

        printf(" * alarm: %lld\n", on_counter);
    }
}
#endif

