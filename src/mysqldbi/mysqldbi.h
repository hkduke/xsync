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
 * @file: mysqldbi.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.0
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-29 13:08:28
 */

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
