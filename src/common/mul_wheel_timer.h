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
 * mul_wheel_timer.h
 *   multiply wheel timer for linux and windows
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
#ifndef MUL_WHEEL_TIMER_H_INCLUDED
#define MUL_WHEEL_TIMER_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif


#if defined _MSC_VER || WIN32
    #undef MWT_OS_LINUX
    #define MWT_OS_WINDOWS
    #pragma message("build for windows ...")
#else
    #undef MWT_OS_WINDOWS
    #define MWT_OS_LINUX
#endif


/**
 * thread lock
 */
#ifdef MWT_OS_WINDOWS
    // Windows
    //   https://msdn.microsoft.com/en-us/library/windows/desktop/ms687003(v=vs.85).aspx
    #include <windows.h>
    #include <process.h>

#if _WIN32_WINNT  < 0x0500
    #undef _WIN32_WINNT
    #define _WIN32_WINNT    0x0500
    #pragma message("windows version is too low")
#endif

    typedef CRITICAL_SECTION thread_lock_t;

    typedef LONGLONG mwt_bigint_t;

    typedef LONGLONG volatile mwt_counter_t;

    typedef struct itimerval {
        struct timeval it_value;
        struct timeval it_interval;
    } itimerval;


    #define __disable_warning_unused_static  static \

    static inline void bzero(void * pv, size_t size)
    {
        memset(pv, 0, size);
    }

    /* CriticalSection */
    static inline int threadlock_init(thread_lock_t *lock)
    {
        InitializeCriticalSection(lock);
        return 0;
    }

    #define threadlock_fini(lock)    DeleteCriticalSection(lock)

    static inline int threadlock_enter(thread_lock_t *lock)
    {
        EnterCriticalSection(lock);
        return 0;
    }

    #define threadlock_tryenter(lock)   (TryEnterCriticalSection(lock)? 0:(-1))
    #define threadlock_leave(lock)      LeaveCriticalSection(lock)

    /* refcount */
    #define __interlock_add(add)        InterlockedIncrement64(add)
    #define __interlock_sub(sub)        InterlockedDecrement64(sub)
    #define __interlock_release(val)    InterlockedExchange64(val, 0)

    void __stdcall win_sigalrm_handler (PVOID lpParameter, BOOLEAN TimerOrWaitFired);

#else
    /* linux */

    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>

    typedef pthread_mutex_t  thread_lock_t;

    typedef int64_t volatile mwt_counter_t;

    typedef int64_t mwt_bigint_t;

    #define __disable_warning_unused_static    __attribute__((used)) static \

    /* pthread mutex */
    #define threadlock_init(lock)        pthread_mutex_init(lock, 0)
    #define threadlock_fini(lock)        pthread_mutex_destroy(lock)

    #define threadlock_enter(lock)       pthread_mutex_lock(lock)
    #define threadlock_tryenter(lock)    pthread_mutex_trylock(lock)
    #define threadlock_leave(lock)       pthread_mutex_unlock(lock)

    /* refcount */
    #define __interlock_add(add)         __sync_add_and_fetch(add, 1)
    #define __interlock_sub(sub)         __sync_sub_and_fetch(sub, 1)
    #define __interlock_release(val)     __sync_lock_release(val)

    __disable_warning_unused_static void sigalarm_handler(int signo);
#endif

#include <stdint.h>
#include <time.h>

#include "dhlist.h"


/***********************************************************************
 * 多定时器是否支持毫秒级?
 *
 * 下面声明 多定时器不支持毫秒级：
 *
 * #define MWTIMER_MILLI_SECOND  0
 * #include <mul_wheel_timer.h>
 *
 **********************************************************************/
#ifndef MWTIMER_MILLI_SECOND
#  define MWTIMER_MILLI_SECOND    1000
#endif


/***********************************************************************
 * 多定时器是否支持微秒级?
 *
 * 下面声明 多定时器不支持微秒级：
 *
 * #define MWTIMER_MICRO_SECOND  0
 * #include <mul_wheel_timer.h>
 *
 **********************************************************************/
#ifndef MWTIMER_MICRO_SECOND
#  define MWTIMER_MICRO_SECOND    1000000
#endif


#ifdef MWT_OS_WINDOWS
/* windows 不支持微秒级的定时器, 取消之 */
    #undef MWTIMER_MICRO_SECOND
