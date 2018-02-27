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

#ifndef DBPOOL_H_INCLUDED
#define DBPOOL_H_INCLUDED


#if defined(__cplusplus)
extern "C" {
#endif

#define DBPOOL_ERRMSG_LEN  255


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

#define DBPOOL_SUCCESS         0
#define DBPOOL_ERROR         (-1)


#define DBPOOL_INIT_SIZE         5
#define DBPOOL_MAX_SIZE         20

#define DBPOOL_REAP_INTERVAL    30
#define DBPOOL_CONN_TIMEOUT     30


typedef struct dbpool_t
{
    ConnectionPool_T pool;

    char errmsg[DBPOOL_ERRMSG_LEN + 1];
} dbpool_t;


/**
 * connURI:
 *    "mysql://127.0.0.1/xsync?user=root&password=Abc123"
 *
 * poolSize:
 *    20
 */
__attribute__((used))
static void dbpool_init (dbpool_t *dbPool, void(*errorHandler)(const char *error),
    const char * connURI, int initPoolSize, int maxPoolSize, int reapSweepInterval, int connectionTimeout)
{
    ConnectionPool_T pool;

    bzero(dbPool, sizeof(dbpool_t));

    pool = ConnectionPool_new(URL_new(connURI));

    if (initPoolSize == 0) {
        initPoolSize = DBPOOL_INIT_SIZE;
    }

    if (maxPoolSize == 0) {
        maxPoolSize = DBPOOL_MAX_SIZE;
    }

    if (reapSweepInterval == 0) {
        reapSweepInterval = DBPOOL_REAP_INTERVAL;
    }

    if (connectionTimeout == 0) {
        connectionTimeout = DBPOOL_CONN_TIMEOUT;
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

    snprintf(dbPool->errmsg, DBPOOL_ERRMSG_LEN, "libzdb version: %s", ConnectionPool_version());
    dbPool->errmsg[DBPOOL_ERRMSG_LEN] = 0;

    dbPool->pool = pool;
}


__attribute__((used))
static void dbpool_end (dbpool_t *dbPool)
{
    URL_T url = ConnectionPool_getURL(dbPool->pool);

    ConnectionPool_stop(dbPool->pool);
    ConnectionPool_free(&dbPool->pool);

    URL_free(&url);

    bzero(dbPool, sizeof(dbpool_t));
}


/**
 *
 * if (dbpool_execute(&db_pool, "select * from some_table", db_pool.errmsg) != 0) {
 *     LOGGER_ERROR("dbpool_execute: (%s)", db_pool.errmsg);
 * }
 *
 */
__attribute__((used))
static int dbpool_execute (dbpool_t *dbPool, const char *sql, char errmsg[DBPOOL_ERRMSG_LEN + 1])
{
    Connection_T conn = 0;

    *dbPool->errmsg = 0;

    TRY
    {
        conn = ConnectionPool_getConnection(dbPool->pool);

        if (conn) {
            Connection_execute(conn, "%s", sql);

            return DBPOOL_SUCCESS;
        } else {
            snprintf(errmsg, DBPOOL_ERRMSG_LEN, "ConnectionPool is busy");
            errmsg[DBPOOL_ERRMSG_LEN] = 0;
        }
    }
    CATCH(SQLException) {
        snprintf(errmsg, DBPOOL_ERRMSG_LEN, "SQLException: %s", Exception_frame.message);
        errmsg[DBPOOL_ERRMSG_LEN] = 0;
    }
    FINALLY {
        if (conn) {
            Connection_close(conn);
        }
    }
    END_TRY;

    return DBPOOL_ERROR;
}


#if defined(__cplusplus)
}
#endif

#endif /* DBPOOL_H_INCLUDED */
