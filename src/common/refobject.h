/**
 * refobject.h
 *
 * 2017-01-14: first created by master@pepstack.com
 * 2018-01-21: last modified by master@pepstack.com
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

#ifndef _REF_OBJECT_H_INCLUDED
#define _REF_OBJECT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <pthread.h>
#include <unistd.h>


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


typedef void (* FinalizeObjectFunc)(void *);


__attribute__((unused))
static inline int RefObjectInit (void *pv)
{
    int err = pthread_mutex_init(& ((RefObjectPtr) pv)->__lock, 0);
    ((RefObjectPtr) pv)->__refc = 1;
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
            exit(-4);
        }
    }

    return 0;
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
