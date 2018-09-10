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
#define MULTIMER_PRINT  0

#include "mul_timer.h"


struct mul_timer_t multimer_singleton = {0};


extern struct mul_timer_t * get_multimer_singleton ()
{
    struct mul_timer_t *mtr = &multimer_singleton;
#if MULTIMER_PRINT == 1
    printf("\033[32m* mul_timer=%p\033[0m\n", mtr);
#endif
    return mtr;
}


#ifdef _LINUX_GNUC
extern void sigalarm_handler (int signo)
{
    int err;

    mul_counter_t on_counter;

    mul_timer_t *mtr = get_multimer_singleton();

    if (mtr->start_flag == 0) {
    #if MULTIMER_PRINT == 1
        printf("[mul:warn]\033[33m timer not start counting !\033[0m\n");
    #endif
        return;
    }

    /** 锁定使用定时器 */
    err = threadlock_trylock(&mtr->lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MULTIMER_PRINT == 1
        printf("[mul:error(%d)]\033[33m threadlock_trylock failed: %s.\033[0m\n", err, strerror(err));
    #endif
        return;
    }

    /** 当前激发的计数器 */
    on_counter = mtr->counter;

    /**
     * 激发事件：此事件在锁定状态下调用用户提供的回调函数：
     *   on_timer_event
     *
     * !! 因此不可以在on_timer_event中执行长时间的操作，会阻塞系统调用 !!
     */
    mul_timer_fire_event(mtr, on_counter);

    mtr->counter++;

    /** 解锁定时器 */
    threadlock_unlock(&mtr->lock);

#if MULTIMER_PRINT == 1
    printf(" * [mul: %p] alarm: %lld\n", mtr, (long long) on_counter);
#endif
}
#endif


#ifdef _WINDOWS_MSVC
extern void __stdcall win_sigalarm_handler_example (PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    if (TimerOrWaitFired) {
        // TimerOrWaitFired:
        //   If this parameter is TRUE, the wait timed out.
        //   If this parameter is FALSE, the wait event has been signaled.
        // This parameter is always TRUE for timer callbacks.

        int err;
        mul_counter_t on_counter;

        mul_timer_t *mtr = get_multimer_singleton();

        if (mtr->start_flag == 0) {
        #if MULTIMER_PRINT == 1
            printf("[mul:warn]\033[33m timer not start counting !\033[0m\n");
        #endif
            return;
        }

        // The wait timed out
        err = threadlock_trylock(&mtr->lock);
        if (err) {
            /** 多线程锁定失败 */
        #if MULTIMER_PRINT == 1
            printf("[mul]\033[33m lock error: %lld\033[0m\n", (long long) mtr->counter);
        #endif
            return;
        }

        /** 当前激发的计数器 */
        on_counter = mtr->counter;

        /**
         * 阻塞式激发事件：此事件在锁定状态下调用用户提供的回调函数：
         *   on_timer_event
         *
         * !! 因此不可以在on_timer_event中执行长时间的操作 !!
         */
        mul_timer_fire_event(mtr, on_counter);

        mtr->counter++;

        threadlock_unlock(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf(" * [mul: %p] alarm: %lld\n", mtr, (long long) on_counter);
    #endif
    }
}
#endif


/**
 * mul_timer_init
 *
 *   初始化定时器。如果初始化不成功，不能使用定时器。
 *
 * params:
 *       timeunit - time unit: second, millisecond or microsecond (for linux)
 *       timeintval - The period of the timer, in timeunit.
 *                  If this parameter is zero, the timer is signaled once.
 *                  If this parameter is greater than zero, the timer is periodic.
 *                  A periodic timer automatically reactivates each time the period
*                     elapses, until the timer is canceled.
 *       delay - The amount of time in timeunit relative to the current time that
 *                 must elapse before the timer is signaled for the first time.
 *       start - only for linux:
 *          = 0 (do not start timer);
 *          = 1 (start immediately when init success)
 *
 *       lpParameter - A single parameter value that will be passed to the callback function.
 *
 * returns:
 *    0: success  初始化成功
 *   -1: error    初始化失败
 */
extern int mul_timer_init (mul_timeunit_t timeunit, unsigned int timeintval, unsigned int delay,
#ifdef _LINUX_GNUC
    void (*sigalrm_cb)(int),
#endif

#ifdef _WINDOWS_MSVC
    void __stdcall(*win_sigalrm_cb)(PVOID, BOOLEAN),
#endif
    void *timerarg, int start)
{
    int i, err;

    mul_timer_t *mtr = get_multimer_singleton();

    bzero(mtr, sizeof(*mtr));

    err = threadlock_init(&mtr->lock);
    if (err) {
        /* nerver run to this ! */
    #if MULTIMER_PRINT == 1
        printf("[mul:error(%d)] threadlock_init: %s.\n", err, strerror(err));
    #endif
        return (-1);
    }

    err = threadlock_lock(&mtr->lock);
    if (err) {
    #if MULTIMER_PRINT == 1
        printf("[mul:error(%d)] threadlock_lock: %s.\n", err, strerror(err));
    #endif
        threadlock_destroy(&mtr->lock);
        return (-1);
    }

#ifdef _LINUX_GNUC
    if ( (mtr->old_sighdl = signal(SIGALRM, sigalrm_cb)) == SIG_ERR ) {
        threadlock_destroy(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul:error(%d)] signal: %s.\n", errno, strerror(errno));
    #endif
        return (-1);
    }
#endif

    if (timeunit == MUL_TIMEUNIT_SEC) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mtr->value.it_value.tv_sec = delay;
        mtr->value.it_value.tv_usec = 0;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mtr->value.it_interval.tv_sec = timeintval;
        mtr->value.it_interval.tv_usec = 0;
    }

#if MULTIMER_MILLI_SECOND == 1000
    /** 如果支持毫秒定时器 */
    else if (timeunit == MUL_TIMEUNIT_MSEC) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mtr->value.it_value.tv_sec = delay / MULTIMER_MILLI_SECOND;
        mtr->value.it_value.tv_usec = (delay % MULTIMER_MILLI_SECOND) * MULTIMER_MILLI_SECOND;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mtr->value.it_interval.tv_sec = timeintval / MULTIMER_MILLI_SECOND;
        mtr->value.it_interval.tv_usec = (timeintval % MULTIMER_MILLI_SECOND) * MULTIMER_MILLI_SECOND;
    }