#endif


/***********************************************************************
 * 是否打印信息?
 *
 * 下面声明 不包含打印信息 (正式使用情况下)
 *
 * #define  MWTIMER_PRINTF   0
 * #include <mul_wheel_timer.h>
 *
 **********************************************************************/
#ifndef MWTIMER_PRINTF
#  define MWTIMER_PRINTF   1
#endif


/***********************************************************************
 * MWTIMER_HASHLEN_MAX
 *
 * 定义 hash 桶的大小 (2^n -1) = [255, 1023, 4095, 8191]
 * 可以根据需要在编译时指定。桶越大，查找速度越快。
 **********************************************************************/
#ifndef MWTIMER_HASHLEN_MAX
#  define MWTIMER_HASHLEN_MAX     1023
#endif


/***********************************************************************
 * 是否使用双链表：如果不需要按次序遍历定时器不要启用它
 *
 * MWTIMER_HAS_DLIST == 1
 *   use dlist for traversing by sequence
 *
 * MWTIMER_HAS_DLIST == 0
 *   DONOT use dlist since we need not traversing by sequence
 **********************************************************************/
#define MWTIMER_HAS_DLIST   0


typedef mwt_bigint_t mwt_eventid_t;

typedef mwt_eventid_t * mwt_event_hdl;


typedef enum
{
    mwt_timeunit_sec =  0

#if MWTIMER_MILLI_SECOND == 1000
    ,mwt_timeunit_msec = 1
#endif

#if MWTIMER_MICRO_SECOND == 1000000
    ,mwt_timeunit_usec = 2
#endif
} mwt_timeunit_t;


typedef struct mwt_event_t
{
    mwt_eventid_t eventid;

    /* 引用计数： 0 删除, 1 保留 */
    mwt_counter_t refc;

    void *eventarg;
    int (*timer_event_cb) (mwt_event_hdl eventhdl, void *eventarg, void *lpParameter);

    mwt_counter_t on_counter;

    /* 指定定时器首次激发时间和以后每次间隔激发时间 */
    struct itimerval value;

    int hash;

    /** dhlist node */
#if MWTIMER_HAS_DLIST == 1
    struct list_head  i_list;
#endif
    struct hlist_node i_hash;
} mwt_event_t;


typedef struct mul_wheel_timer_t
{
    mwt_eventid_t volatile eventid;

    mwt_counter_t counter;

    thread_lock_t lock;

    /**
     * 定时器状态:
     *    1: 启动. (windows: status== 1)
     *    0: 暂停
     */
    int status;

    /** 最小时间单元值: 微秒 */
    mwt_bigint_t    timeunit_usec;
    mwt_timeunit_t  timeunit_type;

    struct itimerval value;

#ifdef MWT_OS_LINUX
    struct itimerval ovalue;

    void (* old_sigalarm)(int);
    void (* new_sigalarm)(int);
#endif

#ifdef MWT_OS_WINDOWS
    HANDLE hTimerQueue;
#endif

    void * lpParameter;

    /** dhlist for timer entry */
#if MWTIMER_HAS_DLIST == 1
    struct list_head dlist;
#endif

    struct hlist_head hlist[MWTIMER_HASHLEN_MAX + 1];
} mul_wheel_timer_t;


/* global timer variable for one linux process */
__disable_warning_unused_static
struct mul_wheel_timer_t  mulwheeltimer;


__disable_warning_unused_static
inline void free_timer_event (struct mwt_event_t * event)
{
#if MWTIMER_PRINTF == 1
    printf("\033[32m-delete event_%lld\033[0m\n", (long long) event->eventid);
#endif

    free(event);
}


__disable_warning_unused_static
inline mwt_event_t * mul_handle_cast_event (mwt_event_hdl eventhdl)
{
    struct mwt_event_t * event = CONTAINER_OF(eventhdl, struct mwt_event_t, eventid);
    return event;
}


