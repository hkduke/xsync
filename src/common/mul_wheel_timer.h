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

/**
 * mul_wheel_timer.h
 *   multi wheel timer for linux
 *
 *   秒级多定时器的工业级实现
 *
 * refer:
 *   http://blog.csdn.net/zhangxinrun/article/details/5914191
 *   https://linux.die.net/man/2/setitimer
 *
 * author: master@pepstack.com
 *
 * create: 2018-02-03 11:00
 * update: 2018-02-03 22:09
 */
#ifndef MUL_WHEEL_TIMER_H_INCLUDED
#define MUL_WHEEL_TIMER_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include "dhlist.h"

#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>


/**
 * 是否使用双链表：如果不需要按次序遍历定时器不要启用它
 *
 * MUL_WHEEL_TIMER_HAS_DLIST == 1
 *   use dlist for traversing by sequence
 *
 * MUL_WHEEL_TIMER_HAS_DLIST == 0
 *   DONOT use dlist since we need not traversing by sequence
 */
#define MUL_WHEEL_TIMER_HAS_DLIST   0


/**
 * MUL_WHEEL_TIMER_HASHLEN_MAX
 *
 * 定义 hash 桶的大小 (2^n -1) = [255, 1023, 4095, 8191]
 * 可以根据需要在编译时指定。桶越大，查找速度越快。
 */
#ifndef MUL_WHEEL_TIMER_HASHLEN_MAX
#  define MUL_WHEEL_TIMER_HASHLEN_MAX     1023
#endif


typedef enum
{
    mwt_timeunit_sec =  0,
    mwt_timeunit_msec = 1,
    mwt_timeunit_usec = 2
} mwt_timeunit_t;


typedef int64_t mul_eventid_t;

typedef mul_eventid_t * mul_event_handle;


typedef struct mul_timer_event_t
{
    mul_eventid_t eventid;

    /* 引用计数： 0 删除, 1 保留 */
    int64_t refc;

    void *eventarg;
    int (*timer_event_cb) (mul_event_handle eventhdl, void *eventarg);

    int64_t on_counter;

    /* 指定定时器首次激发时间和以后每次间隔激发时间 */
    struct itimerval value;

    int hash;

    /** dhlist node */
#if MUL_WHEEL_TIMER_HAS_DLIST == 1
    struct list_head  i_list;
#endif
    struct hlist_node i_hash;
} mul_timer_event_t;


typedef struct mul_wheel_timer_t
{
    pthread_mutex_t lock;

    /**
     * 定时器状态:
     *    1: 启动
     *    0: 暂停
     */
    int status;

    volatile mul_eventid_t eventid;

    volatile int64_t counter;

    void (* old_sigalarm)(int);
    void (* new_sigalarm)(int);

    unsigned int timeunit;

    /** 最小时间单元值: 微秒 */
    int64_t timeunit_usec;

    mwt_timeunit_t  timeunit_type;

    struct itimerval value, ovalue;

    /** dhlist for timer entry */
#if MUL_WHEEL_TIMER_HAS_DLIST == 1
    struct list_head dlist;
#endif
    struct hlist_head hlist[MUL_WHEEL_TIMER_HASHLEN_MAX + 1];
} mul_wheel_timer_t;


/* global timer variable for one linux process */
__attribute__((used))
static struct mul_wheel_timer_t  mulwheeltimer;


__attribute__((used))
static void sigalarm_handler(int signo);


__attribute__((used))
static inline void free_timer_event (struct mul_timer_event_t * event)
{
    printf("\033[32m-delete event_%llu\033[0m\n", (unsigned long long) event->eventid);
    free(event);
}


__attribute__((used))
static inline mul_timer_event_t * mul_handle_cast_event (mul_event_handle eventhdl)
{
    struct mul_timer_event_t * event = CONTAINER_OF(eventhdl, struct mul_timer_event_t, eventid);
    return event;
}


__attribute__((used))
static inline int64_t mwt_hash_on_counter (mul_wheel_timer_t * mwt, int64_t timeval_usec, int *hash)
{
    int64_t on_counter = (int64_t) (timeval_usec / mwt->timeunit_usec + mwt->counter);
    *hash = (on_counter & MUL_WHEEL_TIMER_HASHLEN_MAX);

    //printf(" * \033[36m on_counter=%" PRI64d ", hash=%d\033[0m\n", on_counter, (*hash));
    return on_counter;
}


__attribute__((used))
static int timer_select_sleep (int sec, int ms)
{
    if (sec || ms) {
        struct timeval tv = {0};

        tv.tv_sec = sec;
        tv.tv_usec = ms * 1000;

        return select (0, NULL, NULL, NULL, &tv);
    } else {
        return 0;
    }
}


