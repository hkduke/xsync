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
 * @file: mysqldbi.c
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.3
 *
 * @create: 2018-01-25
 *
 * @update: 2018-08-10 18:11:59
 */

#include "mysqldbi.h"

#include <mysql/mysql.h>


extern int test_mysqldbi (const char *host, const char *user, const char *passwd, const char *db, unsigned int port)
{
    MYSQL *mysql;

    mysql = mysql_init(NULL);

    if (! mysql) {
        printf("insufficent memory\n");
        exit(-4);
    }

    printf("mysql_get_client_info(): %s\n", mysql_get_client_info());

    /* Connect to mysql database */
    if (! mysql_real_connect(mysql, host, user, passwd, db, port, NULL, 0)) {
        printf("mysql_real_connect error: %s\n", mysql_error(mysql));

        mysql_close(mysql);
        return (-1);
    }

    /* TODO: query ... */


    mysql_close(mysql);

    return 0;
}