__disable_warning_unused_static
inline mwt_bigint_t mwt_hash_on_counter (mul_wheel_timer_t * mwt, mwt_bigint_t timeval_usec, int *hash)
{
    mwt_bigint_t on_counter = (mwt_bigint_t) (timeval_usec / mwt->timeunit_usec + mwt->counter);
    *hash = (on_counter & MWTIMER_HASHLEN_MAX);

#if MWTIMER_PRINTF == 1
    printf(" * \033[36m on_counter=%lld, hash=%d\033[0m\n", (long long) on_counter, (*hash));
#endif

    return on_counter;
}


__disable_warning_unused_static
int timer_select_sleep (int sec, int ms)
{
    if (sec || ms) {
        struct timeval tv = {0};

        tv.tv_sec = sec;
        tv.tv_usec = ms * MWTIMER_MILLI_SECOND;

        return select (0, NULL, NULL, NULL, &tv);
    } else {
        return 0;
    }
}


/**
 * mul_wheel_timer_init
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
__disable_warning_unused_static
int mul_wheel_timer_init (mwt_timeunit_t timeunit, unsigned int timeintval, unsigned int delay, void *timerParameter ,int start)
{
    int i, err;

    bzero(&mulwheeltimer, sizeof(struct mul_wheel_timer_t));

    mulwheeltimer.eventid = 1;

#if MWTIMER_HAS_DLIST == 1
    INIT_LIST_HEAD(&mulwheeltimer.dlist);
#endif
    for (i = 0; i <= MWTIMER_HASHLEN_MAX; i++) {
        INIT_HLIST_HEAD(&mulwheeltimer.hlist[i]);
    }

    err = threadlock_init(&mulwheeltimer.lock);
    if (err) {
        /* nerver run to this ! */
    #if MWTIMER_PRINTF == 1
        printf("[mwt:error(%d)] threadlock_init: %s.\n", err, strerror(err));
    #endif
        return (-1);
    }

    err = threadlock_enter(&mulwheeltimer.lock);
    if (err) {
    #if MWTIMER_PRINTF == 1
        printf("[mwt:error(%d)] threadlock_enter: %s.\n", err, strerror(err));
    #endif
        return (-1);
    }


#ifdef MWT_OS_LINUX
    if ( (mulwheeltimer.old_sigalarm = signal(SIGALRM, sigalarm_handler)) == SIG_ERR ) {
        threadlock_fini(&mulwheeltimer.lock);
    #if MWTIMER_PRINTF == 1
        printf("[mwt:error(%d)] signal: %s.\n", errno, strerror(errno));
    #endif
        return (-1);
    }
#endif


#ifdef MWT_OS_LINUX
    mulwheeltimer.new_sigalarm = sigalarm_handler;
#endif

    if (timeunit == mwt_timeunit_sec) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mulwheeltimer.value.it_value.tv_sec = delay;
        mulwheeltimer.value.it_value.tv_usec = 0;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mulwheeltimer.value.it_interval.tv_sec = timeintval;
        mulwheeltimer.value.it_interval.tv_usec = 0;
    }

#if MWTIMER_MILLI_SECOND == 1000
    /** 如果支持毫秒定时器 */
    else if (timeunit == mwt_timeunit_msec) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mulwheeltimer.value.it_value.tv_sec = delay / MWTIMER_MILLI_SECOND;
        mulwheeltimer.value.it_value.tv_usec = (delay % MWTIMER_MILLI_SECOND) * MWTIMER_MILLI_SECOND;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mulwheeltimer.value.it_interval.tv_sec = timeintval / MWTIMER_MILLI_SECOND;
        mulwheeltimer.value.it_interval.tv_usec = (timeintval % MWTIMER_MILLI_SECOND) * MWTIMER_MILLI_SECOND;
    }
#endif

#if MWTIMER_MICRO_SECOND == 1000000
    /** 如果支持微秒定时器 */
    else if (timeunit == mwt_timeunit_usec) {
        mulwheeltimer.value.it_value.tv_sec = delay / MWTIMER_MICRO_SECOND;
        mulwheeltimer.value.it_value.tv_usec = (delay % MWTIMER_MICRO_SECOND) * MWTIMER_MICRO_SECOND;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mulwheeltimer.value.it_interval.tv_sec = timeintval / MWTIMER_MICRO_SECOND;
        mulwheeltimer.value.it_interval.tv_usec = (timeintval % MWTIMER_MICRO_SECOND) * MWTIMER_MICRO_SECOND;
    }
