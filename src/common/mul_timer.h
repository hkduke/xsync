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
 * mul_timer.h
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
#ifndef MUL_TIMER_H_INCLUDED
#define MUL_TIMER_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include "threadlock.h"
#include "dhlist.h"


#if defined(_WINDOWS_MSVC)
    void __stdcall win_sigalarm_handler (PVOID lpParameter, BOOLEAN TimerOrWaitFired);
#endif

#if defined(_LINUX_GNUC)
    extern void sigalarm_handler(int signo);
#endif


/***********************************************************************
 * 多定时器是否支持毫秒级?
 *
 * 下面声明 多定时器不支持毫秒级：
 *
 * #define MULTIMER_MILLI_SECOND  0
 * #include <mul_timer.h>
 *
 **********************************************************************/
#ifndef MULTIMER_MILLI_SECOND
#  define MULTIMER_MILLI_SECOND    1000
#endif


/***********************************************************************
 * 多定时器是否支持微秒级?
 *
 * 下面声明 多定时器不支持微秒级：
 *
 * #define MULTIMER_MICRO_SECOND  0
 * #include <mul_timer.h>
 *
 **********************************************************************/
#ifndef MULTIMER_MICRO_SECOND
#  define MULTIMER_MICRO_SECOND    1000000
#endif


#ifdef _WINDOWS_MSVC
    /* windows 不支持微秒级的定时器, 取消之 */
    #ifdef MULTIMER_MICRO_SECOND
    #  undef MULTIMER_MICRO_SECOND
    #endif
#endif


/***********************************************************************
 * 是否打印信息?
 *
 * 下面声明 不包含打印信息 (正式使用情况下)
 *
 * #define  MULTIMER_PRINT   0
 * #include <mul_timer.h>
 *
 **********************************************************************/
#ifndef MULTIMER_PRINT
#  define MULTIMER_PRINT   1
#endif


/***********************************************************************
 * MULTIMER_HASHLEN_MAX
 *
 * 定义 hash 桶的大小 (2^n -1) = [255, 1023, 4095, 8191]
 * 可以根据需要在编译时指定。桶越大，查找速度越快。
 **********************************************************************/
#ifndef MULTIMER_HASHLEN_MAX
#  define MULTIMER_HASHLEN_MAX     1023
#endif


/***********************************************************************
 * 是否使用双链表：如果不需要按次序遍历定时器不要启用它
 *
 * MULTIMER_HAS_DLIST == 1
 *   use dlist for traversing by sequence
 *
 * MULTIMER_HAS_DLIST == 0
 *   DONOT use dlist since we need not traversing by sequence
 **********************************************************************/
#define MULTIMER_HAS_DLIST   0

/**
 * global timer in process-wide
 */
extern struct mul_timer_t multimer_singleton;

extern struct mul_timer_t * get_multimer_singleton ();


typedef bigint_t mul_eventid_t;

typedef mul_eventid_t * mul_event_hdl;


typedef enum
{
    mul_timeunit_sec =  0

#if MULTIMER_MILLI_SECOND == 1000
    ,mul_timeunit_msec = 1
#endif

#if MULTIMER_MICRO_SECOND == 1000000
    ,mul_timeunit_usec = 2
#endif
} mul_timeunit_t;


typedef struct mul_event_t
{
    mul_eventid_t eventid;

    /* 引用计数： 0 删除, 1 保留 */
    ref_counter_t refc;

    void *eventarg;
    int (*timer_event_cb) (mul_event_hdl eventhdl, void *eventarg, void *lpParameter);

    ref_counter_t on_counter;

    /* 指定定时器首次激发时间和以后每次间隔激发时间 */
    struct itimerval value;

    int hash;

    /** dhlist node */
#if MULTIMER_HAS_DLIST == 1
    struct list_head  i_list;
#endif
    struct hlist_node i_hash;
} mul_event_t;