#endif

#if MULTIMER_MICRO_SECOND == 1000000
    /** 如果支持微秒定时器 */
    else if (timeunit == MUL_TIMEUNIT_USEC) {
        mtr->value.it_value.tv_sec = delay / MULTIMER_MICRO_SECOND;
        mtr->value.it_value.tv_usec = (delay % MULTIMER_MICRO_SECOND) * MULTIMER_MICRO_SECOND;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mtr->value.it_interval.tv_sec = timeintval / MULTIMER_MICRO_SECOND;
        mtr->value.it_interval.tv_usec = (timeintval % MULTIMER_MICRO_SECOND) * MULTIMER_MICRO_SECOND;
    }
#endif

    else {
#ifdef _LINUX_GNUC
        /** 恢复原有的信号处理函数 */
        signal(SIGALRM, mtr->old_sighdl);
#endif
        threadlock_destroy(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul:error] invalid timeunit: %d.", timeunit);
    #endif
        return (-1);
    }

    /** 时间单位：秒，毫秒，微秒 */
    mtr->timeunit_id = timeunit;

    /** 自动转化为微妙的时间单元 */
    mtr->timeunit_usec = mtr->value.it_interval.tv_usec + mtr->value.it_interval.tv_sec * 1000000;

#if MULTIMER_PRINT == 1
    printf("[mul:info] timeunit=%lld microseconds.\n", (long long) mtr->timeunit_usec);
#endif

    err = -1;

#ifdef _WINDOWS_MSVC
    do {
        DWORD delay_ms = (DWORD) (mtr->value.it_value.tv_sec * 1000 + mtr->value.it_value.tv_usec / 1000);
        DWORD interval_ms = (DWORD) (mtr->value.it_interval.tv_sec * 1000 + mtr->value.it_interval.tv_usec / 1000);

        HANDLE hTimer;

        // Create the timer queue
        HANDLE hTimerQueue = CreateTimerQueue();

        if (! hTimerQueue) {
        #if MULTIMER_PRINT == 1
            printf("CreateTimerQueue failed (%d)\n", GetLastError());
        #endif
            threadlock_destroy(&mtr->lock);
            return (-1);
        }

        // Set a timer to call the timer routine in 10 seconds.
        if (! CreateTimerQueueTimer(&hTimer, hTimerQueue,
                (WAITORTIMERCALLBACK) win_sigalrm_cb,
                timerarg,
                (DWORD) delay_ms,
                (DWORD) interval_ms,
                WT_EXECUTEDEFAULT))
        {
        #if MULTIMER_PRINT == 1
            printf("CreateTimerQueueTimer failed (%d)\n", GetLastError());
        #endif
            DeleteTimerQueue(hTimerQueue);
            threadlock_destroy(&mtr->lock);
            return (-1);
        } else {
            // Success
            mtr->hTimerQueue = hTimerQueue;
            mtr->lpParameter = timerarg;
            err = 0;
        }
    } while(0);
