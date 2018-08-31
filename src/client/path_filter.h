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
 * @file: path_filter.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.1
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-10 18:11:59
 */

#ifndef PATH_FILTER_H_INCLUDED
#define PATH_FILTER_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../xsync-error.h"
#include "../xsync-config.h"

#include "../common/common_util.h"

#include "../xsync-xmlconf.h"


#define XS_path_filter_type_excluded   0
#define XS_path_filter_type_included   1

#define XS_path_filter_patterns_block  8


// 正则表达式模式匹配
#define XS_pattern_target_regex        0

// 外部脚本模式匹配
#define XS_pattern_target_script       1


static const char * XS_pattern_target_names[] = {
    "regex",
    "script"
};


/**
 * 模式匹配对象
 */
typedef struct xs_pattern_t
{
    // 指向下一个正则模式对象
    struct xs_pattern_t * next;

    // 要匹配的目标对象类型: 正则表达式, 外部脚本
    //
    int target;

    // 根据原始正则语句存放编译好的正则对象
    regex_t  reg;

    char errbuf[XSYNC_ERRBUF_MAXLEN + 1];

    // 语句长度(不包括结尾'\0'字符)
    ssize_t pattern_len;

    // 原始语句
    char pattern[0];
} * XS_pattern, xs_pattern_t;


typedef struct xs_path_filter_t
{
    EXTENDS_REFOBJECT_TYPE();
    int sid;
    ssize_t patterns_used;
    ssize_t patterns_capacity;
    struct xs_pattern_t ** patterns;
} xs_path_filter_t;


__no_warning_unused(static)
XS_pattern xs_pattern_create (const char * pattern, ssize_t pattern_len)
{
    if (pattern_len > 4 &&
        pattern[0] == '{' && pattern[1] == '{' &&
        pattern[pattern_len - 1] == '}' && pattern[pattern_len - 2] == '}') {

        XS_pattern p = (XS_pattern) mem_alloc(1, sizeof(xs_pattern_t) + pattern_len + 1);

        if (pattern[2] == '@' && pattern[3] == '@') {
            // 脚本文件名
            p->target = XS_pattern_target_script;
            p->pattern_len = pattern_len - 6;
            memcpy(p->pattern, pattern + 4, p->pattern_len);
        } else {
            // 正则表达式
            p->target = XS_pattern_target_regex;
            p->pattern_len = pattern_len - 4;
            memcpy(p->pattern, pattern + 2, p->pattern_len);
        }

        p->pattern[p->pattern_len] = 0;

        LOGGER_DEBUG("%s: '%s'", XS_pattern_target_names[p->target], p->pattern);

        return p;
    }

    LOGGER_ERROR("invalid pattern: %s", pattern);
    return 0;
}


__no_warning_unused(static)
void xs_pattern_free (xs_pattern_t * p)
{
    while (p) {
        if (p->target == XS_pattern_target_regex) {
            regfree(&p->reg);
        }

        xs_pattern_free(p->next);

        free(p);

        LOGGER_TRACE0();
    }
}


__no_warning_unused(static)
int xs_pattern_build_and_add (XS_path_filter pf, int at, XS_pattern p)
{
    int err;

    if (p->target == XS_pattern_target_regex) {
        err = regcomp(&p->reg, p->pattern, 0);
    } else {
        err = 0;
    }

    if (! err) {
        XS_pattern head = pf->patterns[at];

        assert(p->next == 0);

        if (! head) {
            pf->patterns[at] = p;
        } else {
            while (head->next) {
                head = head->next;
            }
            // add at end of list
            head->next = p;
        }
        // success
        return 0;
    } else {
        regerror(err, &p->reg, p->errbuf, sizeof(p->errbuf));
        LOGGER_ERROR("regcomp error(%d): %s. (%s)", err, p->errbuf, p->pattern);
        return err;
    }
}


__no_warning_unused(static)
void xs_filter_set_xmlnode (XS_path_filter filter, mxml_node_t *filter_node)
{
    int i;

    mxmlElementSetAttrf(filter_node, "sid", "%d", filter->sid);
    mxmlElementSetAttrf(filter_node, "size", "%d", (int) filter->patterns_used);

    for (i = 0; i < filter->patterns_used; i++) {
        XS_pattern pattern = filter->patterns[i];
        mxml_node_t *first_pattern_node = 0;

        while (pattern) {
            mxml_node_t *pattern_node;

            if (! first_pattern_node) {
                first_pattern_node = mxmlNewElement(filter_node, "xs:pattern");
                pattern_node = first_pattern_node;
            } else {
                pattern_node = mxmlNewElement(first_pattern_node, "xs:sub-pattern");
            }

            mxmlElementSetAttr(pattern_node, XS_pattern_target_names[pattern->target], pattern->pattern);

            //mxmlNewTextf(pattern_node, 0, "%s", pattern->pattern);

            // next pattern
            pattern = pattern->next;
        }
    }
}


extern XS_path_filter XS_path_filter_create (int sid, int capacity);

extern XS_VOID XS_path_filter_release (XS_path_filter * pfilt);

extern XS_RESULT XS_path_filter_add_patterns (XS_path_filter filt, char * patterns);

#if defined(__cplusplus)
}
#endif

#endif /* PATH_FILTER_H_INCLUDED */
