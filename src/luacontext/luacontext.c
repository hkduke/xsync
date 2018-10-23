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
 * luacontext.c
 *   lua with C interop helper
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.6
 *
 * @create: 2018-10-15
 *
 * @update: 2018-10-22 10:56:41
 *
 */

/* lua stack:
 *   https://blog.csdn.net/qweewqpkn/article/details/46806731
 *   https://www.ibm.com/developerworks/cn/linux/l-lua.html
 *
 * top     +-----------------+
 *      4  |                 |  -1
 *         +-----------------+
 *      3  |                 |  -2
 *         +-----------------+
 *      2  |                 |  -3
 *         +-----------------+
 *      1  |                 |  -4
 * bottom  +-----------------+
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "luacontext.h"


/**
 * Creating a single lua_State per thread is a good solution
 *  to having multiple threads of Lua execution.
 */
typedef struct lua_context_t
{
    int thread_mode;

    pthread_mutex_t lock;

    /* initialize Lua */
    lua_State * L;

    /* 保存错误信息 */
    char error[LUACTX_ERROR_MAXLEN + 1];

    /**
     * 静态返回值输出表: 当前不支持动态返回值输出表!!
     */

    /* 输出的 key-value 对数目: 默认为 0 */
    int kv_pairs;

    /* 输出的 key 名称偏移: out_keys_buffer */
    int keys_offset[LUACTX_PAIRS_MAXNUM + 2];

    /* 输出的 value 偏移: out_values_buffer */
    int values_offset[LUACTX_PAIRS_MAXNUM + 2];

    /* 存储所有输出的 key 名称 */
    char keys_buffer[LUACTX_KEYS_BUFSIZE];

    /* 存储所有输出的 value 值 */
    char values_buffer[LUACTX_VALUES_BUFSIZE];
} lua_context_t;


int LuaCtxNew (const char *scriptfile, int threadmode, lua_context *outctx)
{
    int err;
    lua_State * L;

    lua_context ctx = (lua_context) malloc(sizeof(lua_context_t));
    if (! ctx) {
        *outctx = 0;
        return LUACTX_OUT_MEMORY;
    }

    *outctx = 0;

    bzero(ctx, sizeof(lua_context_t));

    ctx->thread_mode = threadmode;

    /* initialize Lua */
    L = luaL_newstate();
    if (! L) {
        free(ctx);
        return LUACTX_E_L_NEWSTATE;
    }

    /*load Lua base libraries*/
    luaL_openlibs(L);

    /* luaL_loadfile
     *   PANIC: unprotected error in call to Lua API (attempt to call a nil value)
     */
    err = luaL_loadfile(L, scriptfile);
    if (err) {
        lua_close(L);
        free(ctx);
        return LUACTX_E_L_LOADFILE;
    }

    /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
    if (lua_pcall(L, 0, 0, 0)) {
        lua_close(L);
        free(ctx);
        return LUACTX_E_L_PCALL;
    }

    if (ctx->thread_mode) {
        if (pthread_mutex_init(&ctx->lock, 0) != 0) {
            lua_close(L);
            free(ctx);
            return LUACTX_LOCK_ERROR;
        }
    }

    /* cleanup stack */
    lua_settop(L, 0);

    /* success */
    ctx->L = L;

    *outctx = ctx;

    return LUACTX_SUCCESS;
}


void LuaCtxFree (lua_context *pctx)
{
    lua_context ctx = *pctx;

    if (ctx) {
        *pctx = 0;

        lua_State *L = LuaCtxLockState(ctx);
        if (L) {
            ctx->L = 0;

            /* cleanup Lua */
            lua_close(L);

            if (ctx->thread_mode) {
                pthread_mutex_destroy(&ctx->lock);
            }
        }

        free(ctx);
    }
}


lua_State * LuaCtxLockState (lua_context ctx)
{
    if (! ctx) {
        return NULL;
    }

    if (! ctx->thread_mode) {
        return ctx->L;
    }

    if (pthread_mutex_lock(&ctx->lock) == 0) {
        if (ctx->L) {
            return ctx->L;
        }

        pthread_mutex_unlock(&ctx->lock);
    }

    return NULL;
}


void LuaCtxUnlockState (lua_context ctx)
{
    if (ctx && ctx->thread_mode) {
        pthread_mutex_unlock(&ctx->lock);
    }
}


