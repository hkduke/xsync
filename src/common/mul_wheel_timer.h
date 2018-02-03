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


/**
 * MUL_WHEEL_TIMER_TIMEUNIT_SEC
 *
 * 定义多定时器的时间单位：即最小时间间隔。系统将按照这个时间间隔定时激发
 * 定时器回调函数：sigalarm_handler
 */
#ifndef MUL_WHEEL_TIMER_TIMEUNIT_SEC
#  define MUL_WHEEL_TIMER_TIMEUNIT_SEC    1
#endif


#define MUL_WHEEL_TIMER_TIMEUNIT_DEFAULT   (-1)

typedef int64_t mul_eventid_t;

typedef mul_eventid_t * mul_event_handle;


typedef struct mul_timer_event_t
{
    mul_eventid_t eventid;

    /* 引用计数： 0 删除, 1 保留 */
    int refc;

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
static int64_t timer_on_counter_hash (mul_wheel_timer_t * mwtr, unsigned int tv_sec, int *hash)
{
    int64_t on_counter = (int64_t) (tv_sec / mwtr->timeunit + mwtr->counter);
    *hash = (on_counter & MUL_WHEEL_TIMER_HASHLEN_MAX);

    printf(" * \033[36mon_counter=%lld, hash=%d\033[0m\n", (long long) on_counter, (*hash));
    return on_counter;
}


__attribute__((used))
static int timer_select_sleep (int sec, int ms)
{
    if (sec || ms) {
        struct timeval tv = {0};

        tv.tv_sec = sec;
        tv.tv_usec =ms*1000;

        return select (0, NULL, NULL, NULL, &tv);
    } else {
        return 0;
    }
}


/**
 * mul_wheel_timer_create
 *
 *  根据给定值初始化多定时器。本程序不提供秒以下的定时器，如果需要请用户自行扩充。
 *
 * params:
 *   start - 1: 立即启动定时器; 0: 不启动定时器
 *
 *  1) 初始化最小时间单位 (= MUL_WHEEL_TIMER_TIMEUNIT_SEC) 秒的多定时器:
 *
 *      mul_wheel_timer_create(MUL_WHEEL_TIMER_TIMEUNIT_DEFAULT, 0);
 *
 *  2) 初始化最小时间单位 (= 10) 秒的多定时器，且在60秒以后开始启用, 启用
 *     之后每隔 10 秒激发 1 次:
 *
 *      mul_wheel_timer_create(10, 60);
 *
 * returns:
 *    0: success
 *   -1: failed.
 *      use strerror(errno) for error message.
 */
__attribute__((used))
static int mul_wheel_timer_create (long timeunit_sec, long timedelay_sec, int start)
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
        return (-1);
    }

    err = pthread_mutex_lock(&mulwheeltimer.lock);
    if (err) {
        printf("[mwt] pthread_mutex_lock error(%d): %s\n", err, strerror(err));
        return (-1);
    }

    if ( (mulwheeltimer.old_sigalarm = signal(SIGALRM, sigalarm_handler)) == SIG_ERR ) {
        pthread_mutex_destroy(&mulwheeltimer.lock);
        return (-1);
    }

    mulwheeltimer.new_sigalarm = sigalarm_handler;

    /**
     * 设置时间间隔为 it_interval 的定时器。
     * it_interval 应该设置成为后期添加的定时器的最小时间间隔。默认为秒:
     *   MUL_WHEEL_TIMER_TIMEUNIT_SEC
     */
    if (timeunit_sec == MUL_WHEEL_TIMER_TIMEUNIT_DEFAULT) {
        mulwheeltimer.value.it_interval.tv_sec = MUL_WHEEL_TIMER_TIMEUNIT_SEC;
        mulwheeltimer.value.it_interval.tv_usec = 0;
    } else {
        mulwheeltimer.value.it_interval.tv_sec = timeunit_sec;
        mulwheeltimer.value.it_interval.tv_usec = 0;
    }

    /** 设置时间间隔，以秒为单位：本程序不提供秒以下的定时器，如果需要请用户自行扩充！*/
    mulwheeltimer.timeunit = mulwheeltimer.value.it_interval.tv_sec;

    /** 设置到期时间为 it_value 的定时器，一旦时间达到 it_value，内核使用 it_interval 重启定时器 */
    if (timedelay_sec <= 0) {
        mulwheeltimer.value.it_value.tv_sec = mulwheeltimer.value.it_interval.tv_sec;
        mulwheeltimer.value.it_value.tv_usec = 0;
    } else {
        mulwheeltimer.value.it_value.tv_sec = timedelay_sec;
        mulwheeltimer.value.it_value.tv_usec = 0;
    }

    if (start) {
        err = setitimer(ITIMER_REAL, &mulwheeltimer.value, &mulwheeltimer.ovalue);
        if (! err) {
            /** 定时器成功创建并启动 */
            mulwheeltimer.status = 1;
            pthread_mutex_unlock(&mulwheeltimer.lock);
            printf("[mwt] create success.\n");
            return 0;
        } else {
            /** 定时器创建但启动失败 */
            mulwheeltimer.status = 0;
            pthread_mutex_destroy(&mulwheeltimer.lock);
            printf("[mwt] create failed.\n");
            return (-1);
        }
    } else {
        /** 定时器成功创建, 但不要求启动 */
        mulwheeltimer.status = 0;
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt] create success with no start.\n");
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
        printf("[mwt] pthread_mutex_trylock error(%d): %s\n", err, strerror(err));
        return (-1);
    }

    if (mulwheeltimer.status == 1) {
        /** 已经启动 */
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt] already start.\n");
        return 1;
    }

    err = setitimer(ITIMER_REAL, &mulwheeltimer.value, &mulwheeltimer.ovalue);
    if (! err) {
        /** 定时器成功启动 */
        mulwheeltimer.status = 1;
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt] start success.\n");
        return 0;
    } else {
        /** 定时器启动失败 */
        mulwheeltimer.status = 0;
        pthread_mutex_unlock(&mulwheeltimer.lock);
        printf("[mwt] start error.\n");
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
 * params:
 *   timedelay_sec - 指定首次激发的时间：当前定时器首次启动之后多少秒时间激发 event。
 *       0 : 立即激发
 *     > 0 : 延迟时间(秒)
 *
 *   timeinterval_sec - 首次激发 event 之后间隔多少秒激发。
 *       0 : 不激发
 *     > 0 : 间隔多少秒时间激发
 *
 * returns:
 *    > 0: mul_eventid_t, success
 *    < 0: failed
 */
__attribute__((used))
static mul_eventid_t mul_wheel_timer_set_event (unsigned int timedelay_sec, unsigned int timeinterval_sec,
    int (*on_event_cb)(mul_event_handle eventhdl, void *eventarg), void *eventarg)
{
    int err, hash;
    int64_t on_counter;
    mul_timer_event_t *new_event;

    if (timedelay_sec == -1 || timeinterval_sec == -1) {
        /** 无效的定时器 */
        return (-2);
    }

    err = pthread_mutex_trylock(&mulwheeltimer.lock);
    if (err) {
        /** 多线程锁定失败 */
        printf("[mwt] pthread_mutex_trylock error(%d): %s\n", err, strerror(err));
        return (-1);
    }

    /** 当 mulwheeltimer.counter == on_counter 时激发
     * 因此设置以 on_counter 为 hash 键保存 event
     */
    on_counter = timer_on_counter_hash(&mulwheeltimer, timedelay_sec, &hash);

    new_event = (mul_timer_event_t *) malloc(sizeof(mul_timer_event_t));
    if (! new_event) {
        /** out of memory */
        pthread_mutex_unlock(&mulwheeltimer.lock);
        return (-4);
    }

    bzero(new_event, sizeof(mul_timer_event_t));

    new_event->eventid = mulwheeltimer.eventid++;

    new_event->on_counter = on_counter;

    /** 当前时间 */
    new_event->value.it_value.tv_sec = timedelay_sec;

    /** 下次时间 */
    new_event->value.it_interval.tv_sec = timeinterval_sec;

    new_event->hash = hash;

    /** 设置回调参数和函数，回调函数由用户自己实现 */
    new_event->eventarg = eventarg;
    new_event->timer_event_cb = on_event_cb;

    printf("\033[31m+create event_%lld. (%d:%d)\033[0m\n", (long long) new_event->eventid,
        (int) new_event->value.it_value.tv_sec, (int) new_event->value.it_interval.tv_sec);

#if MUL_WHEEL_TIMER_HAS_DLIST == 1
    /** 串入长串 */
    list_add(&new_event->i_list, &mulwheeltimer.dlist);
#endif

    /** 串入HASH短串 */
    hlist_add_head(&new_event->i_hash, &mulwheeltimer.hlist[hash]);

    /** 设置引用计数为 1 */
    new_event->refc = 1;

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
 */
__attribute__((used))
static mul_timer_event_t * mul_wheel_timer_fire_event (int64_t fire_counter)
{
    struct hlist_node *hp;
    struct hlist_node *hn;

    int hash = (int) (fire_counter & MUL_WHEEL_TIMER_HASHLEN_MAX);

    hlist_for_each_safe(hp, hn, &mulwheeltimer.hlist[hash]) {
        struct mul_timer_event_t * event = hlist_entry(hp, struct mul_timer_event_t, i_hash);

        if (event->on_counter == fire_counter) {
            /** 首先从链表中删除自己 */
            hlist_del(&event->i_hash);

        #if MUL_WHEEL_TIMER_HAS_DLIST == 1
            list_del(&event->i_list);
        #endif

            if (__sync_lock_test_and_set(&event->refc, 1) == 0) {
                /** 要求删除事件 */
                free_timer_event(event);
            } else {
                /** 激发事件回调函数 */
                event->timer_event_cb(&event->eventid, event->eventarg);

                if (event->value.it_interval.tv_sec == 0) {
                    /* 只使用一次, 下次不再激发，删除事件 */
                    free_timer_event(event);
                } else {
                    event->on_counter = timer_on_counter_hash(&mulwheeltimer, event->value.it_interval.tv_sec, &event->hash);

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

    return 0;
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
