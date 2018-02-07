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
 * update: 2018-02-04
 */

#ifndef _REF_OBJECT_H_INCLUDED
#define _REF_OBJECT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif


#include "threadlock.h"


#define REF_OBJECT_INVALID     0

#define REF_OBJECT_NOERROR     0

#define REF_OBJECT_ERROR     (-1)
#define REF_OBJECT_ESYSTEM   (-4)

#define REF_OBJECT_EINVAL    EINVAL
#define REF_OBJECT_ENOMEM    ENOMEM


typedef struct RefObjectType
{
    ref_counter_t refc__;            /* reference count */
    thread_lock_t lock__;            /* global thread mutex */
} * RefObjectPtr;


#define EXTENDS_REFOBJECT_TYPE() \
    union { \
        struct { \
            ref_counter_t refc__; \
            thread_lock_t lock__; \
        }; \
        struct RefObjectType  _notused_; \
    }


typedef void (* FinalObjectFunc) (void *);


#define RefObjectLock(pvOb)    \
    threadlock_lock(&((RefObjectPtr) (pvOb))->lock__)


#define RefObjectTryLock(pvOb)    \
    threadlock_trylock(&((RefObjectPtr) (pvOb))->lock__)


#define RefObjectUnlock(pvOb)    \
    threadlock_unlock(&((RefObjectPtr) (pvOb))->lock__)


__no_warning_unused(static)
inline RefObjectPtr RefObjectInit (void *pv)
{
    threadlock_init(&((RefObjectPtr) pv)->lock__);

    /* As the moment of init the refcount alway == 1 */
    ((RefObjectPtr) pv)->refc__ = 1LL;

    return ((RefObjectPtr) pv);
}


__no_warning_unused(static)
inline RefObjectPtr RefObjectRetain (void **ppv)
{
    RefObjectPtr obj = *((RefObjectPtr*) ppv);

    if (obj) {
        if (__interlock_add(&obj->refc__) > 1) {
            /* fix bug: __interlock_add(&obj->refc__) > 0 */
            return obj;
        }

        exit(REF_OBJECT_ESYSTEM);
    }

    /* should never run to this! */
    return (RefObjectPtr) 0;
}


__no_warning_unused(static)
inline void RefObjectRelease (void **ppv, FinalObjectFunc pfnFinalObject)
{
    RefObjectPtr obj = *((RefObjectPtr *) ppv);

    if (obj) {
        if (0 == __interlock_sub(&obj->refc__)) {
            *ppv = 0;

            threadlock_lock(&obj->lock__);
            threadlock_destroy(&obj->lock__);

            pfnFinalObject(obj);
        }
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* _REF_OBJECT_H_INCLUDED */
