/*******************************************************************************
 * ptrarray.c                                                                  *
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

#include "ptrarray.h"

#define PTRARRAY_NULL 0

static int partition(ptrelement *arr, int left, int right,
  PtrElementCompareFunc pfcbElemComp, void* compFuncParam)
{
  int p, q;
  ptrelement x, t;

  x = arr[left];
  p = left-1;
  q = right+1;

  while(p+1 != q) {
    if(pfcbElemComp(arr[p+1], x, compFuncParam) <= 0)
      p++;
    else if(pfcbElemComp(arr[q-1], x, compFuncParam) > 0)
      q--;
    else {
      t = arr[p+1];
      arr[++p] = arr[q-1];
      arr[--q] = t;
    }
  }

  arr[left] = arr[p];
  arr[p] = x;
  return p;
}

static void quicksort(ptrelement *arr, int left, int right,
  PtrElementCompareFunc pfcbElemComp, void* compFuncParam)
{
  int p;
  if (left < right) {
    p=partition(arr, left, right, pfcbElemComp, compFuncParam);
    quicksort(arr, left, p-1, pfcbElemComp, compFuncParam);
    quicksort(arr, p+1, right, pfcbElemComp, compFuncParam);
  }
}

void ptrarray_create (ptrarray *pa, size_t numElements)
{
  ptrarray_t *arr = (ptrarray) calloc(1, sizeof(ptrarray_t));
  if (!arr)
    exit(PTRARRAY_E_OUTMEMORY);
  *pa = arr;
  arr->_maxCount = numElements;
  if (numElements > 0) {
    arr->_elements = (ptrelement*) calloc(numElements, sizeof(ptrelement));
    if (!arr->_elements)
      exit(PTRARRAY_E_OUTMEMORY);
    arr->_ptrUnused = arr->_elements;
  }
}

void ptrarray_push_back (ptrarray pa, void *element)
{
  if (pa->_usedCount==pa->_maxCount) {
    pa->_maxCount += (PTRARRAY_INCREMENT<8? 8 : 8*(PTRARRAY_INCREMENT/8));
    pa->_elements = (ptrelement*) realloc(pa->_elements, sizeof(ptrelement)*pa->_maxCount);
    if (!pa->_elements)
      exit(PTRARRAY_E_OUTMEMORY);
    memset(pa->_elements+pa->_usedCount, PTRARRAY_NULL, sizeof(ptrelement)*PTRARRAY_INCREMENT);
    pa->_ptrUnused = pa->_elements+pa->_usedCount++;
    assert(pa->_ptrUnused && !*pa->_ptrUnused);
    *pa->_ptrUnused++ = (ptrelement)element;
    assert(!*pa->_ptrUnused);
  }
  else if (pa->_ptrUnused && pa->_ptrUnused != pa->_elements+pa->_maxCount && !*pa->_ptrUnused) {
    pa->_usedCount++;
    *pa->_ptrUnused++ = (ptrelement)element;
  }
  else {
    ptrelement *end = pa->_elements + pa->_maxCount;
    pa->_ptrUnused = pa->_elements;
    while (pa->_ptrUnused != end) {
      if (!*pa->_ptrUnused) {
        pa->_usedCount++;
        *pa->_ptrUnused++ = (ptrelement)element;
        break;
      }
    }
  }  
}

void ptrarray_push_front (ptrarray pa, void *element)
{
  
}

void* ptrarray_pop_element (ptrarray pa, void *element)
{
  ptrelement *end = pa->_elements + pa->_maxCount;
  pa->_ptrUnused = pa->_elements;
  while (pa->_ptrUnused != end) {
    if (*pa->_ptrUnused==element) {
      *pa->_ptrUnused = PTRARRAY_NULL;
      pa->_usedCount--;
      return element;
    }
    pa->_ptrUnused++;
  }
  return PTRARRAY_NULL;
}

void ptrarray_shrink (ptrarray pa)
{
}

size_t ptrarray_get_count (ptrarray pa)
{
  return pa->_maxCount;  
}

size_t ptrarray_get_used_count (ptrarray pa)
{
  return pa->_usedCount;
}

void* ptrarray_get_element (ptrarray pa, size_t at)
{
  return (*(pa->_elements+at));
}

void ptrarray_traverse (ptrarray pa, PtrElementTraverseFunc pfcbElemFunc, void* param)
{
  if (pa) {
    ptrelement *end = pa->_elements + pa->_maxCount;
    pa->_ptrUnused = pa->_elements;
    while (pa->_ptrUnused != end) {
      if (pfcbElemFunc && *pa->_ptrUnused)
        pfcbElemFunc(*pa->_ptrUnused, param);
      pa->_ptrUnused++;
    }
  }
}

void ptrarray_sort(ptrarray pa, int start, int count, PtrElementCompareFunc pfcbElemComp, void* param)
{
  int end = start;
  if (count==-1) 
    end = pa->_maxCount-1;
  else
    end += (count-1);

  if (end>start)
    quicksort(pa->_elements, start, end, pfcbElemComp, param);
}

void ptrarray_clear (ptrarray pa, PtrElementTraverseFunc pfcbFreeElement, void *param)
{
  ptrelement *end = pa->_elements + pa->_maxCount;
  pa->_ptrUnused = pa->_elements;
  while (pa->_ptrUnused != end) {
    if (pfcbFreeElement)
      pfcbFreeElement(*pa->_ptrUnused, param);
    *pa->_ptrUnused++ = PTRARRAY_NULL;
  }
}

void ptrarray_destroy (ptrarray pa, PtrElementTraverseFunc pfcbFreeElement, void *param)
{
  assert(pa);
  if (pfcbFreeElement) {
    ptrelement *end = pa->_elements + pa->_maxCount;
    pa->_ptrUnused = pa->_elements;
    while (pa->_ptrUnused != end) {
      pfcbFreeElement(*pa->_ptrUnused++, param);
    }
  }
  free(pa->_elements);
  free(pa);
}
