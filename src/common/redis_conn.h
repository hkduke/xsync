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
 * redis_conn.h
 *   redis client connection api
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
 * @version: 0.0.9
 *
 * @create: 2018-02-10
 *
 * @update: 2018-08-16 10:44:16
 *
 */
#ifndef REDIS_CONN_H_INCLUDED
#define REDIS_CONN_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <hiredis/hiredis.h>

typedef struct RedisNode_t
{
    redisContext *redctx;

    int index;
    int slot;
    int  port;

    char *host;
} RedisNode_t;


typedef struct RedisConn_t
{
    RedisNode_t *active_node;

    struct timeval timeo_conn;
    struct timeval timeo_data;

    char password[33];

    char errmsg[128];

    int num;
    RedisNode_t *nodes;
} RedisConn_t;


/**
 * low level functions
 */

extern int RedisConnInitiate(RedisConn_t * redconn, int nodesNum, const char * password, int conn_timeo_ms, int data_timeo_ms);

extern int RedisConnInitiate2(RedisConn_t * redconn, const char * host_post_pairs, const char * password, int conn_timeo_ms, int data_timeo_ms);

extern void RedisConnRelease(RedisConn_t * redconn);

extern int RedisConnSetNode(RedisConn_t * redconn, int nodeIndex, const char *host, int port);

extern void RedisConnCloseNode(RedisConn_t * redconn, int nodeIndex);

extern redisContext * RedisConnOpenNode(RedisConn_t * redconn, int nodeIndex);

extern redisContext * RedisConnGetActiveContext(RedisConn_t * redconn, const char *host, int port);

extern redisReply * RedisConnExecCommand(RedisConn_t * redconn, int argc, const char **argv, const size_t *argvlen);

extern void RedisFreeReplyObject(redisReply **reply);

/**
 * high level functions
 */
extern int RedisHMSET(RedisConn_t * redconn, const char *key, int num, const char * fields[], const char *values[], const size_t *valueslen, int64_t expire_ms);


#if defined(__cplusplus)
}
#endif

#endif /* REDIS_CONN_H_INCLUDED */
