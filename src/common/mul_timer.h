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

/* 激发 1 次 */
#define MULTIMER_EVENT_ONCEOFF     ((bigint_t)(1))

/* 永远激发 */
#define MULTIMER_EVENT_INFINITE    ((bigint_t)(-1))

#define MULTIMER_EVENT_COUNT(n)    \
    ((bigint_t) ( (n) < MULTIMER_EVENT_INFINITE ? MULTIMER_EVENT_INFINITE : ((n) == 0 ? MULTIMER_EVENT_ONCEOFF : (n)) ))

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

    void (*old_sighdl)(int);
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
inline int timer_select_sleep (int sec, int ms)
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
inline int mul_timer_fire_event (mul_timer_t *mtr, bigint_t fire_counter)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int num_events_fired = 0;

    int hash = (int) (fire_counter & MULTIMER_HASHLEN_MAX);

    hlist_for_each_safe(hp, hn, &mtr->hlist[hash]) {
        struct mul_event_t * event = hlist_entry(hp, struct mul_event_t, i_hash);

        if (event->on_counter == fire_counter) {
            bigint_t  refc;

            /** 首先从链表中删除自己 */
            hlist_del(&event->i_hash);

        #if MULTIMER_HAS_DLIST == 1
            list_del(&event->i_list);
        #endif

            refc = __interlock_sub(&event->refc);
            if (refc >= 0) {
                /** 激发事件回调函数 */
                event->timer_event_cb(&event->eventid, event->eventarg, mtr->lpParameter);
                num_events_fired++;
            }

            if (refc <= 0) {
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
    void *timerParameter, int start);


/**
 * mul_timer_start
 *   定时器创建之后已经启用，这里启动定时器实际上只是启用计数器
 */
extern void mul_timer_start ();


/**
 * 暂停定时器：暂停计数器
 */
extern void mul_timer_pause ();


/**
 * mul_timer_destroy
 *
 * returns:
 *    0: success
 *   -1: failed.
 *      use strerror(errno) for error message.
 */
extern int mul_timer_destroy ();


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
 *      MULTIMER_EVENT_ONCEOFF:  只激发1次
 *      MULTIMER_EVENT_INFINITE: 永久激发
 *      MULTIMER_EVENT_COUNT(n): 指定激发次数
 *
 * returns:
 *    > 0: mul_eventid_t, success
 *    < 0: failed
 */
extern mul_eventid_t mul_timer_set_event (bigint_t delay, bigint_t interval, bigint_t count,
    int (*on_event_cb)(mul_event_hdl eventhdl, void *eventarg, void *timerarg), void *eventarg);


/**
 * mul_timer_remove_event
 *   从多定时器中删除事件。该调用仅仅标记事件要删除。真正删除的行为由系统决定。
 *
 * returns:
 *    0: success
 *   -1: failed.
 */
extern int mul_timer_remove_event (mul_event_hdl eventhdl);


#if defined(__cplusplus)
}
#endif

#endif /* MUL_TIMER_H_INCLUDED */
