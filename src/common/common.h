/******************************************************************************
 * common.h
 *
 * Copyright (C)1996-2009 by cheungmine <cheungmine@gmail.com>.
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published
 *   by the Free Software Foundation, either version 2.1 of the License, or
 *   (at your option) any later version.
 *
 * This program  is  distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU Lesser General Public License for more details.
 *
 * You should  have received a copy of the GNU Lesser General Public License
 *   along with  this program.
 *****************************************************************************/
#ifndef _CLANG_COMMON_H__
#define _CLANG_COMMON_H__

/**
 * Standard Headers
 */
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>  /* <math.h>, <complex.h> */
#include <stdbool.h> /* bool, true, false */
#include <ctype.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <endian.h>

#ifndef uchar_t
typedef unsigned char uchar_t;
#endif

#ifndef byte_t
typedef unsigned char byte_t;
#endif

#ifndef stricmp
  #define stricmp strcasecmp
#endif

#ifndef _IN
  #define _IN
#endif

#ifndef _OUT
  #define _OUT
#endif

/**
 * Define NULL pointer value
 */
#ifndef NULL
  #ifdef __cplusplus
    #define NULL 0
  #else
    #define NULL ((void*)0)
  #endif
#endif 

#ifndef   BOOL
  #define BOOL bool
#endif

#ifndef   TRUE
  #define TRUE true
#endif

#ifndef   FALSE
  #define FALSE false
#endif

#ifndef SWAP
  #define SWAP(x,y) ({\
    (x) ^= (y); \
    (y) ^= (x); \
    (x) ^= (y); })
#endif

static void swap_n(void *a, void *b, int size) {
  void *t;
  if (a!=b) {
    t = alloca(size);
    memcpy(t, a, size);
    memcpy(a, b, size);
    memcpy(b, t, size);
  }
}

#ifndef ABS
  #define ABS(x) ((x)>0?(x):((x)<0?(-x):(x)))
#endif

/* linux-2.6.38.8/include/linux/kernel.h */
#ifndef MIN
#define MIN(x, y) ({ \
  typeof(x) _min1 = (x); \
  typeof(y) _min2 = (y); \
  (void) (&_min1 == &_min2); \
  _min1 < _min2 ? _min1 : _min2; })
#endif

#ifndef MAX
#define MAX(x, y) ({ \
  typeof(x) _max1 = (x); \
  typeof(y) _max2 = (y); \
  (void) (&_max1 == &_max2); \
  _max1 > _max2 ? _max1 : _max2; })
#endif

typedef struct _blob_t {
  uint32_t size;
  void    *data;
} blob_t;

/**
 * C Standard Data Type
 */
typedef enum _CTYPE_ENUM {
  /* 0-end chars array */
  CTYPE_STRING = 0x0,

  /* 00-end wchars array */
  CTYPE_WSTRING = 0x1,

  /* 8-bits types */
  CTYPE_CHAR = 0x2,
  CTYPE_BOOL = CTYPE_CHAR,
  CTYPE_UCHAR = 0xa,
  CTYPE_BYTE = CTYPE_UCHAR,

  /* 16-bits types */
  CTYPE_INT16 = 0x3,
  CTYPE_SHORT = CTYPE_INT16,
  CTYPE_UINT16 = 0xb,
  CTYPE_USHORT = CTYPE_UINT16,
  CTYPE_WCHAR = CTYPE_UINT16,

  /* 32-bits types */
  CTYPE_INT32 = 0x4,
  CTYPE_INT = CTYPE_INT32,
  CTYPE_UINT32 = 0xc,
  CTYPE_UINT = CTYPE_UINT32,

  /* 64-bits types */
  CTYPE_INT64 = 0x5,
  CTYPE_LONGLONG = CTYPE_INT64,
  CTYPE_UINT64 = 0xd,
  CTYPE_ULONGLONG = CTYPE_UINT64,

  /* 32-bits float point types */
  CTYPE_FLOAT = 0x6,
  CTYPE_FLOAT32 = CTYPE_FLOAT,

  /* 64-bits float point types */
  CTYPE_FLOAT64 = 0x7,
  CTYPE_DOUBLE = CTYPE_FLOAT64,

  /* 0x8, 0x9 left for furture used */
  CTYPE_NUMERIC = 0x8,
  CTYPE_BIGINT = 0x9,

  /* 128-bits uuid */
  CTYPE_UUID = 0xe,

  /* blob */
  CTYPE_BLOB = 0xf
} CTYPE_ENUM;

/**
 * Variant for C Standard Data Type
 */
typedef struct _CTYPE_VARIANT {
  CTYPE_ENUM  val_type;

  union {
    /* string */
    char    *string_val;
    wchar_t *wstring_val;

    /* 8-bits types */
    char    char_val;
    char    bool_val;  /* 0, 1 */
    uchar_t uchar_val;
    byte_t  byte_val;

    /* 16-bits types */
    int16_t  int16_val;
    int16_t  short_val;
    uint16_t uint16_val;
    uint16_t ushort_val;
    wchar_t  wchar_val;
    
    /* 32-bits types */
    int32_t  int_val;
    int32_t  int32_val;
    uint32_t uint_val;
    uint32_t uint32_val;

    /* 64-bits types */
    int64_t  int64_val;
    int64_t  longlong_val;
    uint64_t uint64_val;
    uint64_t ulonglong_val;

    /* 32-bits float point types */
    float float_val;
    float float32_val;

    /* 64-bits float point types */
    double float64_val;
    double double_val;
  
    /* 128-bits uuid */
    byte_t uuid_val[16];

    /* bytes array */
    blob_t blob_val;
  };
} CTYPE_VARIANT;

