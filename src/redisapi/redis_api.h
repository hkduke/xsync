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
 * redis_api.h
 *   redis 集群同步 API
 *
 * @author: master@pepstack.com
 *
 * Reference:
 *    https://github.com/redis/hiredis
 *    https://redislabs.com/lp/hiredis/
 *    https://github.com/aclisp/hiredispool
 *    https://github.com/eyjian/r3c/blob/master/r3c.cpp
 *    http://www.mamicode.com/info-detail-501902.html
 *
 * 异步:
 *    https://blog.csdn.net/l1902090/article/details/38583663
 *
 * @version: 0.1.1
 *
 * @create: 2018-02-10
 *
 * @update: 2018-08-29 13:09:01
 *
 */
#ifndef REDIS_CONN_SYN_H_INCLUDED
#define REDIS_CONN_SYN_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#define REDISAPI_SUCCESS   0
#define REDISAPI_NOERROR   REDISAPI_SUCCESS

#define REDISAPI_ERROR     (-1)
#define REDISAPI_EINDEX    (-2)
#define REDISAPI_EARG      (-3)
#define REDISAPI_ERETVAL   (-4)
#define REDISAPI_ETYPE     (-5)
#define REDISAPI_EAUTH     (-6)


typedef struct RedisAsynNode_t
{
    /**
     * redisAsyncContext is NOT thread-safe
     */
    redisAsyncContext *redAsynCtx;

    int index;
    int slot;
    int  port;

    char *host;
} RedisAsynNode_t;


typedef struct RedisSynNode_t
{
    /**
     * redisContext is NOT thread-safe
     */
    redisContext *redCtx;

    int index;
    int slot;
    int  port;

    char *host;
} RedisSynNode_t;


/**
 * redis 同步连接
 */
typedef struct RedisConn_t
{
    RedisSynNode_t *active_node;

    struct timeval timeo_conn;
    struct timeval timeo_data;

    char password[33];
    char errmsg[128];

    int num;
    RedisSynNode_t *nodes;
} RedisConn_t;


/**
 * redis 异步连接
 */
typedef struct RedisAsynConn_t
{
    RedisAsynNode_t *active_node;

    struct timeval timeo_conn;
    struct timeval timeo_data;

    char password[33];
    char errmsg[128];

    int num;
    RedisAsynNode_t *nodes;
} RedisAsynConn_t;


/**
 * 同步 API
 * low level functions
 */

extern int RedisConnInit(RedisConn_t * redconn, int nodesNum, const char * password, int conn_timeo_ms, int data_timeo_ms);

extern int RedisConnInit2(RedisConn_t * redconn, const char * host_post_pairs, const char * password, int conn_timeo_ms, int data_timeo_ms);

extern void RedisConnFree(RedisConn_t * redconn);

extern int RedisConnSetNode(RedisConn_t * redconn, int nodeIndex, const char *host, int port);

extern void RedisConnCloseNode(RedisConn_t * redconn, int nodeIndex);

extern redisContext * RedisConnOpenNode(RedisConn_t * redconn, int nodeIndex);

extern redisContext * RedisConnGetActiveContext(RedisConn_t * redconn, const char *host, int port);

extern redisReply * RedisConnExecCommand(RedisConn_t * redconn, int argc, const char **argv, const size_t *argvlen);

extern void RedisFreeReplyObject(redisReply **reply);

/**
 * 同步 API
 * high level functions
 */

// 设置 key 过期时间(毫秒): expire_ms=0 忽略, expire_ms=-1 永不过期, expire_ms>0 过期的毫秒时间
extern int RedisExpireSet(RedisConn_t * redconn, const char *key, int64_t expire_ms);

extern int RedisHashMultiSet(RedisConn_t * redconn, const char *key, int num, const char * fields[], const char *values[], const size_t *valueslen, int64_t expire_ms);


#if defined(__cplusplus)
}
#endif

#endif /* REDIS_CONN_SYN_H_INCLUDED */