const char * LuaCtxGetError (lua_context ctx)
{
    ctx->error[ LUACTX_ERROR_MAXLEN ] = '\0';
    return ctx->error;
}


int LuaCtxCall (lua_context ctx, const char *funcname, const char *key, const char *value)
{
    lua_State * L = ctx->L;

    ctx->kv_pairs = 0;
    ctx->keys_offset[0] = 0;
    ctx->values_offset[0] = 0;

    lua_settop(L, 0);

    lua_getglobal(L, funcname);                 /* Tell it to run callfuncscript.lua->tweaktable() */

    lua_newtable(L);                            /* Push empty table onto stack table now at -1 */

    lua_pushstring(L, key);          /* Push a key onto the stack, table now at -2 */
    lua_pushstring(L, value);        /* Push a value onto the stack, table now at -3 */

    lua_settable(L, -3);                    /* Take key and value, put into table at -3, */
                                                /*  then pop key and value so table again at -1 */

    /* Run function, !!! NRETURN=1 !!! */
    if ( lua_pcall(L, 1, 1, 0) ) {
        snprintf(ctx->error, sizeof(ctx->error), "lua_pcall fail: %s", lua_tostring(L, -1));
        return LUACTX_ERROR;
    }

    /* table is in the stack at index 't' */
    /* Make sure lua_next starts at beginning */
    lua_pushnil(L);

    while (lua_next(L, -2)) {                    /* TABLE LOCATED AT -2 IN STACK */
        const char *k, *v;
        int kcb, vcb, kcb_next, vcb_next;

        v = lua_tostring(L, -1);                 /* Value at stacktop */
        lua_pop(L, 1);                           /* Remove value */

        k = lua_tostring(L, -1);                 /* Read key at stacktop, */
                                                 /* leave in place to guide next lua_next() */

        kcb = (int) (k ? strlen(k) + 1 : 0);
        vcb = (int) (v ? strlen(v) + 1 : 0);

        kcb_next = ctx->keys_offset[ctx->kv_pairs] + kcb;
        vcb_next = ctx->values_offset[ctx->kv_pairs] + vcb;

        ctx->keys_offset[ctx->kv_pairs + 1] = kcb_next;
        ctx->values_offset[ctx->kv_pairs + 1] = vcb_next;

        if (kcb) {
            if (kcb_next < LUACTX_KEYS_BUFSIZE) {
                memcpy(ctx->keys_buffer + ctx->keys_offset[ctx->kv_pairs], k, kcb);
            } else {
                snprintf(ctx->error, sizeof(ctx->error), "too large out keys: more than %d bytes.", LUACTX_KEYS_BUFSIZE);
                return LUACTX_ERROR;
            }
        }

        if (vcb) {
            if (vcb_next < LUACTX_VALUES_BUFSIZE) {
                memcpy(ctx->values_buffer + ctx->values_offset[ctx->kv_pairs], v, vcb);
            } else {
                snprintf(ctx->error, sizeof(ctx->error), "too large out values: more than %d bytes.", LUACTX_VALUES_BUFSIZE);
                return LUACTX_ERROR;
            }
        }

        ctx->kv_pairs++;

        if (ctx->kv_pairs > LUACTX_PAIRS_MAXNUM) {
            snprintf(ctx->error, sizeof(ctx->error), "too many out pairs: more than %d.", LUACTX_PAIRS_MAXNUM);
            return LUACTX_ERROR;
        }
    }

    return LUACTX_SUCCESS;
}


