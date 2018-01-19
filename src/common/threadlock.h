/**
 * threadlock.h
 *
 * for GNUC
 * 
 * created by cheungmine
 * 2013-4-23
 */
#ifndef _THREAD_LOCK_H_GNUC
#define _THREAD_LOCK_H_GNUC

#ifdef _MSC_VER
 /* TODO: support for windows */
 #error Currently windows is not supported
#elif defined(__GNUC__)
 #if __GNUC__<4 || (__GNUC__==4 && __GNUC_MINOR__<1)
  #error GCC version must be greater or equal than 4.1.2
 #endif
 #include <sched.h>
#else
 #error Currently only windows and linux os are supported
#endif

#include <pthread.h>

/**
 * get thread id
 */
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)  

#ifdef USE_SPINLOCK
  typedef pthread_spinlock_t thread_lock_t;
  #define thread_lock_init            pthread_spin_init
  #define thread_lock_acquire         pthread_spin_lock
  #define thread_trylock_acquire      pthread_spin_trylock
  #define thread_lock_release         pthread_spin_unlock
  #define thread_lock_destroy         pthread_spin_destroy
#else
  typedef pthread_mutex_t thread_lock_t;
  #define thread_lock_init            pthread_mutex_init
  #define thread_lock_acquire         pthread_mutex_lock
  #define thread_trylock_acquire      pthread_mutex_trylock
  #define thread_lock_release         pthread_mutex_unlock
  #define thread_lock_destroy         pthread_mutex_destroy
#endif

#endif /* _THREAD_LOCK_H_GNUC */
