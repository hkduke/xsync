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
 * watch_event.h
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-24
 * update: 2018-01-24
 *
 */
#ifndef WATCH_EVENT_H_INCLUDED
#define WATCH_EVENT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common.h"

#define XS_watch_event_typeid   100


typedef struct xs_watch_event_t
{
    EXTENDS_REFOBJECT_TYPE();



} * XS_watch_event, xs_watch_event_t;


extern int XS_watch_event_create (XS_watch_event * event);

extern void XS_watch_event_release (XS_watch_event * event);


#if defined(__cplusplus)
}
#endif

#endif /* WATCH_EVENT_H_INCLUDED */