static void ctype_variant_init(CTYPE_VARIANT *var, CTYPE_ENUM type) {

}

static void ctype_variant_free(CTYPE_VARIANT *var) {
}

/**
 * ctype: from enum to name
 */
static const char* ctype_enum_to_name(CTYPE_ENUM ctype, char name[]) {
  switch(ctype) {
  case 0x0:
    strcpy(name, "STRING");
    break;
  case 0x1:
    strcpy(name, "WSTRING");
    break;
  case 0x2:
    strcpy(name, "CHAR");
    break;
  case 0xa:
    strcpy(name, "BYTE");
    break;
  case 0x3:
    strcpy(name, "SHORT");
    break;
  case 0xb:
    strcpy(name, "USHORT");
    break;
  case 0x4:
    strcpy(name, "INT");
    break;
  case 0xc:
    strcpy(name, "UINT");
    break;
  case 0x5:
    strcpy(name, "LONGLONG");
    break;
  case 0xd:
    strcpy(name, "ULONGLONG");
    break;
  case 0x6:
    strcpy(name, "FLOAT");
    break;
  case 0x7:
    strcpy(name, "DOUBLE");
    break;
  case 0x8:
    strcpy(name, "NUMERIC");
    break;
  case 0x9:
    strcpy(name, "BIGINT");
    break;
  case 0xe:
    strcpy(name, "UUID");
    break;
  case 0xf:
    strcpy(name, "BLOB");
    break;
  default :
    return NULL;
  }
  return name;
}

/**
 * ctype: from name to enum
 */
static BOOL ctype_name_to_enum(const char* ctype, CTYPE_ENUM *outType) {
  if (!strcmp("STRING", ctype)) {
    *outType = CTYPE_STRING;
    return TRUE;
  }
  else if (!strcmp("WSTRING", ctype)) {
    *outType = CTYPE_WSTRING;
    return TRUE;
  }
  else if (!strcmp("CHAR", ctype)) {
    *outType = CTYPE_CHAR;
    return TRUE;
  }
  else if (!strcmp("BOOL", ctype)) {
    *outType = CTYPE_BOOL;
    return TRUE;
  }
  else if (!strcmp("UCHAR", ctype)) {
    *outType = CTYPE_UCHAR;
    return TRUE;
  }
  else if (!strcmp("BYTE", ctype)) {
    *outType = CTYPE_BYTE;
    return TRUE;
  }
  else if (!strcmp("INT16", ctype)) {
    *outType = CTYPE_INT16;
    return TRUE;
  }
  else if (!strcmp("SHORT", ctype)) {
    *outType = CTYPE_SHORT;
    return TRUE;
  }
  else if (!strcmp("UINT16", ctype)) {
    *outType = CTYPE_UINT16;
    return TRUE;
  }
  else if (!strcmp("USHORT", ctype)) {
    *outType = CTYPE_USHORT;
    return TRUE;
  }
  else if (!strcmp("WCHAR", ctype)) {
    *outType = CTYPE_WCHAR;
    return TRUE;
  }
  else if (!strcmp("INT32", ctype)) {
    *outType = CTYPE_INT32;
    return TRUE;
  }
  else if (!strcmp("INT", ctype)) {
    *outType = CTYPE_INT;
    return TRUE;
  }
  else if (!strcmp("UINT32", ctype)) {
    *outType = CTYPE_UINT32;
    return TRUE;
  }
  else if (!strcmp("UINT", ctype)) {
    *outType = CTYPE_UINT;
    return TRUE;
  }
  else if (!strcmp("INT64", ctype)) {
    *outType = CTYPE_INT64;
    return TRUE;
  }
  else if (!strcmp("LONGLONG", ctype)) {
    *outType = CTYPE_LONGLONG;
    return TRUE;
  }
  else if (!strcmp("UINT64", ctype)) {
    *outType = CTYPE_UINT64;
    return TRUE;
  }
  else if (!strcmp("ULONGLONG", ctype)) {
    *outType = CTYPE_ULONGLONG;
    return TRUE;
  }
  else if (!strcmp("FLOAT", ctype)) {
    *outType = CTYPE_FLOAT;
    return TRUE;
  }
  else if (!strcmp("FLOAT32", ctype)) {
    *outType = CTYPE_FLOAT32;
    return TRUE;
  }
  else if (!strcmp("FLOAT64", ctype)) {
    *outType = CTYPE_FLOAT64;
    return TRUE;
  }
  else if (!strcmp("DOUBLE", ctype)) {
    *outType = CTYPE_DOUBLE;
    return TRUE;
  }
  else if (!strcmp("NUMERIC", ctype)) {
    *outType = CTYPE_NUMERIC;
    return TRUE;
  }
  else if (!strcmp("BIGINT", ctype)) {
    *outType = CTYPE_BIGINT;
    return TRUE;
  }
  else if (!strcmp("UUID", ctype)) {
    *outType = CTYPE_UUID;
    return TRUE;
  }
  else if (!strcmp("BLOB", ctype)) {
    *outType = CTYPE_BLOB;
    return TRUE;
  }
  else {
    return FALSE;
  }
  return FALSE;
}

#endif /* _CLANG_COMMON_H__ */

