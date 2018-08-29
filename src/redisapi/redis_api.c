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
 * redis_api.c
 *   redis 同步/异步 API
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.1
 *
 * @create: 2018-02-10
 *
 * @update: 2018-08-29 13:09:01
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "redis_api.h"


__attribute__((used))
static char * trim(char *s, char c)
{
    return (*s==0)?s:(((*s!=c)?(((trim(s+1,c)-1)==s)?s:(*(trim(s+1,c)-1)=*s,*s=c,trim(s+1,c))):trim(s+1,c)));
}


__attribute__((used))
int RedisConnInit(RedisConn_t * redconn, int maxNodes, const char * password, int conn_timeo_ms, int data_timeo_ms)
{
    size_t authlen = 0;

    if (password) {
        authlen = strlen(password);
        if (authlen >= sizeof(redconn->password)) {
            snprintf(redconn->errmsg, sizeof(redconn->errmsg), "%s", "password is too long");
            redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
            return REDISAPI_EARG;
        }
    }

    if (maxNodes < 1) {
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "invalid nodes(=%d)", maxNodes);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        return REDISAPI_EARG;
    }

    if (maxNodes > 20) {
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "too many nodes(=%d)", maxNodes);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        return REDISAPI_EARG;
    }

    bzero(redconn, sizeof(RedisConn_t));

    redconn->nodes = (RedisSynNode_t *) calloc(maxNodes, sizeof(RedisSynNode_t));
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

    return REDISAPI_SUCCESS;;
}


__attribute__((used))
int RedisConnInit2(RedisConn_t * redconn, const char * host_post_pairs, const char * password, int conn_timeo_ms, int data_timeo_ms)
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

            return REDISAPI_EARG;
        }
        *port++ = 0;
        while(*port) {
            if (! isdigit(*port++)) {
                free(buf);

                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad argument: invalid port");
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

                return REDISAPI_EARG;
            }
        }
        ++nodes;
        hp = strtok(0, ",");
    }

    rc = RedisConnInit(redconn, nodes, password, conn_timeo_ms, data_timeo_ms);
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

    return REDISAPI_SUCCESS;
}


__attribute__((used))
void RedisConnFree(RedisConn_t * redconn)
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
        return REDISAPI_EINDEX;
    }

    for (i = 0; i < redconn->num; ++i) {
        if (redconn->nodes[i].index == i) {
            if (redconn->nodes[i].port == port && ! strcmp(redconn->nodes[i].host, host)) {
                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "duplicated node(%s:%d)", host, port);
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
                return REDISAPI_EARG;
            }
        }
    }

    assert(redconn->nodes[nodeIndex].index == -1);

    redconn->nodes[nodeIndex].host = (char *) calloc(strlen(host) + 1, sizeof(char));
    memcpy(redconn->nodes[nodeIndex].host, host, strlen(host));

    redconn->nodes[nodeIndex].port = port;
    redconn->nodes[nodeIndex].index = nodeIndex;

    return REDISAPI_SUCCESS;
}