#endif

    else {
#ifdef MWT_OS_LINUX
        signal(SIGALRM, mulwheeltimer.old_sigalarm);
#endif
        threadlock_fini(&mulwheeltimer.lock);

    #if MWTIMER_PRINTF == 1
        printf("[mwt:error] invalid timeunit: %d.", timeunit);
    #endif

        return (-1);
    }

    /** 时间单位：秒，毫秒，微秒 */
    mulwheeltimer.timeunit_type = timeunit;

    /** 自动转化为微妙的时间单元 */
    mulwheeltimer.timeunit_usec = mulwheeltimer.value.it_interval.tv_usec + mulwheeltimer.value.it_interval.tv_sec * 1000000;

#if MWTIMER_PRINTF == 1
    printf("[] timeunit=%lld microseconds.\n", (long long) mulwheeltimer.timeunit_usec);
#endif

#ifdef MWT_OS_LINUX
    if (start) {
#endif

#ifdef MWT_OS_WINDOWS
    if (1) {
#endif
        err = -1;

    #ifdef MWT_OS_LINUX
        err = setitimer(ITIMER_REAL, &mulwheeltimer.value, &mulwheeltimer.ovalue);
    #endif

    #ifdef MWT_OS_WINDOWS
        do {
            DWORD delay_ms = (DWORD) (mulwheeltimer.value.it_value.tv_sec * 1000 + mulwheeltimer.value.it_value.tv_usec / 1000);
            DWORD interval_ms = (DWORD) (mulwheeltimer.value.it_interval.tv_sec * 1000 + mulwheeltimer.value.it_interval.tv_usec / 1000);

            HANDLE hTimer;

            // Create the timer queue
            HANDLE hTimerQueue = CreateTimerQueue();

            if (! hTimerQueue) {
                printf("CreateTimerQueue failed (%d)\n", GetLastError());

                threadlock_fini(&mulwheeltimer.lock);

                return (-1);
            }

            // Set a timer to call the timer routine in 10 seconds.
            if (! CreateTimerQueueTimer(&hTimer, hTimerQueue,
                    (WAITORTIMERCALLBACK) win_sigalrm_handler,
                    timerParameter,
                    (DWORD) delay_ms,
                    (DWORD) interval_ms,
                    WT_EXECUTEDEFAULT)) {
                printf("CreateTimerQueueTimer failed (%d)\n", GetLastError());
                err = -1;
            } else {
                // Success
                mulwheeltimer.hTimerQueue = hTimerQueue;
                mulwheeltimer.lpParameter = timerParameter;
                err = 0;
            }
        } while(0);
    #endif

        if (! err) {
            /** 定时器成功创建并启动 */
            mulwheeltimer.status = 1;

            threadlock_leave(&mulwheeltimer.lock);
        #if MWTIMER_PRINTF == 1
            printf("[mwt:info] mul_wheel_timer_init success.\n");
        #endif
            return 0;
        } else {
            /** 定时器创建但启动失败 */
            mulwheeltimer.status = 0;

        #ifdef MWT_OS_LINUX
            /** 定时器不可用，自动销毁 */
            signal(SIGALRM, mulwheeltimer.old_sigalarm);
        #endif

        #ifdef MWT_OS_WINDOWS
            DeleteTimerQueue(mulwheeltimer.hTimerQueue);
        #endif

            threadlock_fini(&mulwheeltimer.lock);

        #if MWTIMER_PRINTF == 1
            printf("[mwt:error] mul_wheel_timer_init failed. setitimer error(%d): %s.\n", err, strerror(err));
        #endif

            return (-1);
        }
    } else {
        /** 定时器成功创建, 但不要求启动 */
        mulwheeltimer.status = 0;
        threadlock_leave(&mulwheeltimer.lock);

    #if MWTIMER_PRINTF == 1
        printf("[mwt:info] mul_wheel_timer_init success without starting.\n");
    #endif

    #ifdef MWT_OS_WINDOWS
        // Windows 不支持随后启动定时器
        #pragma message ("warnings: windows always start timer when init success")
    #endif
        return 0;
    }
}


/**
 * mul_wheel_timer_start
 *   启动定时器，仅对Linux有效。Windows自动启动定时器。
 *
 * returns:
 *    0: success  成功启动
 *    1: sfalse   已经启动
 *   -1: error    启动失败
 */