__attribute__((used))
static int mul_wheel_timer_init (mwt_timeunit_t timeunit, unsigned int timeintval, unsigned int delay, int start)
{
    int i, err;

    bzero(&mulwheeltimer, sizeof(struct mul_wheel_timer_t));

    mulwheeltimer.eventid = 1;

#if MUL_WHEEL_TIMER_HAS_DLIST == 1
    INIT_LIST_HEAD(&mulwheeltimer.dlist);
#endif
    for (i = 0; i <= MUL_WHEEL_TIMER_HASHLEN_MAX; i++) {
        INIT_HLIST_HEAD(&mulwheeltimer.hlist[i]);
    }

    err = pthread_mutex_init(&mulwheeltimer.lock, 0);
    if (err) {
        /* nerver run to this ! */
        printf("[mwt:error(%d)] pthread_mutex_init: %s.\n", err, strerror(err));
        return (-1);
    }

    err = pthread_mutex_lock(&mulwheeltimer.lock);
    if (err) {
        printf("[mwt:error(%d)] pthread_mutex_lock: %s.\n", err, strerror(err));
        return (-1);
    }

    if ( (mulwheeltimer.old_sigalarm = signal(SIGALRM, sigalarm_handler)) == SIG_ERR ) {
        pthread_mutex_destroy(&mulwheeltimer.lock);
        printf("[mwt:error(%d)] signal: %s.\n", errno, strerror(errno));
        return (-1);
    }

    mulwheeltimer.new_sigalarm = sigalarm_handler;

    if (timeunit == mwt_timeunit_sec) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mulwheeltimer.value.it_value.tv_sec = delay;
        mulwheeltimer.value.it_value.tv_usec = 0;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mulwheeltimer.value.it_interval.tv_sec = timeintval;
        mulwheeltimer.value.it_interval.tv_usec = 0;
    } else if (timeunit == mwt_timeunit_msec) {
        /** 定义首次激发延迟时间: setitimer 之后 timeval_delay 首次激发 */
        mulwheeltimer.value.it_value.tv_sec = delay / 1000;
        mulwheeltimer.value.it_value.tv_usec = (delay % 1000) * 1000;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mulwheeltimer.value.it_interval.tv_sec = timeintval / 1000;
        mulwheeltimer.value.it_interval.tv_usec = (timeintval % 1000) * 1000;
    } else if (timeunit == mwt_timeunit_usec) {
        mulwheeltimer.value.it_value.tv_sec = delay / 1000000;
        mulwheeltimer.value.it_value.tv_usec = (delay % 1000000) * 1000000;

        /** 定义间隔激发时间: 首次激发之后每隔 timeval_interval 激发 */
        mulwheeltimer.value.it_interval.tv_sec = timeintval / 1000000;
        mulwheeltimer.value.it_interval.tv_usec = (timeintval % 1000000) * 1000000;
    } else {
        signal(SIGALRM, mulwheeltimer.old_sigalarm);
        pthread_mutex_destroy(&mulwheeltimer.lock);

        printf("[mwt:error] invalid timeunit: %d.", timeunit);
        return (-1);
    }

    /** 时间单位：秒，毫秒，微秒 */
    mulwheeltimer.timeunit_type = timeunit;

    /** 自动转化为微妙的时间单元 */
    mulwheeltimer.timeunit_usec = mulwheeltimer.value.it_interval.tv_usec + mulwheeltimer.value.it_interval.tv_sec * 1000000;

    printf("[] timeunit=%" PRId64 " microseconds.\n", mulwheeltimer.timeunit_usec);

    if (start) {
        err = setitimer(ITIMER_REAL, &mulwheeltimer.value, &mulwheeltimer.ovalue);
        if (! err) {
            /** 定时器成功创建并启动 */
            mulwheeltimer.status = 1;

            pthread_mutex_unlock(&mulwheeltimer.lock);

            printf("[mwt:info] mul_wheel_timer_init success.\n");

            return 0;
        } else {
            /** 定时器创建但启动失败 */
            mulwheeltimer.status = 0;

            /** 定时器不可用，自动销毁 */
            signal(SIGALRM, mulwheeltimer.old_sigalarm);
            pthread_mutex_destroy(&mulwheeltimer.lock);

            printf("[mwt:error] mul_wheel_timer_init failed. setitimer error(%d): %s.\n", err, strerror(err));

            return (-1);
        }
    } else {
        /** 定时器成功创建, 但不要求启动 */
        mulwheeltimer.status = 0;
        pthread_mutex_unlock(&mulwheeltimer.lock);

        printf("[mwt:info] mul_wheel_timer_init success without starting.\n");

        return 0;
    }
}


