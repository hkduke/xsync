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
 * redis_conn.c
 *   redis client connection implementation
 *
 * @author: master@pepstack.com
 *
 * @version: 0.0.6
 *
 * @create: 2018-02-10
 *
 * @update: 2018-08-14 14:53:12
 *
 */

#include "redis_conn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


__attribute__((used))
int RedisConnInitiate(RedisConn_t * redconn, int maxNodes, const char * password, int conn_timeo_ms, int data_timeo_ms)
{
    size_t authlen = 0;

    if (password) {
        authlen = strlen(password);
        if (authlen >= sizeof(redconn->password)) {
            return -1;
        }
    }

    bzero(redconn, sizeof(RedisConn_t));

    redconn->nodes = (RedisNode_t *) calloc(maxNodes, sizeof(RedisNode_t));
    redconn->num = maxNodes;
    do {
        int i;
        for (i = 0; i < maxNodes; ++i) {
            redconn->nodes[i].index = i;
        }
    } while (0);

    if (conn_timeo_ms) {
        redconn->timeo_conn.tv_sec = conn_timeo_ms / 1000;
        redconn->timeo_conn.tv_usec = conn_timeo_ms * 1000;
    }

    if (data_timeo_ms) {
        redconn->timeo_data.tv_sec = data_timeo_ms / 1000;
        redconn->timeo_data.tv_usec = data_timeo_ms * 1000;
    }

    if (authlen) {
        memcpy(redconn->password, password, authlen);
    }

    return 0;
}


__attribute__((used))
void RedisConnRelease(RedisConn_t * redconn)
{
    while (redconn->num-- > 0) {
        char * hp = redconn->nodes[redconn->num].host;

        RedisConnCloseNode(redconn, redconn->num);

        if ( hp ) {
            redconn->nodes[redconn->num].host = 0;

            free( hp );
        }
    }

    free(redconn->nodes);

    redconn->nodes = 0;
}


__attribute__((used))
void RedisConnSetNode(RedisConn_t * redconn, int nodeIndex, const char *host, int port)
{
    assert(redconn->nodes[nodeIndex].index == nodeIndex);

    redconn->nodes[nodeIndex].host = (char *) calloc(strlen(host) + 1, sizeof(char));
    memcpy(redconn->nodes[nodeIndex].host, host, strlen(host));

    redconn->nodes[nodeIndex].port = port;
}


void RedisConnCloseNode(RedisConn_t * redconn, int index)
{
    redisContext * ctx = redconn->nodes[index].redctx;
    redconn->nodes[index].redctx = 0;
    if (ctx) {
        if (redconn->active_node == &(redconn->nodes[index])) {
            redconn->active_node = 0;
        }
        redisFree(ctx);
    }
}


redisContext * RedisConnOpenNode(RedisConn_t * redconn, int index)
{
    RedisConnCloseNode(redconn, index);

    do {
        RedisNode_t *node = &(redconn->nodes[index]);
        redisContext * ctx = 0;

        if (redconn->timeo_conn.tv_sec || redconn->timeo_conn.tv_usec) {
            ctx = redisConnectWithTimeout(node->host, node->port, redconn->timeo_conn);
        } else {
            ctx = redisConnect(node->host, node->port);
        }

        if (ctx) {
            if (redconn->password[0]) {
                redisReply * reply = (redisReply *) redisCommand(ctx, "AUTH %s", redconn->password);
                if (reply) {
                    // Authorization success
                    freeReplyObject(reply);
                } else {
                    // Authorization failed
                    redisFree(ctx);
                    ctx = 0;
                }
            }

            if (ctx) {
                if (redconn->timeo_data.tv_sec || redconn->timeo_data.tv_usec) {
                    if (redisSetTimeout(ctx, redconn->timeo_data) == REDIS_ERR) {
                        // set timeout failed
                        redisFree(ctx);
                        ctx = 0;
                    }
                }
            }

            if (ctx) {
                node->redctx = ctx;
                redconn->active_node = node;
            }
        }

        return node->redctx;
    } while (0);

    /* never run to this. */
    return 0;
}


redisContext * RedisConnGetActiveContext(RedisConn_t * redconn, const char *host, int port)
{
    int index;

    if (! host) {
        if (redconn->active_node && redconn->active_node->redctx) {
            return redconn->active_node->redctx;
        } else {
            // 找到第一个活动节点
            redconn->active_node = 0;

            for (index = 0; index < redconn->num; index++) {
                if (redconn->nodes[index].redctx) {
                    redconn->active_node = &(redconn->nodes[index]);
                    return redconn->active_node->redctx;
                }
            }
        }

        // 全部节点都不是活的, 创建连接. 遇到第一个成功的就返回
        for (index = 0; index < redconn->num; index++) {
            if (RedisConnOpenNode(redconn, index)) {
                return redconn->active_node->redctx;
            }
        }
    } else {
        for (index = 0; index < redconn->num; index++) {
            RedisNode_t * node = &(redconn->nodes[index]);

            if (node->port == port && ! strcmp(node->host, host)) {
                if (node->redctx) {
                    redconn->active_node = node;
                    return redconn->active_node->redctx;
                } else {
                    return RedisConnOpenNode(redconn, index);
                }
            }
        }
    }

    // 没有连接可以使用
    return 0;
}


/**
 * https://github.com/redis/hiredis
 *   The return value of redisCommand holds a reply when the command was
 *   successfully executed. When an error occurs, the return value is NULL
 *   and the err field in the context will be set (see section on Errors).
 *
 *   Once an error is returned the context cannot be reused and you should
 *   set up a new connection.
 */
redisReply * RedisConnCommand(RedisConn_t * redconn, int argc, const char **argv, const size_t *argvlen)
{
    redisReply * reply = 0;

    redisContext *ctx = RedisConnGetActiveContext(redconn, 0, 0);
    if (ctx == 0) {
        return 0;
    }

    reply = (redisReply *) redisCommandArgv(ctx, argc, argv, argvlen);

    if (! reply) {
        // 执行失败, 连接不能再被使用. 必须建立新连接!!
        RedisConnCloseNode(redconn, redconn->active_node->index);
        return 0;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        /**
         * REDIS_REPLY_STRING=1
         * REDIS_REPLY_ARRAY=2
         * REDIS_REPLY_INTEGER=3
         * REDIS_REPLY_NIL=4
         * REDIS_REPLY_STATUS=5
         * REDIS_REPLY_ERROR=6
         */

        if (reply->len > 5) {
            // 'MOVED 7142 127.0.0.1:7002'
            if (strstr(reply->str, "MOVED ")) {
                char * start = &(reply->str[6]);
                char * end = strchr(start, 32);

                if (end) {
                    start = end;
                    ++start;
                    *end = 0;

                    end = strchr(start, ':');

                    if (end) {
                        *end++ = 0;

                        /**
                         * start => host
                         * end => port
                         */
                        printf("Redirected to slot [%s] located at [%s:%s]\n", &reply->str[6], start, end);

                        ctx = RedisConnGetActiveContext(redconn, start, atoi(end));

                        if (ctx) {
                            freeReplyObject(reply);

                            return RedisConnCommand(redconn, argc, argv, argvlen);
                        }
                    }
                }
            }
        }
    }

    return reply;
}
