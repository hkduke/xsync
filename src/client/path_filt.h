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
 * path_filt.h
 *   path filter using regex
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-25
 * update: 2018-01-25
 *
 */
#ifndef PATH_FILT_H_INCLUDED
#define PATH_FILT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common.h"


typedef struct xs_path_filt_t
{
    EXTENDS_REFOBJECT_TYPE();

    int      num_regs;
    regex_t  reg_list[0];

    ssize_t  length;
    char     filters[0];
} * XS_path_filt, xs_path_filt_t;


extern int XS_path_filt_create (const char **filters, int num_filters, XS_path_filt * pfilt);

extern void XS_path_filt_release (XS_path_filt * pfilt);



#if defined(__cplusplus)
}
#endif

#endif /* PATH_FILT_H_INCLUDED */
