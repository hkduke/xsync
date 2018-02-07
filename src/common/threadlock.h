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
 * threadlock.h
 *
 * author: master@pepstack.com
 *
 * create: 2018-02-04
 * update: 2018-02-04
 */
#ifndef THREAD_LOCK_H_INCLUDED
#define THREAD_LOCK_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32_WINNT)
    #include <windows.h>
    #include <process.h>
    #include <stdint.h>
    #include <time.h>

    #if _WIN32_WINNT  < 0x0500
        #error  Windows version is too lower than 0x0500
    #endif

    #ifndef _WINDOWS_MSVC
        #define _WINDOWS_MSVC
    #endif

    #ifdef _LINUX_GNUC
        #undef _LINUX_GNUC
    #endif

#elif defined(__GNUC__)

    #if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
        #error GCC version must be greater or equal than 4.1.2
    #endif

    #include <pthread.h>
    #include <unistd.h>
    #include <stdint.h>
    #include <sched.h>
    #include <time.h>
    #include <signal.h>
    #include <errno.h>

    #include <sys/time.h>
    #include <sys/syscall.h>

    #ifndef _LINUX_GNUC
        #define _LINUX_GNUC
    #endif

    #ifdef _WINDOWS_MSVC
        #undef _WINDOWS_MSVC
    #endif
#else
    /* TODO: MACOSX */
    #error Currently only Windows and Linux os are supported.
#endif


/**
 * Windows
 */
#if defined(_WINDOWS_MSVC)

    typedef CRITICAL_SECTION thread_lock_t;

    typedef LONGLONG bigint_t;

    typedef bigint_t volatile ref_counter_t;

    typedef struct itimerval {
        struct timeval it_value;
        struct timeval it_interval;
    } itimerval;

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

    #define threadlock_destroy(lock)    DeleteCriticalSection(lock)

    static inline int threadlock_lock(thread_lock_t *lock)
    {
        EnterCriticalSection(lock);
        return 0;
    }

    #define threadlock_trylock(lock)    (TryEnterCriticalSection(lock)? 0:(-1))
    #define threadlock_unlock(lock)      LeaveCriticalSection(lock)

    /* refcount */
    #define __interlock_add(addr)        InterlockedIncrement64(addr)
    #define __interlock_sub(addr)        InterlockedDecrement64(addr)
    #define __interlock_release(addr)    InterlockedExchange64(addr, 0)

    /**
     * InterlockedCompareExchange64
     *   https://msdn.microsoft.com/en-us/library/windows/desktop/ms683562(v=vs.85).aspx
     */
    #define __interlock_get(addr)            InterlockedCompareExchange64(addr, 0, 0)
    #define __interlock_set(addr, newval)   InterlockedExchange64(addr, newval)

    /* get current thread id */
    #define gettid()  GetCurrentThreadId(void)

    /* get current process id */
    #define getpid()  GetCurrentProcessId(void)

    #define __no_warning_unused(prefix)   prefix
#endif


/**
 * Linux
 *   http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html#Atomic-Builtins
 */
#if defined(_LINUX_GNUC)

    typedef pthread_mutex_t  thread_lock_t;

    typedef int64_t bigint_t;

    typedef bigint_t volatile ref_counter_t;

    /* pthread mutex */
    #define threadlock_init(lock)         pthread_mutex_init(lock, 0)
    #define threadlock_destroy(lock)      pthread_mutex_destroy(lock)

    #define threadlock_lock(lock)         pthread_mutex_lock(lock)
    #define threadlock_trylock(lock)      pthread_mutex_trylock(lock)
    #define threadlock_unlock(lock)       pthread_mutex_unlock(lock)

    /* refcount */
    #define __interlock_add(addr)         __sync_add_and_fetch(addr, 1)
    #define __interlock_sub(addr)         __sync_sub_and_fetch(addr, 1)
    #define __interlock_release(addr)     __sync_lock_release(addr)

    #define __interlock_get(addr)         __sync_fetch_and_add(addr, 0)
    #define __interlock_set(addr, newval)   __sync_lock_test_and_set(addr, newval)


    /* get current thread id */
    #define gettid()    syscall(__NR_gettid)

    #define __no_warning_unused(prefix)    __attribute__((used))  prefix

#endif


#if defined(__cplusplus)
}
#endif

#endif /* THREAD_LOCK_H_INCLUDED */