__disable_warning_unused_static
int mul_wheel_timer_start (void)
{
#ifdef MWT_OS_WINDOWS
    #pragma message("windows always return success(0) for this.")
    return 0;
#endif

#ifdef MWT_OS_LINUX
    int err;

    err = threadlock_tryenter(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MWTIMER_PRINTF == 1
        printf("[mwt:error(%d)] threadlock_tryenter: %s\n", err, strerror(err));
    #endif
        return (-1);
    }

    if (mulwheeltimer.status == 1) {
        /** 已经启动 */
        threadlock_leave(&mulwheeltimer.lock);
    #if MWTIMER_PRINTF == 1
        printf("[mwt:warn] already start.\n");
    #endif
        return 1;
    }

    err = setitimer(ITIMER_REAL, &mulwheeltimer.value, &mulwheeltimer.ovalue);
    if (! err) {
        /** 定时器成功启动 */
        mulwheeltimer.status = 1;
        threadlock_leave(&mulwheeltimer.lock);
    #if MWTIMER_PRINTF == 1
        printf("[mwt:info] mul_wheel_timer_start success.\n");
    #endif
        return 0;
    } else {
        /** 定时器启动失败 */
        mulwheeltimer.status = 0;
        threadlock_leave(&mulwheeltimer.lock);
    #if MWTIMER_PRINTF == 1
        printf("[mwt:error] mul_wheel_timer_start. setitimer error(%d): %s.\n", err, strerror(err));
    #endif
        return (-1);
    }
#endif
}


/**
 * mul_wheel_timer_destroy
 *
 * returns:
 *    0: success
 *   -1: failed.
 *      use strerror(errno) for error message.
 */
__disable_warning_unused_static
int mul_wheel_timer_destroy (void)
{
    int err;

    err = threadlock_enter(&mulwheeltimer.lock);
    if (err) {
    #if MWTIMER_PRINTF == 1
        printf("[mwt] threadlock_enter error(%d): %s\n", err, strerror(err));
    #endif
        return (-1);
    }

#ifdef MWT_OS_LINUX
    if ((signal(SIGALRM, mulwheeltimer.new_sigalarm)) == SIG_ERR) {
        threadlock_leave(&mulwheeltimer.lock);
    #if MWTIMER_PRINTF == 1
        printf("[mwt] destroy failed. signal error.\n");
    #endif
        return (-1);
    }

    /** 恢复进程原有的定时器 */
    err = setitimer(ITIMER_REAL, &mulwheeltimer.ovalue, &mulwheeltimer.value);
    if (err < 0) {
        threadlock_leave(&mulwheeltimer.lock);
    #if MWTIMER_PRINTF == 1
        printf("[mwt] destroy failed. setitimer error(%d): %s.\n", errno, strerror(errno));
    #endif
        return (-1);
    }
#endif

#ifdef MWT_OS_WINDOWS
    // Delete all timers in the timer queue.
    if (! DeleteTimerQueue(mulwheeltimer.hTimerQueue)) {
        printf("DeleteTimerQueue failed (%d)\n", GetLastError());
        threadlock_leave(&mulwheeltimer.lock);
        return (-1);
    }
#endif

    /** 清空定时器链表 */
#if MWTIMER_HAS_DLIST == 1
    do {
        struct list_head *list, *node;

        list_for_each_safe(list, node, &mulwheeltimer.dlist) {
            struct mwt_event_t * event = list_entry(list, struct mwt_event_t, i_list);

            hlist_del(&event->i_hash);
            list_del(&event->i_list);

            free_timer_event(event);
        }
    } while(0);
#else
    do {
        int hash;
        struct hlist_node *hp, *hn;

        for (hash = 0; hash <= MWTIMER_HASHLEN_MAX; hash++) {
            hlist_for_each_safe(hp, hn, &mulwheeltimer.hlist[hash]) {
                struct mwt_event_t * event = hlist_entry(hp, struct mwt_event_t, i_hash);

                hlist_del(&event->i_hash);

                free_timer_event(event);
            }
        }
    } while(0);
#endif

    threadlock_fini(&mulwheeltimer.lock);

    bzero(&mulwheeltimer, sizeof(struct mul_wheel_timer_t));

#if MWTIMER_PRINTF == 1
    printf("[mwt] destroy success.\n");
#endif

    return(0);
}