/**
 * mul_wheel_timer_start
 *   启动定时器
 *
 * returns:
 *    0: success  成功启动
 *    1: sfalse   已经启动
 *   -1: error    启动失败
 */
__attribute__((used))
static int mul_wheel_timer_start (void)
{
    int err;

    err = pthread_mutex_trylock(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
        printf("[mwt:error(%d)] pthread_mutex_trylock: %s\n", err, strerror(err));
        return (-1);
    }

    if (mulwheeltimer.status == 1) {
        /** 已经启动 */
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt:warn] already start.\n");
        return 1;
    }

    err = setitimer(ITIMER_REAL, &mulwheeltimer.value, &mulwheeltimer.ovalue);
    if (! err) {
        /** 定时器成功启动 */
        mulwheeltimer.status = 1;
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt:info] mul_wheel_timer_start success.\n");
        return 0;
    } else {
        /** 定时器启动失败 */
        mulwheeltimer.status = 0;
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt:error] mul_wheel_timer_start. setitimer error(%d): %s.\n", err, strerror(err));
        return (-1);
    }
}


/**
 * mul_wheel_timer_destroy
 *
 * returns:
 *    0: success
 *   -1: failed.
 *      use strerror(errno) for error message.
 */
__attribute__((used))
static int mul_wheel_timer_destroy (void)
{
    int err;

    err = pthread_mutex_lock(&mulwheeltimer.lock);
    if (err) {
        printf("[mwt] pthread_mutex_lock error(%d): %s\n", err, strerror(err));
        return (-1);
    }

    if ((signal(SIGALRM, mulwheeltimer.new_sigalarm)) == SIG_ERR) {
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt] destroy failed. signal error.\n");
        return (-1);
    }

    /** 恢复进程原有的定时器 */
    err = setitimer(ITIMER_REAL, &mulwheeltimer.ovalue, &mulwheeltimer.value);
    if (err < 0) {
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt] destroy failed. setitimer error(%d): %s.\n", errno, strerror(errno));
        return (-1);
    }

    /** 清空定时器链表 */
#if MUL_WHEEL_TIMER_HAS_DLIST == 1
    do {
        struct list_head *list, *node;

        list_for_each_safe(list, node, &mulwheeltimer.dlist) {
            struct mul_timer_event_t * event = list_entry(list, struct mul_timer_event_t, i_list);

            hlist_del(&event->i_hash);
            list_del(&event->i_list);

            free_timer_event(event);
        }
    } while(0);
#else
    do {
        int hash;
        struct hlist_node *hp, *hn;

        for (hash = 0; hash <= MUL_WHEEL_TIMER_HASHLEN_MAX; hash++) {
            hlist_for_each_safe(hp, hn, &mulwheeltimer.hlist[hash]) {
                struct mul_timer_event_t * event = hlist_entry(hp, struct mul_timer_event_t, i_hash);

                hlist_del(&event->i_hash);

                free_timer_event(event);
            }
        }
    } while(0);
#endif

    pthread_mutex_destroy(&mulwheeltimer.lock);

    bzero(&mulwheeltimer, sizeof(struct mul_wheel_timer_t));

    printf("[mwt] destroy success.\n");
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
 *    > 0: mul_eventid_t, success
 *    < 0: failed
 */
#define MUL_WHEEL_TIMER_EVENT_ONEOFF       (1)
#define MUL_WHEEL_TIMER_EVENT_INFINITE    (-1)


