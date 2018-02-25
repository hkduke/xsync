/**
* randstd.h
*
*   Standard definitions and types, Bob Jenkins
*
* 2015-01-19: revised by cheungmine
*/
#ifndef _RANDSTD_H__
#define _RANDSTD_H__

#ifndef STDIO
#  include <stdio.h>
#  define STDIO
#endif

#ifndef STDDEF
#  include <stddef.h>
#  define STDDEF
#endif

#if defined (_SVR4) || defined (SVR4) || defined (__OpenBSD__) ||\
    defined (_sgi) || defined (__sun) || defined (sun) || \
    defined (__digital__) || defined (__HP_cc)
    #include <inttypes.h>
#elif defined (_MSC_VER) && _MSC_VER < 1600
    /* VS 2010 (_MSC_VER 1600) has stdint.h */
    typedef __int8 int8_t;
    typedef unsigned __int8 uint8_t;
    typedef __int16 int16_t;
    typedef unsigned __int16 uint16_t;
    typedef __int32 int32_t;
    typedef unsigned __int32 uint32_t;
    typedef __int64 int64_t;
    typedef unsigned __int64 uint64_t;
#elif defined (_AIX)
    #include <sys/inttypes.h>
#else
    #include <stdint.h>
#endif


typedef uint64_t ub8;
#define UB8MAXVAL 0xffffffffffffffffLL
#define UB8BITS 64

typedef int64_t sb8;
#define SB8MAXVAL 0x7fffffffffffffffLL

/* unsigned 4-byte quantities */
typedef uint32_t ub4;
#define UB4MAXVAL 0xffffffff

typedef int32_t sb4;
#define UB4BITS 32
#define SB4MAXVAL 0x7fffffff

typedef uint16_t ub2;
#define UB2MAXVAL 0xffff
#define UB2BITS 16

typedef int16_t sb2;
#define SB2MAXVAL 0x7fff

/* unsigned 1-byte quantities */
typedef unsigned char ub1;
#define UB1MAXVAL 0xff
#define UB1BITS 8

/* signed 1-byte quantities */
typedef signed char sb1;
#define SB1MAXVAL 0x7f

/* fastest type available */
typedef int word;

#define bis(target,mask)  ((target) |=  (mask))
#define bic(target,mask)  ((target) &= ~(mask))
#define bit(target,mask)  ((target) &   (mask))

#ifndef min
#  define min(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */

#ifndef max
#  define max(a,b) (((a)<(b)) ? (b) : (a))
#endif /* max */

#ifndef abs
#  define abs(a)   (((a)>0) ? (a) : -(a))
#endif

#ifndef align
#  define align(a) (((ub4)a+(sizeof(void *)-1))&(~(sizeof(void *)-1)))
#endif /* align */

#define RAND_TRUE    1
#define RAND_FALSE   0

#define RAND_SUCCESS 0  /* 1 on VAX */

#endif /* _RANDSTD_H__ */