/**
 * mul_wheel_timer_set_event
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
 *      MUL_WHEEL_TIMER_EVENT_INFINITE: 永久激发
 *     > 0 : 次数
 *
 * returns:
 *    > 0: mwt_eventid_t, success
 *    < 0: failed
 */
#define MUL_WHEEL_TIMER_EVENT_ONEOFF       (1)
#define MUL_WHEEL_TIMER_EVENT_INFINITE    (-1)


__disable_warning_unused_static
mwt_eventid_t mul_wheel_timer_set_event (mwt_bigint_t delay, mwt_bigint_t interval, mwt_bigint_t count,
    int (*on_event_cb)(mwt_event_hdl eventhdl, void *eventarg, void *timerarg), void *eventarg)
{
    int err, hash;
    mwt_bigint_t on_counter;
    mwt_event_t *new_event;

    mwt_bigint_t delay_usec = 0;
    mwt_bigint_t interval_usec = 0;

    if (count == MUL_WHEEL_TIMER_EVENT_INFINITE) {
        count = INT64_MAX;
    }

    if (delay < 0 || interval < 0 || count <= 0) {
        /** 无效的定时器 */
        return (-2);
    }

    if (mulwheeltimer.timeunit_type == mwt_timeunit_sec) {
        delay_usec = delay * 1000000;
        interval_usec = interval * 1000000;
    }

#if MWTIMER_MILLI_SECOND == 1000
    else if (mulwheeltimer.timeunit_type == mwt_timeunit_msec) {
        delay_usec = delay * MWTIMER_MILLI_SECOND;
        interval_usec = interval * MWTIMER_MILLI_SECOND;
    }
#endif

#if MWTIMER_MICRO_SECOND == 1000000
    else if (mulwheeltimer.timeunit_type == mwt_timeunit_usec) {
        delay_usec = delay;
        interval_usec = interval;
    }
#endif

    if (delay_usec == 0 && interval_usec == 0) {
        /** 无效的定时器 */
        return (-2);
    }

    err = threadlock_tryenter(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MWTIMER_PRINTF == 1
        printf("[mwt] threadlock_tryenter error(%d): %s\n", err, strerror(err));
    #endif
        return (-1);
    }

    /**
     * 当 mulwheeltimer.counter == on_counter 时激发
     * 因此设置以 on_counter 为 hash 键保存 event
     */
    on_counter = mwt_hash_on_counter(&mulwheeltimer, delay_usec, &hash);

    new_event = (mwt_event_t *) malloc(sizeof(mwt_event_t));
    if (! new_event) {
        /** out of memory */
        threadlock_leave(&mulwheeltimer.lock);
        return (-4);
    }

    bzero(new_event, sizeof(mwt_event_t));

    new_event->eventid = mulwheeltimer.eventid++;

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

#if MWTIMER_PRINTF == 1
    printf("\033[31m+create event_%lld\033[0m\n", (long long) new_event->eventid);

    printf("\033[31m+create event_%lld. (%d : %d)\033[0m\n", (long long) new_event->eventid,
        (int) new_event->value.it_value.tv_sec, (int) new_event->value.it_interval.tv_sec);
#endif

#if MWTIMER_HAS_DLIST == 1
    /** 串入长串 */
    list_add(&new_event->i_list, &mulwheeltimer.dlist);
#endif

    /** 串入HASH短串 */
    hlist_add_head(&new_event->i_hash, &mulwheeltimer.hlist[hash]);

    /** 设置引用计数为 1 */
    new_event->refc = count;

#if MWTIMER_PRINTF == 1
    // 演示如何删除自身：
    ////hlist_del(&new_event->i_hash);
    ////list_del(&new_event->i_list);
    ////free_timer_event(new_event);
#endif

    threadlock_leave(&mulwheeltimer.lock);

    return new_event->eventid;
}


/**
 * mul_wheel_timer_remove_event
 *   从多定时器中删除事件。该调用仅仅标记事件要删除。真正删除的行为由系统决定。
 *
 * returns:
 *    0: success
 *   -1: failed.
 */
