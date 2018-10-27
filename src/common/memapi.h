/***********************************************************************
* COPYRIGHT (C) 2018 PEPSTACK, PEPSTACK.COM
*
* THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES
* ARISING FROM THE USE OF THIS SOFTWARE.
*
* PERMISSION IS GRANTED TO ANYONE TO USE THIS SOFTWARE FOR ANY PURPOSE,
* INCLUDING COMMERCIAL APPLICATIONS, AND TO ALTER IT AND REDISTRIBUTE IT
* FREELY, SUBJECT TO THE FOLLOWING RESTRICTIONS:
*
*  THE ORIGIN OF THIS SOFTWARE MUST NOT BE MISREPRESENTED; YOU MUST NOT
*  CLAIM THAT YOU WROTE THE ORIGINAL SOFTWARE. IF YOU USE THIS SOFTWARE
*  IN A PRODUCT, AN ACKNOWLEDGMENT IN THE PRODUCT DOCUMENTATION WOULD
*  BE APPRECIATED BUT IS NOT REQUIRED.
*
*  ALTERED SOURCE VERSIONS MUST BE PLAINLY MARKED AS SUCH, AND MUST NOT
*  BE MISREPRESENTED AS BEING THE ORIGINAL SOFTWARE.
*
*  THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
***********************************************************************/

/**
 * @file: memapi.h
 *
 *   memory helper api
 *
 * @author: master@pepstack.com
 *
 * @create: 2018-10-25
 *
 * @update: 2018-10-26 21:57:54
 */
#ifndef MEMAPI_H_INCLUDED
#define MEMAPI_H_INCLUDED

#include <assert.h>  /* assert */
#include <string.h>  /* memset */
#include <stdio.h>   /* printf, perror */
#include <limits.h>  /* realpath, PATH_MAX=4096 */
#include <stdbool.h> /* memset */
#include <ctype.h>
#include <stdlib.h>  /* malloc, alloc */
#include <errno.h>


#ifdef MEMAPI_USE_LIBJEMALLOC
#  include <jemalloc/jemalloc.h>    /* need to link: libjemalloc.a */
#endif

#if defined(__cplusplus)
extern "C"
{
#endif


/**
 * mem_alloc_zero() allocates memory for an array of nmemb elements of
 *  size bytes each and returns a pointer to the allocated memory.
 * THE MEMORY IS SET TO ZERO.
 */
static inline void * mem_alloc_zero (int nmemb, size_t size)
{
    void * ptr =
#ifdef MEMAPI_USE_LIBJEMALLOC
        je_calloc(nmemb, size);
#else
        calloc(nmemb, size);
#endif

    if (! ptr) {
#ifdef MEMAPI_USE_LIBJEMALLOC
        perror("je_calloc");
#else
        perror("calloc");
#endif
        exit(ENOMEM);
    }

    return ptr;
}


/**
 * mem_alloc_unset() allocate with THE MEMORY NOT BE INITIALIZED.
 */
static inline void * mem_alloc_unset (size_t size)
{
    void * ptr =
#ifdef MEMAPI_USE_LIBJEMALLOC
        je_malloc(size);
#else
        malloc(size);
#endif

    if (! ptr) {
#ifdef MEMAPI_USE_LIBJEMALLOC
        perror("je_malloc");
#else
        perror("malloc");
#endif
        exit(ENOMEM);
    }

    return ptr;

}


/**
 * mem_realloc() changes the size of the memory block pointed to by ptr
 *  to size bytes. The contents will be unchanged in the range from the
 *  start of the region up to the minimum of the old and new sizes.
 * If the new size is larger than the old size,
 *  THE ADDED MEMORY WILL NOT BE INITIALIZED.
 */
static inline void * mem_realloc (void * ptr, size_t size)
{
    void *newptr =
#ifdef MEMAPI_USE_LIBJEMALLOC 
        je_realloc(ptr, size);
#else
        realloc(ptr, size);
#endif

    if (! newptr) {
#ifdef MEMAPI_USE_LIBJEMALLOC 
        perror("je_realloc");
#else
        perror("realloc");
#endif
        exit(ENOMEM);
    }

    return newptr;
}


/**
 * mem_free() frees the memory space pointed to by ptr, which must have
 *  been returned by a previous call to malloc(), calloc() or realloc().
 *  IF PTR IS NULL, NO OPERATION IS PERFORMED. 
 */
static inline void mem_free (void * ptr)
{
    if (ptr) {
#ifdef MEMAPI_USE_LIBJEMALLOC
        je_free(ptr);
#else
        free(ptr);
#endif
    }
}


/**
 * mem_free_s() frees the memory pointed by the address of ptr and set
 *  ptr to zero. it is a safe version if mem_free().
 */
static inline void mem_free_s (void **pptr)
{
    if (pptr) {
        void *ptr = *pptr;

        if (ptr) {
            *pptr = 0;

#ifdef MEMAPI_USE_LIBJEMALLOC
            je_free(ptr);
#else
            free(ptr);
#endif

        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* MEMAPI_H_INCLUDED */
