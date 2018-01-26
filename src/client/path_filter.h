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
 * path_filter.h
 *   path filter using regex
 *
 * author:
 *     master@pepstack.com
 *
 * create: 2018-01-25
 * update: 2018-01-26
 *
 */
#ifndef PATH_FILTER_H_INCLUDED
#define PATH_FILTER_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "../common.h"

/** 1.included
 * # 目录名总是以字符 '/' 结尾
 * #
 * xs_client_log/{{文件名匹配表达式1}}
 * xs_client_log/{{目录名匹配表达式1}}/{{文件名匹配表达式2}}
 * xs_client_log/{{目录名匹配表达式3}}/{{目录名匹配表达式2}}/{{文件名匹配表达式1}}
 * xs_client_log/{{目录名匹配表达式4}}/
 * xs_client_log/{{目录名匹配表达式4}}/
 * tmp/{{文件匹配表达式3}}
 */

/*
#define XS_reg_target_file_name
#define XS_reg_target_file_
#define XS_reg_target_file_
#define XS_reg_target_file_
#define XS_reg_target_file_

#define XS_reg_target_flnk_name
#define XS_reg_target_flnk_
#define XS_reg_target_flnk_
#define XS_reg_target_flnk_
#define XS_reg_target_flnk_

#define XS_reg_target_dir_name
#define XS_reg_target_dir_
#define XS_reg_target_dir_
#define XS_reg_target_dir_
#define XS_reg_target_dir_

#define XS_reg_target_dlnk_name
#define XS_reg_target_dlnk_
#define XS_reg_target_dlnk_
#define XS_reg_target_dlnk_
#define XS_reg_target_dlnk_
*/

/**
 * 正则模式对象
 */
typedef struct reg_pattern_t
{
    // 指向下一个正则模式对象
    struct regex_pattern_t * next;

    // 要匹配的目标对象类型:
    //   文件名, 目录名, 文件创建时间, 目录创建时间, 文件修改时间, 目录修改时间, ...
    //   链接文件名, 链接目录名
    int target;

    // 根据原始正则语句存放编译好的正则对象
    regex_t  reg;

    // 原始正则语句
    char pattern[0];
} reg_pattern_t;


typedef struct xs_path_filter_t
{
    EXTENDS_REFOBJECT_TYPE();

    int num_regs;
    reg_pattern_t  reg_patterns[0];
} * XS_path_filter, xs_path_filter_t;


extern int XS_path_filter_create (const char **filters, int num_filters, XS_path_filter * pfilt);

extern void XS_path_filter_release (XS_path_filter * pfilt);



#if defined(__cplusplus)
}
#endif

#endif /* PATH_FILTER_H_INCLUDED */