int LuaCtxCallMany (lua_context ctx, const char *funcname, const char *keys[], const char *values[], int kv_pairs)
{
    int i = 0;

    lua_State * L = ctx->L;

    ctx->kv_pairs = 0;
    ctx->keys_offset[0] = 0;
    ctx->values_offset[0] = 0;

    lua_settop(L, 0);

    lua_getglobal(L, funcname);                 /* Tell it to run callfuncscript.lua->tweaktable() */

    lua_newtable(L);                            /* Push empty table onto stack table now at -1 */

    while (i < kv_pairs) {
        const char * ki = keys[i];
        const char * vi = values[i];

        lua_pushstring(L, ki);                  /* Push a key onto the stack, table now at -2 */
        lua_pushstring(L, vi);                  /* Push a value onto the stack, table now at -3 */

        lua_settable(L, -3);                    /* Take key and value, put into table at -3, */
                                                /*  then pop key and value so table again at -1 */
        ++i;
    }

    /* Run function, !!! NRETURN=1 !!! */
    if ( lua_pcall(L, 1, 1, 0) ) {
        snprintf(ctx->error, sizeof(ctx->error), "lua_pcall fail: %s", lua_tostring(L, -1));
        return LUACTX_ERROR;
    }

    /**
     * table is in the stack at index 't' Make sure lua_next starts at beginning
     */
    lua_pushnil(L);

    while (lua_next(L, -2)) {                    /* TABLE LOCATED AT -2 IN STACK */
        const char *k, *v;
        int kcb, vcb, kcb_next, vcb_next;

        v = lua_tostring(L, -1);                 /* Value at stacktop */
        lua_pop(L, 1);                           /* Remove value */

        k = lua_tostring(L, -1);                 /* Read key at stacktop, */
                                                 /* leave in place to guide next lua_next() */

        kcb = (int) (k ? strlen(k) + 1 : 0);
        vcb = (int) (v ? strlen(v) + 1 : 0);

        kcb_next = ctx->keys_offset[ctx->kv_pairs] + kcb;
        vcb_next = ctx->values_offset[ctx->kv_pairs] + vcb;

        ctx->keys_offset[ctx->kv_pairs + 1] = kcb_next;
        ctx->values_offset[ctx->kv_pairs + 1] = vcb_next;

        if (kcb) {
            if (kcb_next < LUACTX_KEYS_BUFSIZE) {
                memcpy(ctx->keys_buffer + ctx->keys_offset[ctx->kv_pairs], k, kcb);
            } else {
                snprintf(ctx->error, sizeof(ctx->error), "too large out keys: more than %d bytes.", LUACTX_KEYS_BUFSIZE);
                return LUACTX_ERROR;
            }
        }

        if (vcb) {
            if (vcb_next < LUACTX_VALUES_BUFSIZE) {
                memcpy(ctx->values_buffer + ctx->values_offset[ctx->kv_pairs], v, vcb);
            } else {
                snprintf(ctx->error, sizeof(ctx->error), "too large out values: more than %d bytes.", LUACTX_VALUES_BUFSIZE);
                return LUACTX_ERROR;
            }
        }

        ctx->kv_pairs++;

        if (ctx->kv_pairs > LUACTX_PAIRS_MAXNUM) {
            snprintf(ctx->error, sizeof(ctx->error), "too many out pairs: more than %d.", LUACTX_PAIRS_MAXNUM);
            return LUACTX_ERROR;
        }
    }

    return LUACTX_SUCCESS;
}


int LuaCtxNumPairs (lua_context ctx)
{
    return ctx->kv_pairs;
}


int LuaCtxGetKey (lua_context ctx, int index, char **outkey)
{
    if (index >= 0 && index < LUACTX_PAIRS_MAXNUM) {
        int start = ctx->keys_offset[index];
        int end = ctx->keys_offset[index + 1];
        *outkey = ctx->keys_buffer + start;
        return (end - start);
    } else {
        /* bad key */
        return 0;
    }
}


int LuaCtxGetValue (lua_context ctx, int index, char **outvalue)
{
    if (index >= 0 && index < LUACTX_PAIRS_MAXNUM) {
        int start = ctx->values_offset[index];
        int end = ctx->values_offset[index + 1];
        *outvalue = ctx->values_buffer + start;
        return (end - start);
    } else {
        /* bad value */
        return 0;
    }
}


int LuaCtxFindKey (lua_context ctx, const char *key, int keylen)
{
    int index;

    if (keylen == -1) {
        keylen = (key? strlen(key) : 0);
    }

    for (index = 0; index < ctx->kv_pairs; index++) {
        char *k;
        int kcb = LuaCtxGetKey(ctx, index, &k);

        if (kcb == keylen + 1) {
            if (! strncmp(key, k, keylen)) {
                return index;
            }
        }
    }

    return LUACTX_BAD_INDEX;
}


int LuaCtxGetValueByKey (lua_context ctx, const char *key, int keylen, char **outvalue)
{
    return LuaCtxGetValue(ctx, LuaCtxFindKey(ctx, key, keylen), outvalue);
}