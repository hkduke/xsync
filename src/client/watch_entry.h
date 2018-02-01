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
 * watch_entry.h
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-24
 * update: 2018-02-01
 *
 */
#ifndef WATCH_ENTRY_H_INCLUDED
#define WATCH_ENTRY_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common.h"

#include "xsync-error.h"

/**
 * xs_watch_entry_t
 *
 */
typedef struct xs_watch_entry_t
{
    EXTENDS_REFOBJECT_TYPE();


    struct xs_watch_entry_t * next;
} * XS_watch_entry, xs_watch_entry_t;


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_ENTRY_H_INCLUDED */
