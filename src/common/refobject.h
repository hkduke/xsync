/**
 * refobject.h
 *
 * 2017-01-14: last modified by master@pepstack.com
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

#ifndef _REF_OBJECT_H_
#define _REF_OBJECT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <pthread.h>
#include <unistd.h>


typedef struct RefObjectType
{
    int __refc;                       /* reference count */
    pthread_mutex_t __lock;          /* global thread mutex */
} * RefObjectHandle;


#define REFOBJECT_TYPE_HEADER() \
    union { \
        struct { \
            int __refc; \
            pthread_mutex_t __lock; \
        }; \
        struct RefObjectType  __notused; \
    }


typedef void (* FinalObjectFunc)(void *);


static int RefObjectInit (void *pv)
{
    int err = pthread_mutex_init(& ((RefObjectHandle) pv)->__lock, 0);
    ((RefObjectHandle) pv)->__refc = 1;
    return err;
}


static RefObjectHandle RefObjectRetain (void **ppv)
{
    RefObjectHandle obj = *((RefObjectHandle *) ppv);

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


static void RefObjectRelease (void **ppv, FinalObjectFunc pfnFinalObject)
{
    RefObjectHandle obj = *((RefObjectHandle *) ppv);

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

#endif /* _REF_OBJECT_H_ */
