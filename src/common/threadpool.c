/*
 * 2014-06-18: last modified by cheungmine
 *
 * Copyright (c) 2011, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file threadpool.c
 * @brief Threadpool implementation file
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "threadpool.h"


/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var shutdown     Flag indicating if the pool is shutting down
 */
struct threadpool_t {
    pthread_mutex_t lock;
    pthread_cond_t notify;

    int head;
    int tail;
    int count;
    int shutdown;
    int started;

    int thread_count;
    int queue_size;

    threadpool_task_t *queues;
    thread_context_t thread_ctxs[0];
};

#define pool_count_get(pool)  __sync_add_and_fetch(&pool->count, 0)

#define pool_count_add(pool)  __sync_add_and_fetch(&pool->count, 1)

#define pool_count_sub(pool)  __sync_sub_and_fetch(&pool->count, 1)


/**
 * @function void *threadpool_run(void *threadpool)
 * @brief the worker thread
 * @param threadpool the pool which own the thread
 */
static void *threadpool_run (void *threadpool);

int threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(int thread_count, int queue_size, void **thread_args, int flags)
{
    int i;
    threadpool_t *pool = NULL;

    /* Check thread_count for negative or otherwise very big input parameters */
    if (thread_count < 0 || thread_count > POOL_MAX_THREADS) {
        goto err;
    }
    if (thread_count == 0) {
        thread_count = POOL_DEFAULT_THREADS;
    }

    /* Check queue_size for negative or otherwise very big input parameters */
    if (queue_size < 0 || queue_size > POOL_MAX_QUEUES) {
        goto err;
    }
    if (queue_size == 0) {
        queue_size = POOL_DEFAULT_QUEUES;
    }

    /* create threadpool */
    if ( (pool = (threadpool_t *) malloc (sizeof(threadpool_t) +
        sizeof(thread_context_t) * thread_count +
        sizeof(threadpool_task_t) * queue_size)) == NULL ) {
        goto err;
    }

    /* Initialize */
    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    pool->queues = (threadpool_task_t *) (& pool->thread_ctxs[thread_count]);

    /* Initialize mutex and conditional variable first */
    if ((pthread_mutex_init (&(pool->lock), NULL) != 0) ||
       (pthread_cond_init (&(pool->notify), NULL) != 0)) {
        goto err;
    }

    /* Start worker threads */
    for (i = 0; i < thread_count; i++) {
        thread_context_t * pctx = & pool->thread_ctxs[i];

        /* set thread id: 1 based */
        pctx->id = i+1;

        /* set pool to each thread context */
        pctx->pool = (void*) pool;

        /* assign thread argument if valid */
        if (thread_args) {
            pctx->thread_arg = thread_args[i];
        } else {
            pctx->thread_arg = 0;
        }

        if ( pthread_create (& pctx->thread, NULL, threadpool_run, (void*) pctx) != 0) {
            threadpool_destroy (pool, 0);
            return NULL;
        } else {
            pool->started++;
        }
    }

    return pool;

 err:
    if(pool) {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_add (threadpool_t *pool, void (*function)(thread_context_t *), void *task_arg, int flags)
{
    int err = 0;
    int next;

    if ( pool == NULL || function == NULL ) {
        return threadpool_invalid;
    }

    if (pthread_mutex_lock (&(pool->lock)) != 0) {
        return threadpool_lock_failure;
    }

    next = pool->tail + 1;
    next = (next == pool->queue_size) ? 0 : next;

    do {
        /* Are we shutting down ? */
        if (pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        /* Are we full ? */
        if (pool_count_get(pool) == pool->queue_size) {
            err = threadpool_queue_full;
            break;
        }

        /* Add task to queue */
        pool->queues[pool->tail].function = function;
        pool->queues[pool->tail].argument = task_arg;
        pool->queues[pool->tail].flags = flags;

        pool->tail = next;

        /* pool->count += 1; */
        pool_count_add(pool);

        /* pthread_cond_broadcast */
        if (pthread_cond_signal (&(pool->notify)) != 0) {
            err = threadpool_lock_failure;
            break;
        }
    } while(0);

    if (pthread_mutex_unlock (&pool->lock) != 0) {
        err = threadpool_lock_failure;
    }

    return err;
}


int threadpool_unused_queues (threadpool_t *pool)
{
    if ( !pool || pool->shutdown ) {
        return threadpool_invalid;
    } else {
        return (pool->queue_size - pool_count_get(pool));
    }
}


int threadpool_destroy (threadpool_t *pool, int flags)
{
    int i, err = 0;

    if (pool == NULL) {
        return threadpool_invalid;
    }

    if (pthread_mutex_lock (&(pool->lock)) != 0) {
        return threadpool_lock_failure;
    }

    do {
        /* Already shutting down */
        if (pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = 1;

        /* Wake up all worker threads */
        if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = threadpool_lock_failure;
            break;
        }

        /* Join all worker thread */
        for (i = 0; i < pool->thread_count; i++) {
            if (pthread_join (pool->thread_ctxs[i].thread, NULL) != 0) {
                err = threadpool_run_failure;
            }
        }
    } while(0);

    if (pthread_mutex_unlock (&pool->lock) != 0) {
        err = threadpool_lock_failure;
    }

    /* Only if everything went well do we deallocate the pool */
    if (!err) {
        threadpool_free (pool);
    }
    return err;
}

int threadpool_free (threadpool_t *pool)
{
    if (pool == NULL || pool->started > 0) {
        return -1;
    }

    pthread_mutex_lock (&(pool->lock));
    pthread_mutex_destroy (&(pool->lock));
    pthread_cond_destroy (&(pool->notify));

    free(pool);
    return 0;
}

/**
 * each thread run function
 */
static void *threadpool_run (void * param)
{
    threadpool_task_t task;

    thread_context_t * thread_ctx = (thread_context_t *) param;
    threadpool_t * pool = thread_ctx->pool;

    for ( ; ; ) {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock (&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while ((pool_count_get(pool) == 0) && (!pool->shutdown)) {
            pthread_cond_wait (&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown) {
            break;
        }

        /* Grab our task */
        task.function = pool->queues[pool->head].function;
        task.argument = pool->queues[pool->head].argument;
        task.flags    = pool->queues[pool->head].flags;

        thread_ctx->task = &task;

        pool->head += 1;
        pool->head = (pool->head == pool->queue_size) ? 0 : pool->head;

        /* pool->count -= 1; */
        pool_count_sub(pool);

        /* Unlock */
        pthread_mutex_unlock (&(pool->lock));

        /* Get to work */
        (*(task.function)) (thread_ctx);
    }

    pool->started--;

    pthread_mutex_unlock (&(pool->lock));
    pthread_exit (NULL);
    return (NULL);
}