typedef struct mul_timer_t
{
    mul_eventid_t volatile eventid;

    ref_counter_t counter;

    thread_lock_t lock;

    /**
     * 定时器状态:
     *    1: 启动. (windows: status== 1)
     *    0: 暂停
     */
    int status;

    /** 最小时间单元值: 微秒 */
    bigint_t    timeunit_usec;
    mul_timeunit_t  timeunit_type;

    struct itimerval value;

#ifdef _LINUX_GNUC
    struct itimerval ovalue;

    void (* old_sigalarm)(int);
    void (* new_sigalarm)(int);
#endif

#ifdef _WINDOWS_MSVC
    HANDLE hTimerQueue;
#endif

    void * lpParameter;

    /** dhlist for timer entry */
#if MULTIMER_HAS_DLIST == 1
    struct list_head dlist;
#endif

    struct hlist_head hlist[MULTIMER_HASHLEN_MAX + 1];
} mul_timer_t;


__no_warning_unused_static
inline void free_timer_event (struct mul_event_t * event)
{
#if MULTIMER_PRINT == 1
    printf("\033[32m-delete event_%lld\033[0m\n", (long long) event->eventid);
#endif

    free(event);
}


__no_warning_unused_static
inline mul_event_t * mul_handle_cast_event (mul_event_hdl eventhdl)
{
    struct mul_event_t *event = CONTAINER_OF(eventhdl, struct mul_event_t, eventid);
    return event;
}


__no_warning_unused_static
inline bigint_t mul_hash_on_counter (mul_timer_t *mtr, bigint_t timeval_usec, int *hash)
{
    bigint_t on_counter = (bigint_t) (timeval_usec / mtr->timeunit_usec + mtr->counter);
    *hash = (on_counter & MULTIMER_HASHLEN_MAX);

#if MULTIMER_PRINT == 1
    printf(" * \033[36m on_counter=%lld, hash=%d\033[0m\n", (long long) on_counter, (*hash));
#endif

    return on_counter;
}


__no_warning_unused_static
int timer_select_sleep (int sec, int ms)
{
    if (sec || ms) {
        struct timeval tv = {0};

        tv.tv_sec = sec;
        tv.tv_usec = ms * 1000;

        return select (0, 0, 0, 0, &tv);
    } else {
        return 0;
    }
}


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
__no_warning_unused_static
int mul_timer_init (mul_timer_t *mtr, mul_timeunit_t timeunit, unsigned int timeintval, unsigned int delay,
#ifdef _LINUX_GNUC
    void (*sigalrm_cb)(int),
#endif

#ifdef _WINDOWS_MSVC
    void __stdcall(*win_sigalrm_cb)(PVOID, BOOLEAN),
#endif
    void *timerParameter, int start)
{
    int i, err;

    bzero(mtr, sizeof(struct mul_timer_t));

    mtr->eventid = 1;

#if MULTIMER_HAS_DLIST == 1
    INIT_LIST_HEAD(&mtr->dlist);
#endif
    for (i = 0; i <= MULTIMER_HASHLEN_MAX; i++) {
        INIT_HLIST_HEAD(&mtr->hlist[i]);
    }

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
        return (-1);
    }


#ifdef _LINUX_GNUC
    if ( (mtr->old_sigalarm = signal(SIGALRM, sigalrm_cb)) == SIG_ERR ) {
        threadlock_destroy(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul:error(%d)] signal: %s.\n", errno, strerror(errno));
    #endif
        return (-1);
    }
#endif


#ifdef _LINUX_GNUC
    mtr->new_sigalarm = sigalrm_cb;
#endif

    if (timeunit == mul_timeunit_sec) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mtr->value.it_value.tv_sec = delay;
        mtr->value.it_value.tv_usec = 0;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mtr->value.it_interval.tv_sec = timeintval;
        mtr->value.it_interval.tv_usec = 0;
    }