#endif

#ifdef _LINUX_GNUC
    err = setitimer(ITIMER_REAL, &mtr->value, &mtr->ovalue);
    if (err) {
        /** 定时器不可用，自动销毁 */
        signal(SIGALRM, mtr->old_sighdl);
        threadlock_destroy(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul:error] mul_timer_init failed. setitimer error(%d): %s.\n", err, strerror(err));
    #endif
        return (-1);
    }
#endif

    /** 定时器创建成功：初始化链表 */
#if MULTIMER_HAS_DLIST == 1
    INIT_LIST_HEAD(&mtr->dlist);
#endif
    for (i = 0; i <= MULTIMER_HASHLEN_MAX; i++) {
        INIT_HLIST_HEAD(&mtr->hlist[i]);
    }

    /** 设置初始事件id 和 多定时器全局参数 */
    mtr->eventid = 0;
    mtr->lpParameter = timerarg;

    /** 定时器成功创建并启动 */
    assert(mtr->start_flag == 0);
    if (start) {
        mtr->start_flag = 1;
    }

    /** 解锁定时器 */
    threadlock_unlock(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul:info] mul_timer_init success.\n");
    #endif

    return 0;
}


extern void mul_timer_start (void)
{
    mul_timer_t *mtr = get_multimer_singleton();
    mtr->start_flag = 1;
}


extern void mul_timer_pause ()
{
    mul_timer_t *mtr = get_multimer_singleton();
    mtr->start_flag = 0;
}


extern int mul_timer_destroy ()
{
    int err;
    int saved_start_flag;

    void (* saved_sighdl)(int);

    mul_timer_t *mtr = get_multimer_singleton();

    saved_start_flag = mtr->start_flag;

    /* first pause it */
    mtr->start_flag = 0;

    err = threadlock_lock(&mtr->lock);
    if (err) {
    #if MULTIMER_PRINT == 1
        printf("[mul] threadlock_lock error(%d): %s\n", err, strerror(err));
    #endif
        mtr->start_flag = saved_start_flag;
        return (-1);
    }

#ifdef _LINUX_GNUC
    /** 先忽略信号处理函数 */
    if ((saved_sighdl = signal(SIGALRM, SIG_IGN)) == SIG_ERR) {
        threadlock_unlock(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul] destroy failed. signal error.\n");
    #endif
        mtr->start_flag = saved_start_flag;
        return (-1);
    }

    /** 恢复进程原有的定时器 */
    err = setitimer(ITIMER_REAL, &mtr->ovalue, &mtr->value);
    if (err < 0) {
        /** 恢复失败，重新设置信号函数 */
        signal(SIGALRM, saved_sighdl);

        threadlock_unlock(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul] destroy failed. setitimer error(%d): %s.\n", errno, strerror(errno));
    #endif

        mtr->start_flag = saved_start_flag;
        return (-1);
    }

    /** 恢复进程原有信号处理函数: 不用检查失败 */
    signal(SIGALRM, mtr->old_sighdl);
#endif

#ifdef _WINDOWS_MSVC
    // Delete all timers in the timer queue.
    if (! DeleteTimerQueue(mtr->hTimerQueue)) {
        printf("DeleteTimerQueue failed (%d)\n", GetLastError());
        threadlock_unlock(&mtr->lock);

        mtr->start_flag = saved_start_flag;
        return (-1);
    }
#endif

    /** 清空定时器链表 */
#if MULTIMER_HAS_DLIST == 1
    do {
        struct list_head *list, *node;

        list_for_each_safe(list, node, &mtr->dlist) {
            struct mul_event_t * event = list_entry(list, struct mul_event_t, i_list);

            hlist_del(&event->i_hash);
            list_del(&event->i_list);

            free_timer_event(event);
        }
    } while(0);

#else

    do {
        int hash;
        struct hlist_node *hp, *hn;

        for (hash = 0; hash <= MULTIMER_HASHLEN_MAX; hash++) {
            hlist_for_each_safe(hp, hn, &mtr->hlist[hash]) {
                struct mul_event_t * event = hlist_entry(hp, struct mul_event_t, i_hash);

                hlist_del(&event->i_hash);

                free_timer_event(event);
            }
        }
    } while(0);

#endif

    /** 销毁定时器 */
    threadlock_destroy(&mtr->lock);
    bzero(mtr, sizeof(*mtr));

#if MULTIMER_PRINT == 1
    printf("[mul] destroy success.\n");
#endif

    return(0);
}


