/**
* standard.h
*
*   Standard definitions and types, Bob Jenkins
*
* 2015-01-21: revised by cheungmine
*/

#ifndef STANDARD_TYPES
#define STANDARD_TYPES

#ifndef STDIO
#  include <stdio.h>
#  define STDIO
#endif

#ifndef STDDEF
#  include <stddef.h>
#  define STDDEF
#endif

#include <float.h>
typedef double fb8;
typedef float  fb4;
#define FB8BITS 64
#define FB4BITS 32
#define FB8MAXVAL   DBL_MAX
#define FB4MAXVAL   FLT_MAX
#define FB8EPSILON  DBL_EPSILON
#define FB4EPSILON  FLT_EPSILON

typedef unsigned long long ub8;
#define UB8MAXVAL 0xffffffffffffffffLL
#define UB8BITS   64

typedef signed long long sb8;
#define SB8MAXVAL 0x7fffffffffffffffLL

typedef unsigned long int ub4;   /* unsigned 4-byte quantities */
#define UB4MAXVAL 0xffffffff

typedef signed long int sb4;
#define UB4BITS   32
#define SB4MAXVAL 0x7fffffff

typedef unsigned short int ub2;
#define UB2MAXVAL 0xffff
#define UB2BITS   16

typedef signed short int sb2;
#define SB2MAXVAL 0x7fff

/* unsigned 1-byte quantities */
typedef unsigned char ub1;
#define UB1MAXVAL 0xff
#define UB1BITS   8

/* signed 1-byte quantities */
typedef signed char sb1;
#define SB1MAXVAL   0x7f

/* fastest type available */
typedef int word;

#define stdbis(target,mask)  ((target) |=  (mask))
#define stdbic(target,mask)  ((target) &= ~(mask))
#define stdbit(target,mask)  ((target) &   (mask))

#ifndef stdmin
#  define stdmin(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */

#ifndef stdmax
#  define stdmax(a,b) (((a)<(b)) ? (b) : (a))
#endif /* max */

#ifndef stdabs
#  define stdabs(a)   (((a)>0) ? (a) : -(a))
#endif

#ifndef stdalign
#  define stdalign(a) (((ub4)a+(sizeof(void *)-1))&(~(sizeof(void *)-1)))
#endif /* align */

#ifndef TRUE
#  define TRUE    1
#  define FALSE   0
#endif

#ifndef SUCCESS
#  define SUCCESS 0  /* 1 on VAX */
#endif

#endif /* STANDARD_TYPES */