__disable_warning_unused_static
int mul_wheel_timer_remove_event (mwt_event_hdl eventhdl)
{
    mwt_event_t * event = mul_handle_cast_event(eventhdl);

#if MWTIMER_PRINTF == 1
    printf("remove event-%lld\n", (long long) event->eventid);
#endif

    /** 设置引用计数为 0，当有事件触发时自动删除 */
    __interlock_release(&event->refc);

    return 0;
}


/**
 * mul_wheel_timer_fire_event
 *   根据当前激发的 counter 查找 event
 *
 * params:
 *   on_counter - 当前激发的计数器
 *
 * returns:
 *   number of events have been fired
 */
__disable_warning_unused_static
int mul_wheel_timer_fire_event (mwt_bigint_t fire_counter, void * lpParamter)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int num_events_fired = 0;

    int hash = (int) (fire_counter & MWTIMER_HASHLEN_MAX);

    hlist_for_each_safe(hp, hn, &mulwheeltimer.hlist[hash]) {
        struct mwt_event_t * event = hlist_entry(hp, struct mwt_event_t, i_hash);

        if (event->on_counter == fire_counter) {
            /** 首先从链表中删除自己 */
            hlist_del(&event->i_hash);

        #if MWTIMER_HAS_DLIST == 1
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
                    event->on_counter = mwt_hash_on_counter(&mulwheeltimer,
                        event->value.it_interval.tv_sec * 1000000 + event->value.it_interval.tv_usec,
                        &event->hash);

                #if MWTIMER_HAS_DLIST == 1
                    /** 串入长串 */
                    list_add(&event->i_list, &mulwheeltimer.dlist);
                #endif
                    /** 串入HASH短串 */
                    hlist_add_head(&event->i_hash, &mulwheeltimer.hlist[event->hash]);
                }
            }
        }
    }

    return num_events_fired;
}


#ifdef MWT_OS_LINUX
__disable_warning_unused_static
void sigalarm_handler(int signo)
{
    int err;
    mwt_bigint_t on_counter;

    err = threadlock_tryenter(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
    #if MWTIMER_PRINTF == 1
        printf("[mwt]\033[33m lock error: %llu\033[0m\n", (unsigned long long) mulwheeltimer.counter);
    #endif
        return;
    }

    /** 当前激发的计数器 */
    on_counter = mulwheeltimer.counter++;

    /**
     * 激发事件：此事件在锁定状态下调用用户提供的回调函数：
     *   on_timer_event
     *
     * !! 因此不可以在on_timer_event中执行长时间的操作 !!
     */
    mul_wheel_timer_fire_event(on_counter, mulwheeltimer.lpParameter);

    threadlock_leave(&mulwheeltimer.lock);

#if MWTIMER_PRINTF == 1
    printf(" * alarm: %llu\n", (unsigned long long) on_counter);
#endif
}
#endif


#ifdef MWT_OS_WINDOWS

void __stdcall win_sigalrm_handler (PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    if (TimerOrWaitFired) {
        // TimerOrWaitFired:
        //   If this parameter is TRUE, the wait timed out.
        //   If this parameter is FALSE, the wait event has been signaled.
        // This parameter is always TRUE for timer callbacks.

        int err;
        mwt_bigint_t on_counter;

        // The wait timed out
        err = threadlock_tryenter(&mulwheeltimer.lock);
        if (err) {
            /** 多线程锁定失败 */
        #if MWTIMER_PRINTF == 1
            printf("[mwt]\033[33m lock error: %llu\033[0m\n", (unsigned long long) mulwheeltimer.counter);
        #endif
            return;
        }

        /** 当前激发的计数器 */
        on_counter = mulwheeltimer.counter++;

        /**
         * 激发事件：此事件在锁定状态下调用用户提供的回调函数：
         *   on_timer_event
         *
         * !! 因此不可以在on_timer_event中执行长时间的操作 !!
         */
        mul_wheel_timer_fire_event(on_counter, lpParameter);

        threadlock_leave(&mulwheeltimer.lock);

    #if MWTIMER_PRINTF == 1
        printf(" * alarm: %llu\n", (unsigned long long) on_counter);
    #endif

        printf(" * alarm: %lld\n", on_counter);
    }
}


#endif


#if defined(__cplusplus)
}
#endif

#endif /* MUL_WHEEL_TIMER_H_INCLUDED */
