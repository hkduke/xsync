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
 * @version: 0.0.7
 *
 * @create: 2018-02-10
 *
 * @update: 2018-08-16 10:44:16
 *
 */

#include "redis_conn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>


__attribute__((used))
static char * trim(char *s, char c)
{
    return (*s==0)?s:(((*s!=c)?(((trim(s+1,c)-1)==s)?s:(*(trim(s+1,c)-1)=*s,*s=c,trim(s+1,c))):trim(s+1,c)));
}


__attribute__((used))
int RedisConnInitiate(RedisConn_t * redconn, int maxNodes, const char * password, int conn_timeo_ms, int data_timeo_ms)
{
    size_t authlen = 0;

    if (password) {
        authlen = strlen(password);
        if (authlen >= sizeof(redconn->password)) {
            snprintf(redconn->errmsg, sizeof(redconn->errmsg), "%s", "password is too long");
            redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
            return -1;
        }
    }

    if (maxNodes < 1) {
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "invalid nodes(=%d)", maxNodes);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        return -1;
    }

    if (maxNodes > 20) {
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "too many nodes(=%d)", maxNodes);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        return -1;
    }

    bzero(redconn, sizeof(RedisConn_t));

    redconn->nodes = (RedisNode_t *) calloc(maxNodes, sizeof(RedisNode_t));
    redconn->num = maxNodes;
    do {
        int i;
        for (i = 0; i < maxNodes; ++i) {
            redconn->nodes[i].index = -1;
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
int RedisConnInitiate2(RedisConn_t * redconn, const char * host_post_pairs, const char * password, int conn_timeo_ms, int data_timeo_ms)
{
    /**
     * host_post_pairs="127.0.0.1:7001,127.0.0.1:7002,127.0.0.1:7003,127.0.0.1:7004,127.0.0.1:7005,127.0.0.1:7006,127.0.0.1:7007,127.0.0.1:7008,127.0.0.1:7009"
     */
    char *buf, *pairs, *hp, *port;

    int nodes, rc = -1;

    size_t len = strlen(host_post_pairs);

    buf = (char*) malloc(len + 1);

    memcpy(buf, host_post_pairs, len);
    buf[len] = 0;
    pairs = trim(buf, 32);

    nodes = 0;
    hp = strtok(pairs, ",");
    while (hp) {
        port = strchr(hp, ':');
        if (! port) {
            free(buf);

            snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad argument: port not found");
            redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

            return -2;
        }
        *port++ = 0;
        while(*port) {
            if (! isdigit(*port++)) {
                free(buf);

                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad argument: invalid port");
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

                return -2;
            }
        }
        ++nodes;
        hp = strtok(0, ",");
    }

    rc = RedisConnInitiate(redconn, nodes, password, conn_timeo_ms, data_timeo_ms);
    if (rc != 0) {
        free(buf);
        return rc;
    }

    memcpy(buf, host_post_pairs, len);
    buf[len] = 0;
    pairs = trim(buf, 32);

    nodes = 0;
    hp = strtok(pairs, ",");
    while (hp) {
        port = strchr(hp, ':');
        *port++ = 0;
        if (RedisConnSetNode(redconn, nodes++, hp, atoi(port)) != 0) {
            free(buf);
            return -1;
        }
        hp = strtok(0, ",");
    }

    free(buf);

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
int RedisConnSetNode(RedisConn_t * redconn, int nodeIndex, const char *host, int port)
{
    int i = 0;

    if (redconn->nodes[nodeIndex].index != -1) {
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "node already existed (index=%d)", nodeIndex);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        return -1;
    }

    for (i = 0; i < redconn->num; ++i) {
        if (redconn->nodes[i].index == i) {
            if (redconn->nodes[i].port == port && ! strcmp(redconn->nodes[i].host, host)) {
                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "duplicated node(%s:%d)", host, port);
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
                return -2;
            }
        }
    }

    assert(redconn->nodes[nodeIndex].index == -1);

    redconn->nodes[nodeIndex].host = (char *) calloc(strlen(host) + 1, sizeof(char));
    memcpy(redconn->nodes[nodeIndex].host, host, strlen(host));

    redconn->nodes[nodeIndex].port = port;
    redconn->nodes[nodeIndex].index = nodeIndex;

    return 0;
}


__attribute__((used))
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


__attribute__((used))
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
    snprintf(redconn->errmsg, sizeof(redconn->errmsg), "should never run to this.");
    redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
    return 0;
}


__attribute__((used))
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
    snprintf(redconn->errmsg, sizeof(redconn->errmsg), "no active context.");
    redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
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
__attribute__((used))
redisReply * RedisConnExecCommand(RedisConn_t * redconn, int argc, const char **argv, const size_t *argvlen)
{
    redisReply * reply = 0;

    redisContext *ctx = RedisConnGetActiveContext(redconn, 0, 0);
    if (ctx == 0) {
        return 0;
    }

    reply = (redisReply *) redisCommandArgv(ctx, argc, argv, argvlen);

    if (! reply) {
        // 执行失败, 连接不能再被使用. 必须建立新连接!!
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "redisCommandArgv failed: bad context.");
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
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

        /* 'MOVED 7142 127.0.0.1:7002' */
        if (! strncmp(reply->str, "MOVED ", 6)) {
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
                    ctx = RedisConnGetActiveContext(redconn, start, atoi(end));

                    if (ctx) {
                        freeReplyObject(reply);
                        return RedisConnExecCommand(redconn, argc, argv, argvlen);
                    }
                }
            }
        } else if (! strncmp(reply->str, "NOAUTH ", 7)) {
            /* 'NOAUTH Authentication required.' */
            const char * cmds[] = {
                "AUTH",
                redconn->password
            };

            freeReplyObject(reply);

            reply = RedisConnExecCommand(redconn, sizeof(cmds)/sizeof(cmds[0]), cmds, 0);

            if (reply &&
                reply->type == REDIS_REPLY_STATUS &&
                reply->len == 2 &&
                reply->str[0] == 'O' && reply->str[1] == 'K') {
                // authentication success
                freeReplyObject(reply);

                // do command
                return RedisConnExecCommand(redconn, argc, argv, argvlen);
            }

            if (reply) {
                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "AUTH failed(%d): %s", reply->type, reply->str);
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
                freeReplyObject(reply);
            }

            return 0;
        }

        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "TODO: REDIS_REPLY_ERROR: %s", reply->str);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        freeReplyObject(reply);
        return 0;
    }

    return reply;
}


__attribute__((used))
void RedisFreeReplyObject(redisReply **ppreply)
{
    redisReply *reply = *ppreply;
    if (reply) {
        freeReplyObject(reply);
        *ppreply = 0;
    }
}
