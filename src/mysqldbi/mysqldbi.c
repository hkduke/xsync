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
