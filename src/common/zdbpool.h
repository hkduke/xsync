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

#ifndef ZDBPOOL_H_INCLUDED
#define ZDBPOOL_H_INCLUDED


#if defined(__cplusplus)
extern "C" {
#endif

#define ZDBPOOL_ERRMSG_LEN  255


/***********************************************************************
 * http://www.thinkingyu.com/articles/
 * https://bitbucket.org/tildeslash/libzdb/
 * http://blog.csdn.net/geqiandebei/article/details/48815371
 *
 * libzdb exception:
 *
 *     TRY
 *     {
 *
 *     }
 *     CATCH(SQLException) {
 *
 *     }
 *     FINALLY {
 *
 *     }
 *     END_TRY;
 *
 **********************************************************************/

#include <zdb/zdb.h>

#define ZDBPOOL_SUCCESS           0
#define ZDBPOOL_ERROR           (-1)


#define ZDBPOOL_INIT_SIZE         5
#define ZDBPOOL_MAX_SIZE         20

#define ZDBPOOL_REAP_INTERVAL    30
#define ZDBPOOL_CONN_TIMEOUT     30


typedef struct zdbpool_t
{
    ConnectionPool_T pool;

    char errmsg[ZDBPOOL_ERRMSG_LEN + 1];
} zdbpool_t;


/**
 * connURI:
 *    "mysql://127.0.0.1/xsync?user=root&password=Abc123"
 *
 * poolSize:
 *    20
 */
__attribute__((used))
static void zdbpool_init (zdbpool_t *dbPool, void(*errorHandler)(const char *error),
    const char * connURI, int initPoolSize, int maxPoolSize, int reapSweepInterval, int connectionTimeout)
{
    ConnectionPool_T pool;

    bzero(dbPool, sizeof(zdbpool_t));

    pool = ConnectionPool_new(URL_new(connURI));

    if (initPoolSize == 0) {
        initPoolSize = ZDBPOOL_INIT_SIZE;
    }

    if (maxPoolSize == 0) {
        maxPoolSize = ZDBPOOL_MAX_SIZE;
    }

    if (reapSweepInterval == 0) {
        reapSweepInterval = ZDBPOOL_REAP_INTERVAL;
    }

    if (connectionTimeout == 0) {
        connectionTimeout = ZDBPOOL_CONN_TIMEOUT;
    }

    if (initPoolSize > 0) {
        ConnectionPool_setInitialConnections(pool, initPoolSize);
    }

    if (maxPoolSize > 0) {
        ConnectionPool_setMaxConnections(pool, maxPoolSize);
    }

    if (connectionTimeout > 0) {
        ConnectionPool_setConnectionTimeout(pool, connectionTimeout);
    }

    if (reapSweepInterval > 0) {
        ConnectionPool_setReaper(pool, reapSweepInterval);
    }

    ConnectionPool_setAbortHandler(pool, errorHandler);

    ConnectionPool_start(pool);

    snprintf(dbPool->errmsg, ZDBPOOL_ERRMSG_LEN, "libzdb version: %s", ConnectionPool_version());
    dbPool->errmsg[ZDBPOOL_ERRMSG_LEN] = 0;

    dbPool->pool = pool;
}


__attribute__((used))
static void zdbpool_end (zdbpool_t *dbPool)
{
    URL_T url = ConnectionPool_getURL(dbPool->pool);

    ConnectionPool_stop(dbPool->pool);
    ConnectionPool_free(&dbPool->pool);

    URL_free(&url);

    bzero(dbPool, sizeof(zdbpool_t));
}


/**
 *
 * if (zdbpool_execute(&db_pool, "select * from some_table", db_pool.errmsg) != 0) {
 *     LOGGER_ERROR("dbpool_execute: (%s)", db_pool.errmsg);
 * }
 *
 */
__attribute__((used))
static int zdbpool_execute (zdbpool_t *dbPool, const char *sql, char errmsg[ZDBPOOL_ERRMSG_LEN + 1])
{
    Connection_T conn = 0;

    *dbPool->errmsg = 0;

    TRY
    {
        conn = ConnectionPool_getConnection(dbPool->pool);

        if (conn) {
            Connection_execute(conn, "%s", sql);

            return ZDBPOOL_SUCCESS;
        } else {
            snprintf(errmsg, ZDBPOOL_ERRMSG_LEN, "ConnectionPool is busy");
            errmsg[ZDBPOOL_ERRMSG_LEN] = 0;
        }
    }
    CATCH(SQLException) {
        snprintf(errmsg, ZDBPOOL_ERRMSG_LEN, "SQLException: %s", Exception_frame.message);
        errmsg[ZDBPOOL_ERRMSG_LEN] = 0;
    }
    FINALLY {
        if (conn) {
            Connection_close(conn);
        }
    }
    END_TRY;

    return ZDBPOOL_ERROR;
}


#if defined(__cplusplus)
}
#endif

#endif /* ZDBPOOL_H_INCLUDED */