__attribute__((used))
static mul_eventid_t mul_wheel_timer_set_event (int64_t delay, int64_t interval, int64_t count,
    int (*on_event_cb)(mul_event_handle eventhdl, void *eventarg), void *eventarg)
{
    int err, hash;
    int64_t on_counter;
    mul_timer_event_t *new_event;

    int64_t delay_usec = 0;
    int64_t interval_usec = 0;

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
    } else if (mulwheeltimer.timeunit_type == mwt_timeunit_msec) {
        delay_usec = delay * 1000;
        interval_usec = interval * 1000;
    } else if (mulwheeltimer.timeunit_type == mwt_timeunit_usec) {
        delay_usec = delay;
        interval_usec = interval;
    }

    if (delay_usec == 0 && interval_usec == 0) {
        /** 无效的定时器 */
        return (-2);
    }

    err = pthread_mutex_trylock(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
        printf("[mwt] pthread_mutex_trylock error(%d): %s\n", err, strerror(err));
        return (-1);
    }

    /**
     * 当 mulwheeltimer.counter == on_counter 时激发
     * 因此设置以 on_counter 为 hash 键保存 event
     */
    on_counter = mwt_hash_on_counter(&mulwheeltimer, delay_usec, &hash);

    new_event = (mul_timer_event_t *) malloc(sizeof(mul_timer_event_t));
    if (! new_event) {
        /** out of memory */
        pthread_mutex_unlock(&mulwheeltimer.lock);
        return (-4);
    }

    bzero(new_event, sizeof(mul_timer_event_t));

    new_event->eventid = mulwheeltimer.eventid++;

    new_event->on_counter = on_counter;

    /** 首次激发时间 */
    new_event->value.it_value.tv_sec = delay_usec / 1000000;
    new_event->value.it_value.tv_usec = delay_usec % 1000000;

    /** 间隔激发时间 */
    new_event->value.it_interval.tv_sec = interval_usec / 1000000;
    new_event->value.it_interval.tv_usec = interval_usec % 1000000;

    new_event->hash = hash;

    /** 设置回调参数和函数，回调函数由用户自己实现 */
    new_event->eventarg = eventarg;
    new_event->timer_event_cb = on_event_cb;

    //printf("\033[31m+create event_%" PRI64d "\033[0m\n", new_event->eventid);

    //printf("\033[31m+create event_%" PRI64d ". (%d : %d)\033[0m\n", new_event->eventid,
    //    (int) new_event->value.it_value.tv_sec, (int) new_event->value.it_interval.tv_sec);

#if MUL_WHEEL_TIMER_HAS_DLIST == 1
    /** 串入长串 */
    list_add(&new_event->i_list, &mulwheeltimer.dlist);
#endif

    /** 串入HASH短串 */
    hlist_add_head(&new_event->i_hash, &mulwheeltimer.hlist[hash]);

    /** 设置引用计数为 1 */
    new_event->refc = count;

    // 演示如何删除自身：
    ////hlist_del(&new_event->i_hash);
    ////list_del(&new_event->i_list);
    ////free_timer_event(new_event);

    pthread_mutex_unlock(&mulwheeltimer.lock);

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
__attribute__((used))
static int mul_wheel_timer_remove_event (mul_event_handle eventhdl)
{
    mul_timer_event_t * event = mul_handle_cast_event(eventhdl);

    printf("remove event-%lld\n", (long long) event->eventid);

    /** 设置引用计数为 0，当有事件触发时自动删除 */
    __sync_lock_release(&event->refc);

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
__attribute__((used))
static int mul_wheel_timer_fire_event (int64_t fire_counter)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int num_events_fired = 0;

    int hash = (int) (fire_counter & MUL_WHEEL_TIMER_HASHLEN_MAX);

    hlist_for_each_safe(hp, hn, &mulwheeltimer.hlist[hash]) {
        struct mul_timer_event_t * event = hlist_entry(hp, struct mul_timer_event_t, i_hash);

        if (event->on_counter == fire_counter) {
            /** 首先从链表中删除自己 */
            hlist_del(&event->i_hash);

        #if MUL_WHEEL_TIMER_HAS_DLIST == 1
            list_del(&event->i_list);
        #endif

            if (__sync_fetch_and_sub(&event->refc, 1) <= 0) {
                /** 要求删除事件 */
                free_timer_event(event);
            } else {
                /** 激发事件回调函数 */
                event->timer_event_cb(&event->eventid, event->eventarg);

                num_events_fired++;

                if (event->value.it_interval.tv_sec == 0) {
                    /* 只使用一次, 下次不再激发，删除事件 */
                    free_timer_event(event);
                } else {
                    event->on_counter = mwt_hash_on_counter(&mulwheeltimer,
                        event->value.it_interval.tv_sec * 1000000 + event->value.it_interval.tv_usec,
                        &event->hash);

                #if MUL_WHEEL_TIMER_HAS_DLIST == 1
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


__attribute__((used))
static void sigalarm_handler(int signo)
{
    int err;
    int64_t on_counter;

    err = pthread_mutex_trylock(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
        printf("[mwt]\033[33m lock error: %llu\033[0m\n", (unsigned long long) mulwheeltimer.counter);
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
    mul_wheel_timer_fire_event(on_counter);

    pthread_mutex_unlock(&mulwheeltimer.lock);

    printf(" * alarm: %llu\n", (unsigned long long) on_counter);
}


#if defined(__cplusplus)
}
#endif

#endif /* MUL_WHEEL_TIMER_H_INCLUDED */
