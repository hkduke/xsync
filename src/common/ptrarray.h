/*******************************************************************************
 * ptrarray.h                                                                  *
 *                                                                             *
 * Copyright (C)1996-2009 by cheungmine <cheungmine@gmail.com>.                *
 * All rights reserved.                                                        *
 *                                                                             *
 * This program is free  software: you can redistribute it and/or modify       *
 *   it under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 2.1 of the License, or    *
 *   (at your option) any later version.                                       *
 *                                                                             *
 * This program  is  distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
 *   GNU Lesser General Public License for more details.                       *
 *                                                                             *
 * You should  have received a copy of the GNU Lesser General Public License   *
 *   along with  this program.                                                 *
 ******************************************************************************/
#ifndef PTRARRAY_H__
#define PTRARRAY_H__

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * user can define PTRARRAY_INCREMENT before include ptrarray.h 
 *   to fit actural requirement
 */
#ifndef PTRARRAY_INCREMENT
  #define PTRARRAY_INCREMENT 8
#endif

#define PTRARRAY_E_OUTMEMORY (-10)

#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _ptrarray_t *ptrarray;
typedef ptrarray *ptrelement;

typedef struct _ptrarray_t
{
  size_t _maxCount;
  size_t _usedCount;
  ptrelement *_ptrUnused;
  ptrelement *_elements;
} ptrarray_t;

typedef int(*PtrElementTraverseFunc)(void *element, void *param);

typedef int(*PtrElementCompareFunc)(void *element, void *element2, void *param);

static int defPtrElementFree(void *element, void *param)
{
  printf("====%d\n", (int)element);
  return 0;
}

static int defPtrElementCompInc(void *e1, void *e2, void *param)
{
  int d = (int)e1 - (int)e2;
  if (d > 0)
    return 1;
  else if (d < 0)
    return -1;
  else
    return 0;
}

static int defPtrElementCompareDec(void *e1, void *e2, void *param)
{
  int d = (int)e2 - (int)e1;
  if (d > 0)
    return 1;
  else if (d < 0)
    return -1;
  else
    return 0;
}

void ptrarray_create (ptrarray *pa, size_t numElements);

/**
 * add an element into the end of array
 */
void ptrarray_push_back (ptrarray pa, void *element);

/**
 * add an element into the head of array
 */
void ptrarray_push_front (ptrarray pa, void *element);

/**
 * remove an element from array, not free the space it has occupied
 * success : return element poped
 * failed : 0
 */
void* ptrarray_pop_element (ptrarray pa, void *element);

/**
 * erase all unused elements
 */
void ptrarray_shrink (ptrarray pa);

size_t ptrarray_get_count (ptrarray pa);

size_t ptrarray_get_used_count (ptrarray pa);

void* ptrarray_get_element (ptrarray pa, size_t at);

void ptrarray_traverse (ptrarray pa, PtrElementTraverseFunc pfcbElemFunc, void* param);

void ptrarray_sort(ptrarray pa, int start, int count, PtrElementCompareFunc pfcbElemComp, void* param);

void ptrarray_clear (ptrarray pa, PtrElementTraverseFunc pfcbFreeElement, void *param);

void ptrarray_destroy (ptrarray pa, PtrElementTraverseFunc pfcbFreeElement, void *param);

#ifdef __cplusplus
}
#endif

#endif  /* PTRARRAY_H__ */