#if MULTIMER_MILLI_SECOND == 1000
    /** 如果支持毫秒定时器 */
    else if (timeunit == mul_timeunit_msec) {
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
    else if (timeunit == mul_timeunit_usec) {
        mtr->value.it_value.tv_sec = delay / MULTIMER_MICRO_SECOND;
        mtr->value.it_value.tv_usec = (delay % MULTIMER_MICRO_SECOND) * MULTIMER_MICRO_SECOND;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mtr->value.it_interval.tv_sec = timeintval / MULTIMER_MICRO_SECOND;
        mtr->value.it_interval.tv_usec = (timeintval % MULTIMER_MICRO_SECOND) * MULTIMER_MICRO_SECOND;
    }
#endif

    else {
#ifdef _LINUX_GNUC
        signal(SIGALRM, mtr->old_sigalarm);
#endif
        threadlock_destroy(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul:error] invalid timeunit: %d.", timeunit);
    #endif

        return (-1);
    }

    /** 时间单位：秒，毫秒，微秒 */
    mtr->timeunit_type = timeunit;

    /** 自动转化为微妙的时间单元 */
    mtr->timeunit_usec = mtr->value.it_interval.tv_usec + mtr->value.it_interval.tv_sec * 1000000;

#if MULTIMER_PRINT == 1
    printf("[] timeunit=%lld microseconds.\n", (long long) mtr->timeunit_usec);
#endif

#ifdef _LINUX_GNUC
    if (start) {
#endif

#ifdef _WINDOWS_MSVC
    if (1) {
#endif
        err = -1;

    #ifdef _LINUX_GNUC
        err = setitimer(ITIMER_REAL, &mtr->value, &mtr->ovalue);
    #endif

    #ifdef _WINDOWS_MSVC
        do {
            DWORD delay_ms = (DWORD) (mtr->value.it_value.tv_sec * 1000 + mtr->value.it_value.tv_usec / 1000);
            DWORD interval_ms = (DWORD) (mtr->value.it_interval.tv_sec * 1000 + mtr->value.it_interval.tv_usec / 1000);

            HANDLE hTimer;

            // Create the timer queue
            HANDLE hTimerQueue = CreateTimerQueue();

            if (! hTimerQueue) {
                printf("CreateTimerQueue failed (%d)\n", GetLastError());

                threadlock_destroy(&mtr->lock);

                return (-1);
            }

            // Set a timer to call the timer routine in 10 seconds.
            if (! CreateTimerQueueTimer(&hTimer, hTimerQueue,
                    (WAITORTIMERCALLBACK) win_sigalrm_cb,
                    timerParameter,
                    (DWORD) delay_ms,
                    (DWORD) interval_ms,
                    WT_EXECUTEDEFAULT)) {
                printf("CreateTimerQueueTimer failed (%d)\n", GetLastError());
                err = -1;
            } else {
                // Success
                mtr->hTimerQueue = hTimerQueue;
                mtr->lpParameter = timerParameter;
                err = 0;
            }
        } while(0);
    #endif

        if (! err) {
            /** 定时器成功创建并启动 */
            mtr->status = 1;

            threadlock_unlock(&mtr->lock);
        #if MULTIMER_PRINT == 1
            printf("[mul:info] mul_timer_init success.\n");
        #endif
            return 0;
        } else {
            /** 定时器创建但启动失败 */
            mtr->status = 0;

        #ifdef _LINUX_GNUC
            /** 定时器不可用，自动销毁 */
            signal(SIGALRM, mtr->old_sigalarm);
        #endif

        #ifdef _WINDOWS_MSVC
            DeleteTimerQueue(mtr->hTimerQueue);
        #endif

            threadlock_destroy(&mtr->lock);

        #if MULTIMER_PRINT == 1
            printf("[mul:error] mul_timer_init failed. setitimer error(%d): %s.\n", err, strerror(err));
        #endif

            return (-1);
        }
    } else {
        /** 定时器成功创建, 但不要求启动 */
        mtr->status = 0;
        threadlock_unlock(&mtr->lock);

    #if MULTIMER_PRINT == 1
        printf("[mul:info] mul_timer_init success without starting.\n");
    #endif

    #ifdef _WINDOWS_MSVC
        // Windows 不支持随后启动定时器
        #pragma message ("warnings: windows always start timer when init success")
    #endif
        return 0;
    }
}


/**
 * mul_timer_start
 *   启动定时器，仅对Linux有效。Windows自动启动定时器。
 *
 * returns:
 *    0: success  成功启动
 *    1: sfalse   已经启动
 *   -1: error    启动失败
 */
__no_warning_unused_static
int mul_timer_start (mul_timer_t *mtr)
{
#ifdef _WINDOWS_MSVC
    #pragma message("windows always return success(0) for this.")
    return 0;
#endif

#ifdef _LINUX_GNUC
    int err;

    err = threadlock_trylock(&mtr->lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MULTIMER_PRINT == 1
        printf("[mul:error(%d)] threadlock_trylock: %s\n", err, strerror(err));
    #endif
        return (-1);
    }

    if (mtr->status == 1) {
        /** 已经启动 */
        threadlock_unlock(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul:warn] already start.\n");
    #endif
        return 1;
    }

    err = setitimer(ITIMER_REAL, &mtr->value, &mtr->ovalue);
    if (! err) {
        /** 定时器成功启动 */
        mtr->status = 1;
        threadlock_unlock(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul:info] mul_timer_start success.\n");
    #endif
        return 0;
    } else {
        /** 定时器启动失败 */
        mtr->status = 0;
        threadlock_unlock(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul:error] mul_timer_start. setitimer error(%d): %s.\n", err, strerror(err));
    #endif
        return (-1);
    }
#endif
}


/**
 * mul_timer_destroy
 *
 * returns:
 *    0: success
 *   -1: failed.
 *      use strerror(errno) for error message.
 */
__no_warning_unused_static
int mul_timer_destroy (mul_timer_t *mtr)
{
    int err;

    err = threadlock_lock(&mtr->lock);
    if (err) {
    #if MULTIMER_PRINT == 1
        printf("[mul] threadlock_lock error(%d): %s\n", err, strerror(err));
    #endif
        return (-1);
    }

#ifdef _LINUX_GNUC
    if ((signal(SIGALRM, mtr->new_sigalarm)) == SIG_ERR) {
        threadlock_unlock(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul] destroy failed. signal error.\n");
    #endif
        return (-1);
    }

    /** 恢复进程原有的定时器 */
    err = setitimer(ITIMER_REAL, &mtr->ovalue, &mtr->value);
    if (err < 0) {
        threadlock_unlock(&mtr->lock);
    #if MULTIMER_PRINT == 1
        printf("[mul] destroy failed. setitimer error(%d): %s.\n", errno, strerror(errno));
    #endif
        return (-1);
    }
#endif

#ifdef _WINDOWS_MSVC
    // Delete all timers in the timer queue.
    if (! DeleteTimerQueue(mtr->hTimerQueue)) {
        printf("DeleteTimerQueue failed (%d)\n", GetLastError());
        threadlock_unlock(&mtr->lock);
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

    threadlock_destroy(&mtr->lock);

    bzero(mtr, sizeof(struct mul_timer_t));

#if MULTIMER_PRINT == 1
    printf("[mul] destroy success.\n");
#endif

    return(0);
}


/**
 * mul_timer_set_event
 *   设置 event timer
 *
 * params：
 *   delay - 指定首次激发的时间：当前定时器首次启动之后多少时间激发 event：
 *       0 : 立即激发
 *     > 0 : 延迟时间
 *
 *   interval - 首次激发 event 之后间隔多久激发：
 *       0 : 不激发
 *     > 0 : 间隔多久时间激发
 *
 *   count： 最多激发次数
 *      MULTIMER_EVENT_INFINITE: 永久激发
 *     > 0 : 次数
 *
 * returns:
 *    > 0: mul_eventid_t, success
 *    < 0: failed
 */
#define MULTIMER_EVENT_ONCEOFF       (1)
#define MULTIMER_EVENT_INFINITE     (-1)


__no_warning_unused_static
mul_eventid_t mul_timer_set_event (mul_timer_t *mtr, bigint_t delay, bigint_t interval, bigint_t count,
    int (*on_event_cb)(mul_event_hdl eventhdl, void *eventarg, void *timerarg), void *eventarg)
{
    int err, hash;
    bigint_t on_counter;
    mul_event_t *new_event;

    bigint_t delay_usec = 0;
    bigint_t interval_usec = 0;

    if (count == MULTIMER_EVENT_INFINITE) {
        count = INT64_MAX;
    }

    if (delay < 0 || interval < 0 || count <= 0) {
        /** 无效的定时器 */
        return (-2);
    }

    if (mtr->timeunit_type == mul_timeunit_sec) {
        delay_usec = delay * 1000000;
        interval_usec = interval * 1000000;
    }

#if MULTIMER_MILLI_SECOND == 1000
    else if (mtr->timeunit_type == mul_timeunit_msec) {
        delay_usec = delay * MULTIMER_MILLI_SECOND;
        interval_usec = interval * MULTIMER_MILLI_SECOND;
    }
#endif

#if MULTIMER_MICRO_SECOND == 1000000
    else if (mtr->timeunit_type == mul_timeunit_usec) {
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

    new_event->eventid = mtr->eventid++;

    new_event->on_counter = on_counter;

    /** 首次激发时间 */
    new_event->value.it_value.tv_sec = (long) (delay_usec / 1000000);
    new_event->value.it_value.tv_usec = (long) (delay_usec % 1000000);

    /** 间隔激发时间 */
    new_event->value.it_interval.tv_sec = (long) (interval_usec / 1000000);
    new_event->value.it_interval.tv_usec = (long) (interval_usec % 1000000);

    new_event->hash = hash;

    /** 设置回调参数和函数，回调函数由用户自己实现 */
    new_event->eventarg = eventarg;
    new_event->timer_event_cb = on_event_cb;

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

    /** 设置引用计数为 1 */
    new_event->refc = count;

#if MULTIMER_PRINT == 1
    // 演示如何删除自身：
    ////hlist_del(&new_event->i_hash);
    ////list_del(&new_event->i_list);
    ////free_timer_event(new_event);
#endif

    threadlock_unlock(&mtr->lock);

    return new_event->eventid;
}


/**
 * mul_timer_remove_event
 *   从多定时器中删除事件。该调用仅仅标记事件要删除。真正删除的行为由系统决定。
 *
 * returns:
 *    0: success
 *   -1: failed.
 */
__no_warning_unused_static
int mul_timer_remove_event (mul_timer_t *mtr, mul_event_hdl eventhdl)
{
    mul_event_t *event = mul_handle_cast_event(eventhdl);

#if MULTIMER_PRINT == 1
    printf("remove event-%lld\n", (long long) event->eventid);
#endif

    /** 设置引用计数为 0，当有事件触发时自动删除 */
    __interlock_release(&event->refc);

    return 0;
}


/**
 * mul_timer_fire_event
 *   根据当前激发的 counter 查找 event
 *
 * params:
 *   on_counter - 当前激发的计数器
 *
 * returns:
 *   number of events have been fired
 */
__no_warning_unused_static
int mul_timer_fire_event (mul_timer_t *mtr, bigint_t fire_counter, void * lpParamter)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int num_events_fired = 0;

    int hash = (int) (fire_counter & MULTIMER_HASHLEN_MAX);

    hlist_for_each_safe(hp, hn, &mtr->hlist[hash]) {
        struct mul_event_t * event = hlist_entry(hp, struct mul_event_t, i_hash);

        if (event->on_counter == fire_counter) {
            /** 首先从链表中删除自己 */
            hlist_del(&event->i_hash);

        #if MULTIMER_HAS_DLIST == 1
            list_del(&event->i_list);
        #endif

            /** 激发事件回调函数 */
            event->timer_event_cb(&event->eventid, event->eventarg, lpParamter);
            num_events_fired++;

            if (__interlock_sub(&event->refc) <= 0) {
                /** 要求删除事件 */
                free_timer_event(event);
            } else {
                if (event->value.it_interval.tv_sec == 0) {
                    /* 只使用一次, 下次不再激发，删除事件 */
                    free_timer_event(event);
                } else {
                    event->on_counter = mul_hash_on_counter(mtr,
                        event->value.it_interval.tv_sec * 1000000 + event->value.it_interval.tv_usec,
                        &event->hash);

                #if MULTIMER_HAS_DLIST == 1
                    /** 串入长串 */
                    list_add(&event->i_list, &mtr->dlist);
                #endif
                    /** 串入HASH短串 */
                    hlist_add_head(&event->i_hash, &mtr->hlist[event->hash]);
                }
            }
        }
    }

    return num_events_fired;
}


#if defined(__cplusplus)
}
#endif

#endif /* MUL_TIMER_H_INCLUDED */