extern mul_eventid_t mul_timer_set_event (bigint_t delay, bigint_t interval, mul_counter_t count,
    mul_event_cb_func event_cb, int eventargid, void *eventarg, int event_cb_flag)
{
    int err, hash;

    mul_counter_t on_counter;
    mul_event_t *new_event;

    bigint_t delay_usec = 0;
    bigint_t interval_usec = 0;

    mul_timer_t *mtr = get_multimer_singleton();

    if (count == MULTIMER_EVENT_INFINITE) {
        count = INT64_MAX;
    }

    assert(event_cb_flag == MULTIMER_EVENT_CB_BLOCK ||
        event_cb_flag == MULTIMER_EVENT_CB_NONBLOCK ||
        event_cb_flag == MULTIMER_EVENT_CB_IGNORED);

    if (delay < 0 || interval < 0 || count <= 0) {
        /** 无效的定时器 */
        return (-3);
    }

    if (mtr->timeunit_id == MUL_TIMEUNIT_SEC) {
        delay_usec = delay * 1000000;
        interval_usec = interval * 1000000;
    }

#if MULTIMER_MILLI_SECOND == 1000
    else if (mtr->timeunit_id == MUL_TIMEUNIT_MSEC) {
        delay_usec = delay * MULTIMER_MILLI_SECOND;
        interval_usec = interval * MULTIMER_MILLI_SECOND;
    }
#endif

#if MULTIMER_MICRO_SECOND == 1000000
    else if (mtr->timeunit_id == MUL_TIMEUNIT_USEC) {
        delay_usec = delay;
        interval_usec = interval;
    }
#endif

    if (delay_usec == 0 && interval_usec == 0) {
        /** 无效的定时器 */
        return (-2);
    }

    err = threadlock_trylock(&mtr->lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MULTIMER_PRINT == 1
        printf("[mul] threadlock_trylock error(%d): %s\n", err, strerror(err));
    #endif
        return (-1);
    }

    /**
     * 当 mtr->counter == on_counter 时激发
     * 因此设置以 on_counter 为 hash 键保存 event
     */
    on_counter = mul_hash_on_counter(mtr, delay_usec, &hash);

    new_event = (mul_event_t *) malloc(sizeof(mul_event_t));
    if (! new_event) {
        /** out of memory */
        threadlock_unlock(&mtr->lock);
        return (-4);
    }

    bzero(new_event, sizeof(mul_event_t));

    new_event->eventid = __interlock_add(&mtr->eventid);
    new_event->on_counter = on_counter;

    /** 首次激发时间 */
    new_event->value.it_value.tv_sec = (long) (delay_usec / 1000000);
    new_event->value.it_value.tv_usec = (long) (delay_usec % 1000000);

    /** 间隔激发时间 */
    new_event->value.it_interval.tv_sec = (long) (interval_usec / 1000000);
    new_event->value.it_interval.tv_usec = (long) (interval_usec % 1000000);

    new_event->hash = hash;

    /** 设置回调参数和函数，回调函数由用户自己实现 */
    new_event->cb_flag = MULTIMER_EVENT_CB_IGNORED;
    new_event->eventargid = eventargid;
    new_event->eventarg = eventarg;
    new_event->timer_event_cb = event_cb;

#if MULTIMER_PRINT == 1
    printf("\033[31m+create event_%lld\033[0m\n", (long long) new_event->eventid);

    printf("\033[31m+create event_%lld. (%d : %d)\033[0m\n", (long long) new_event->eventid,
        (int) new_event->value.it_value.tv_sec, (int) new_event->value.it_interval.tv_sec);
#endif

#if MULTIMER_HAS_DLIST == 1
    /** 串入长串 */
    list_add(&new_event->i_list, &mtr->dlist);
#endif

    /** 串入HASH短串 */
    hlist_add_head(&new_event->i_hash, &mtr->hlist[hash]);

#if MULTIMER_PRINT == 1
    // 演示如何删除自身：
    ////hlist_del(&new_event->i_hash);
    ////list_del(&new_event->i_list);
    ////free_timer_event(new_event);
#endif

    /** 设置引用计数 */
    new_event->refc = count;
    new_event->cb_flag = event_cb_flag;

    threadlock_unlock(&mtr->lock);

    return new_event->eventid;
}


extern int mul_timer_remove_event (mul_event_hdl eventhdl)
{
    mul_event_t *event = mul_handle_cast_event(eventhdl);

#if MULTIMER_PRINT == 1
    printf("remove event_%lld\n", (long long) event->eventid);
#endif

    /** 设置引用计数为 0，当有事件触发时自动删除 */
    __interlock_release(&event->refc);

    return 0;
}
