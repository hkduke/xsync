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
 * path_filt.c
 *   path filter using regex
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-25
 * update: 2018-01-26
 *
 */

/* log4c */
#include "client.h"

#include "path_filter.h"



static void free_path_filter (void *pv)
{
    ssize_t i;

    XS_path_filter pf = (XS_path_filter) pv;

    for (i = 0; i < pf->patterns_used; i++) {
        xs_pattern_free(pf->patterns[i]);
    }

    free(pv);
}


XS_path_filter XS_path_filter_create (int capacity)
{
    XS_path_filter filt = (XS_path_filter) mem_alloc(1, sizeof(xs_path_filter_t) + sizeof(XS_pattern) * capacity);

    assert(filt->patterns_used == 0);

    filt->patterns_capacity = capacity;

    RefObjectInit(filt);

    return filt;
}


void XS_path_filter_release (XS_path_filter * pfilt)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) pfilt, free_path_filter);
}


int XS_path_filter_add_patterns (XS_path_filter filt, char * patterns)
{
    char * p1, * p2;
    ssize_t len;
    XS_pattern pattern;

    LOGGER_DEBUG("patterns='%s'", patterns);

    p1 = strstr(patterns, "{{");
    assert(p1 == patterns);

    while (p1 && (p2 = strstr(p1, "}}/{{")) > p1) {
        p2 += 2;
        *p2++ = 0;

        pattern = xs_pattern_create(p1, strlen(p1), XS_reg_target_dir_name);
        if (pattern) {
            xs_pattern_build_and_add(filt, pattern);
        }

        p1 = p2;
    }

    if (p1) {
        len = strlen(p1);

        if (p1[len - 1] == '/') {
            pattern = xs_pattern_create(p1, len - 1, XS_reg_target_dir_name);
        } else {
            pattern = xs_pattern_create(p1, len, XS_reg_target_file_name);
        }

        if (pattern) {
            xs_pattern_build_and_add(filt, pattern);
        }
    }

    return 0;
}
