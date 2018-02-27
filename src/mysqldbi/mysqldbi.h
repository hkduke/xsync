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

#ifndef MYSQLDBI_H_INCLUDED
#define MYSQLDBI_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif


/***********************************************************************
 * mysql client api
 *
 * MySQL 5.7 Reference Manual
 *   https://dev.mysql.com/doc/refman/5.7/en/
 *
 * MySQL C API 安装和使用:
 *   https://segmentfault.com/a/1190000003932403
 *
 * MYSQL C API 入门教程:
 *   http://www.cnblogs.com/sherlockhua/archive/2012/03/31/2426399.html
 *
 * Install mysql-devel on ubuntu:
 *    $ sudo apt-get install libmysqlclient-dev
 *
 *  install at:
 *    /usr/lib/x86_64-linux-gnu/libmysqlclient.a
 *
 *  使用下面的命令查看编译选项:
 *
 *   $ mysql_config --cflags
 *   $ mysql_config --libs
 *
 **********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql/mysql.h>


extern int test_mysqldbi (const char *host, const char *user, const char *passwd, const char *db, unsigned int port);


#if defined(__cplusplus)
}
#endif

#endif /* MYSQLDBI_H_INCLUDED */
