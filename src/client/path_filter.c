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
 * @file: path_filter.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.6
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-10 18:11:59
 */

#include "client_api.h"

#include "path_filter.h"

#include "server_conn.h"
#include "watch_path.h"
#include "watch_entry.h"
#include "watch_event.h"

#include "../common/common_util.h"


static void free_path_filter (void *pv)
{
    ssize_t i;

    XS_path_filter pf = (XS_path_filter) pv;

    for (i = 0; i < pf->patterns_used; i++) {
        xs_pattern_free(pf->patterns[i]);
    }

    free(pf->patterns);
    free(pv);
}


XS_path_filter XS_path_filter_create (int sid, int capacity)
{
    XS_path_filter filt = (XS_path_filter) mem_alloc(1, sizeof(xs_path_filter_t));

    assert(filt->patterns_used == 0);

    filt->sid = sid;
    filt->patterns_capacity = capacity;
    filt->patterns = (XS_pattern *) mem_alloc(capacity, sizeof(XS_pattern));

    RefObjectInit(filt);

    return filt;
}


XS_VOID XS_path_filter_release (XS_path_filter * pfilt)
{
    LOGGER_TRACE0();

    RefObjectRelease((void**) pfilt, free_path_filter);
}


XS_RESULT XS_path_filter_add_patterns (XS_path_filter filt, char * patterns)
{
    char * p1, * p2;
    ssize_t len;
    int err, curpos;
    XS_pattern pattern;

    LOGGER_DEBUG("patterns='%s'", patterns);

    p1 = strstr(patterns, "{{");
    assert(p1 == patterns);

    if (filt->patterns_used == filt->patterns_capacity) {
        filt->patterns_capacity += XS_path_filter_patterns_block;
        filt->patterns = (XS_pattern *) mem_realloc(filt->patterns, sizeof(XS_pattern) * filt->patterns_capacity);
    }

    curpos = (int) filt->patterns_used;

    filt->patterns[curpos] = 0;

    err = XS_ERROR;

    while (p1 && (p2 = strstr(p1, "}}/{{")) > p1) {
        p2 += 2;
        *p2++ = 0;

        pattern = xs_pattern_create(p1, strlen(p1));

        if (pattern) {
            err = xs_pattern_build_and_add(filt, curpos, pattern);
            if (err) {
                xs_pattern_free(pattern);
                goto error_exit;
            }
        }

        p1 = p2;
    }

    if (p1) {
        len = strlen(p1);

        if (p1[len - 1] == '/') {
            pattern = xs_pattern_create(p1, len - 1);
        } else {
            pattern = xs_pattern_create(p1, len);
        }

        if (pattern) {
            err = xs_pattern_build_and_add(filt, curpos, pattern);
            if (err) {
                xs_pattern_free(pattern);
                goto error_exit;
            }
        }
    }

error_exit:
    pattern = filt->patterns[curpos];

    if (err) {
        filt->patterns[curpos] = 0;
        if (pattern) {
            xs_pattern_free(pattern);
        }
    } else if (pattern) {
        assert(err == XS_SUCCESS);
        filt->patterns_used++;
    }

    return err;
}