__attribute__((used))
void RedisConnCloseNode(RedisConn_t * redconn, int index)
{
    redisContext * ctx = redconn->nodes[index].redCtx;
    redconn->nodes[index].redCtx = 0;
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
        RedisSynNode_t *node = &(redconn->nodes[index]);
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
                node->redCtx = ctx;
                redconn->active_node = node;
            }
        }

        return node->redCtx;
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
        if (redconn->active_node && redconn->active_node->redCtx) {
            return redconn->active_node->redCtx;
        } else {
            // 找到第一个活动节点
            redconn->active_node = 0;

            for (index = 0; index < redconn->num; index++) {
                if (redconn->nodes[index].redCtx) {
                    redconn->active_node = &(redconn->nodes[index]);
                    return redconn->active_node->redCtx;
                }
            }
        }

        // 全部节点都不是活的, 创建连接. 遇到第一个成功的就返回
        for (index = 0; index < redconn->num; index++) {
            if (RedisConnOpenNode(redconn, index)) {
                return redconn->active_node->redCtx;
            }
        }
    } else {
        for (index = 0; index < redconn->num; index++) {
            RedisSynNode_t * node = &(redconn->nodes[index]);

            if (node->port == port && ! strcmp(node->host, host)) {
                if (node->redCtx) {
                    redconn->active_node = node;
                    return redconn->active_node->redCtx;
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


__attribute__((used))
int RedisExpireSet(RedisConn_t * redconn, const char *key, int64_t expire_ms)
{
    if (expire_ms == 0) {
        // 忽略过期时间
        // success: no expire time set
        return REDISAPI_SUCCESS;
    } else if (expire_ms > 0) {
        // 设置过期时间(毫秒): PEXPIRE key expire_ms
        redisReply *reply;

        char str_expire_ms[21];

        snprintf(str_expire_ms, sizeof(str_expire_ms), "%ld", expire_ms);
        str_expire_ms[20] = 0;

        const char * pexpire[] = {"PEXPIRE", key, str_expire_ms};

        reply = RedisConnExecCommand(redconn, sizeof(pexpire) / sizeof(pexpire[0]), pexpire, 0);

        if (! reply) {
            // error
            return REDISAPI_ERROR;
        }

        if (reply->type == REDIS_REPLY_INTEGER) {
            if (reply->integer == 1) {
                // success
                RedisFreeReplyObject(&reply);
                return REDISAPI_SUCCESS;
            } else {
                // bad reply integer
                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad reply value(=%lld), required(1)", reply->integer);
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

                RedisFreeReplyObject(&reply);
                return REDISAPI_ERETVAL;
            }
        }

        // bad reply type
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad reply type(=%d), required REDIS_REPLY_INTEGER(%d)", reply->type, REDIS_REPLY_INTEGER);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

        RedisFreeReplyObject(&reply);
        return REDISAPI_ETYPE;
    } else if (expire_ms == -1) {
        // 永不过期: PERSIST key
        redisReply *reply;

        const char * persist[] = {"PERSIST", key};

        reply = RedisConnExecCommand(redconn, sizeof(persist) / sizeof(persist[0]), persist, 0);

        if (! reply) {
            // error
            return REDISAPI_ERROR;
        }

        if (reply->type == REDIS_REPLY_INTEGER) {
            if (reply->integer == -1) {
                // success
                RedisFreeReplyObject(&reply);
                return REDISAPI_SUCCESS;
            } else {
                // bad reply integer
                snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad reply value(=%lld), required(-1)", reply->integer);
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

                RedisFreeReplyObject(&reply);
                return REDISAPI_ERETVAL;
            }
        }

        // bad reply type
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "bad reply type(=%d), required REDIS_REPLY_INTEGER(%d)", reply->type, REDIS_REPLY_INTEGER);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

        RedisFreeReplyObject(&reply);
        return REDISAPI_ETYPE;
    } else {
        snprintf(redconn->errmsg, sizeof(redconn->errmsg), "invalid expire time: %ld", expire_ms);
        redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;
        return REDISAPI_EARG;
    }
}


/**
 * hmset key f1 v1 f2 v2
 * hdel key f3
 *
 *   const char *flds[] = {"f1", "f2", "f3", 0};
 *   const char *vals[] = {"v1", "v2", 0, 0};
 *
 *   RedisHashMultiSet(redconn, key, flds, vals, 0, 0);
 */
__attribute__((used))
int RedisHashMultiSet(RedisConn_t * redconn, const char *key, const char * fields[], const char *values[], const size_t *valueslen, int64_t expire_ms)
{
    int i, k, ret;
    int setargc = 0;
    int delargc = 0;
    int argc = 0;

    const char ** pfld = &fields[0];
    const char ** pval = &values[0];

    while (*pfld++) {
        if (*pval++) {
            ++setargc;
        } else {
            ++delargc;
        }
    }

    argc = (setargc * 2 + 2) + (delargc + 2);

    if (argc > 0) {
        redisReply *reply;
        const char **argv;
        size_t *argvlen;

        void *pv = calloc( argc, sizeof(size_t) + sizeof(char *) );
        if (! pv) {
            return REDISAPI_EMEM;
        }

        argvlen = (size_t *) pv;
        argv = (const char **) (& argvlen[argc]);

        if (setargc > 0) {
            // hmset key f1 v1 f2 v2 f3 v3
            const char hmset[] = "HMSET";

            argv[0] = hmset;  argvlen[0] = 5;
            argv[1] = key;    argvlen[1] = strlen(key);

            pfld = &fields[0];
            pval = &values[0];

            i = k = 0;

            while (*pfld) {
                if (*pval) {
                    argv[i*2 + 2] = *pfld;
                    argvlen[i*2 + 2] = strlen(*pfld);

                    argv[i*2 + 3] = *pval;
                    argvlen[i*2 + 3] = (valueslen? valueslen[k] : strlen(*pval));

                    ++i;
                }

                ++k;
                ++pfld;
                ++pval;
            }

            setargc = i * 2 + 2;

            reply = RedisConnExecCommand(redconn, setargc, argv, argvlen);

            if (! reply) {
                // error
                free(pv);
                return REDISAPI_ERROR;
            }

            if (reply->type == REDIS_REPLY_STATUS &&
                reply->len == 2 &&
                reply->str[0] == 'O' && reply->str[1] == 'K') {
                // success
                RedisFreeReplyObject(&reply);

                // set time out for key
                ret = RedisExpireSet(redconn, key, expire_ms);
                if (ret != REDISAPI_SUCCESS) {
                    free(pv);
                    return ret;
                }
            } else {
                // bad reply type
                snprintf(redconn->errmsg, sizeof(redconn->errmsg),
                    "bad reply type(=%d), required REDIS_REPLY_STATUS(%d)",
                    reply->type, REDIS_REPLY_STATUS);
                redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

                RedisFreeReplyObject(&reply);

                free(pv);
                return REDISAPI_ETYPE;
            }
        }

        if (delargc > 0) {
            // hdel key f1 f2 f3
            const char hdel[] = "HDEL";

            argv[0] = hdel;  argvlen[0] = 4;
            argv[1] = key;   argvlen[1] = strlen(key);

            pfld = &fields[0];
            pval = &values[0];

            i = k = 0;

            while (*pfld) {
                if (! *pval) {
                    argv[i + 2] = *pfld;
                    argvlen[i + 2] = strlen(*pfld);

                    ++i;
                }

                ++k;
                ++pfld;
                ++pval;
            }

            delargc = i + 2;

            reply = RedisConnExecCommand(redconn, delargc, argv, argvlen);

            free(pv);

            if (! reply) {
                // error
                return REDISAPI_ERROR;
            }

            if ( reply->type == REDIS_REPLY_INTEGER &&
                (reply->integer == 1 || reply->integer == 0) ) {
                // success
                RedisFreeReplyObject(&reply);
                return REDISAPI_SUCCESS;
            }

            // bad reply type
            snprintf(redconn->errmsg, sizeof(redconn->errmsg),
                "bad reply type(=%d), required REDIS_REPLY_INTEGER(%d)",
                reply->type, REDIS_REPLY_INTEGER);
            redconn->errmsg[ sizeof(redconn->errmsg) - 1 ] = 0;

            RedisFreeReplyObject(&reply);
            return REDISAPI_ETYPE;
        }

        free(pv);
        return REDISAPI_SUCCESS;
    }

    return REDISAPI_EARG;
}
