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
* 2. Altered source versions must be plainly marked as such, and must not be
*   misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
***********************************************************************/

/**
 * refobject.h
 *
 * create: 2017-01-14
 * update: 2018-02-02
 */

#ifndef _REF_OBJECT_H_INCLUDED
#define _REF_OBJECT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <pthread.h>
#include <unistd.h>

#define REF_OBJECT_INVALID    0

#define REF_OBJECT_NOERROR    0

#define REF_OBJECT_ERROR    (-1)
#define REF_OBJECT_ESYSTEM  (-4)

#define REF_OBJECT_EINVAL    EINVAL
#define REF_OBJECT_ENOMEM    ENOMEM


typedef struct RefObjectType
{
    int __refc;                       /* reference count */
    pthread_mutex_t __lock;          /* global thread mutex */
} * RefObjectPtr;


#define EXTENDS_REFOBJECT_TYPE() \
    union { \
        struct { \
            int __refc; \
            pthread_mutex_t __lock; \
        }; \
        struct RefObjectType  __notused; \
    }


typedef void (* FinalizeObjectFunc) (void *);


__attribute__((unused))
static inline int RefObjectInit (void *pv)
{
    int err = pthread_mutex_init(& ((RefObjectPtr) pv)->__lock, 0);

    ((RefObjectPtr) pv)->__refc = 1;

    /**
     * success: REF_OBJECT_NOERROR
     * failed: REF_OBJECT_EINVAL, REF_OBJECT_ENOMEM
     */
    return err;
}


__attribute__((unused))
static inline RefObjectPtr RefObjectRetain (void **ppv)
{
    RefObjectPtr obj = *((RefObjectPtr *) ppv);

    if (obj) {
        if (__sync_add_and_fetch(&obj->__refc, 1) > 0) {
            return obj;
        } else {
            /* should never run to this */
            exit(REF_OBJECT_ESYSTEM);
        }
    }

    /* NULL object */
    return REF_OBJECT_INVALID;
}


__attribute__((unused))
static inline int RefObjectLock (void *pv, int try)
{
    if (try) {
        return pthread_mutex_trylock(&((RefObjectPtr) pv)->__lock);
    } else {
        return pthread_mutex_lock(&((RefObjectPtr) pv)->__lock);
    }
}


__attribute__((unused))
static inline void RefObjectUnlock (void *pv)
{
    pthread_mutex_unlock(&((RefObjectPtr) pv)->__lock);
}


__attribute__((unused))
static inline void RefObjectRelease (void **ppv, FinalizeObjectFunc pfnFinalObject)
{
    RefObjectPtr obj = *((RefObjectPtr *) ppv);

    if (obj) {
        if (0 == __sync_sub_and_fetch(&obj->__refc, 1)) {
            *ppv = 0;

            pthread_mutex_lock(&obj->__lock);
            pthread_mutex_destroy(&obj->__lock);

            pfnFinalObject(obj);
        }
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* _REF_OBJECT_H_INCLUDED */
