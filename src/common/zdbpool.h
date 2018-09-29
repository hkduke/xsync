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
 * @file: zdbpool.h
 *
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.8
 *
 * @create: 2015-07-21
 *
 * @update: 2018-08-10 18:11:59
 */

#ifndef ZDBPOOL_H_INCLUDED
#define ZDBPOOL_H_INCLUDED


#if defined(__cplusplus)
extern "C" {
#endif

#define ZDBPOOL_ERRMSG_LEN  1200


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
