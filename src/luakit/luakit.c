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
 * luakit.c
 *   lua with C interop helper
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.5
 *
 * @create: 2018-10-15
 *
 * @update: 2018-10-15 15:21:40
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

#include "luakit.h"


int LuaCall (struct luakit_t * lk, const char *funcname, const char *key, const char *value)
{
    lua_State * L = lk->L;

    lk->out_kv_pairs = 0;
    lk->out_keys_offset[0] = 0;
    lk->out_values_offset[0] = 0;

    lua_settop(L, 0);

    lua_getglobal(L, funcname);                 /* Tell it to run callfuncscript.lua->tweaktable() */

    lua_newtable(L);                            /* Push empty table onto stack table now at -1 */

    lua_pushstring(L, key);          /* Push a key onto the stack, table now at -2 */
    lua_pushstring(L, value);        /* Push a value onto the stack, table now at -3 */

    lua_settable(L, -3);                    /* Take key and value, put into table at -3, */
                                                /*  then pop key and value so table again at -1 */

    /* Run function, !!! NRETURN=1 !!! */
    if ( lua_pcall(L, 1, 1, 0) ) {
        snprintf(lk->error, sizeof(lk->error), "lua_pcall fail: %s", lua_tostring(L, -1));
        return (-1);
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

        kcb_next = lk->out_keys_offset[lk->out_kv_pairs] + kcb;
        vcb_next = lk->out_values_offset[lk->out_kv_pairs] + vcb;

        lk->out_keys_offset[lk->out_kv_pairs + 1] = kcb_next;
        lk->out_values_offset[lk->out_kv_pairs + 1] = vcb_next;

        if (kcb) {
            if (kcb_next < LUAKIT_OUT_KEYS_BUFSIZE) {
                memcpy(lk->out_keys_buffer + lk->out_keys_offset[lk->out_kv_pairs], k, kcb);
            } else {
                snprintf(lk->error, sizeof(lk->error), "too large output keys: more than %d bytes.", LUAKIT_OUT_KEYS_BUFSIZE);
                return (-1);
            }
        }

        if (vcb) {
            if (vcb_next < LUAKIT_OUT_VALUES_BUFSIZE) {
                memcpy(lk->out_values_buffer + lk->out_values_offset[lk->out_kv_pairs], v, vcb);
            } else {
                snprintf(lk->error, sizeof(lk->error), "too large output values: more than %d bytes.", LUAKIT_OUT_VALUES_BUFSIZE);
                return (-1);
            }
        }

        lk->out_kv_pairs++;

        if (lk->out_kv_pairs > LUAKIT_OUT_PAIRS_MAXNUM) {
            snprintf(lk->error, sizeof(lk->error), "too many output pairs: more than %d.", LUAKIT_OUT_PAIRS_MAXNUM);
            return (-1);
        }
    }

    /* success */
    return 0;
}


int LuaCallMany (struct luakit_t * lk, const char *funcname, const char *keys[], const char *values[], int kv_pairs)
{
    int i = 0;

    lua_State * L = lk->L;

    lk->out_kv_pairs = 0;
    lk->out_keys_offset[0] = 0;
    lk->out_values_offset[0] = 0;

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
        snprintf(lk->error, sizeof(lk->error), "lua_pcall fail: %s", lua_tostring(L, -1));
        return (-1);
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

        kcb_next = lk->out_keys_offset[lk->out_kv_pairs] + kcb;
        vcb_next = lk->out_values_offset[lk->out_kv_pairs] + vcb;

        lk->out_keys_offset[lk->out_kv_pairs + 1] = kcb_next;
        lk->out_values_offset[lk->out_kv_pairs + 1] = vcb_next;

        if (kcb) {
            if (kcb_next < LUAKIT_OUT_KEYS_BUFSIZE) {
                memcpy(lk->out_keys_buffer + lk->out_keys_offset[lk->out_kv_pairs], k, kcb);
            } else {
                snprintf(lk->error, sizeof(lk->error), "too large output keys: more than %d bytes.", LUAKIT_OUT_KEYS_BUFSIZE);
                return (-1);
            }
        }

        if (vcb) {
            if (vcb_next < LUAKIT_OUT_VALUES_BUFSIZE) {
                memcpy(lk->out_values_buffer + lk->out_values_offset[lk->out_kv_pairs], v, vcb);
            } else {
                snprintf(lk->error, sizeof(lk->error), "too large output values: more than %d bytes.", LUAKIT_OUT_VALUES_BUFSIZE);
                return (-1);
            }
        }

        lk->out_kv_pairs++;

        if (lk->out_kv_pairs > LUAKIT_OUT_PAIRS_MAXNUM) {
            snprintf(lk->error, sizeof(lk->error), "too many output pairs: more than %d.", LUAKIT_OUT_PAIRS_MAXNUM);
            return (-1);
        }
    }

    /* success */
    return 0;
}


int LuaInitialize (struct luakit_t * lk, const char *luafile)
{
    int err;
    lua_State * L;

    bzero(lk, sizeof(*lk));

    /*initialize Lua*/
    L = luaL_newstate();
    if (! L) {
        snprintf(lk->error, sizeof(lk->error), "luaL_newstate fail");
        return (-1);
    }

    /*load Lua base libraries*/
    luaL_openlibs(L);

    /* luaL_loadfile
     *   PANIC: unprotected error in call to Lua API (attempt to call a nil value)
     */
    err = luaL_loadfile(L, luafile);
    if (err) {
        snprintf(lk->error, sizeof(lk->error), "luaL_loadfile fail: %s", lua_tostring(L, -1));
        lua_close(L);
        return (-1);
    }

    /* PRIMING RUN. FORGET THIS AND YOU'RE TOAST */
    if (lua_pcall(L, 0, 0, 0)) {
        snprintf(lk->error, sizeof(lk->error), "lua_pcall fail: %s", lua_tostring(L, -1));
        lua_close(L);
        return (-1);
    }

    /* cleanup stack */
    lua_settop(L, 0);

    /* success */
    lk->L = L;
    return 0;
}


void LuaFinalize (struct luakit_t * lk)
{
    if (lk->L) {
        lua_State * L = lk->L;
        lk->L = 0;

        /*cleanup Lua*/
        lua_close(L);
    }

    bzero(lk, sizeof(*lk));
}


const char * LuaGetError (struct luakit_t * lk)
{
    lk->error[ sizeof(lk->error) - 1 ] = '\0';
    return lk->error;
}


int LuaNumPairs (struct luakit_t * lk)
{
    return lk->out_kv_pairs;
}


int LuaGetKey (struct luakit_t * lk, int index, char **outkey)
{
    int start = lk->out_keys_offset[index];
    int end = lk->out_keys_offset[index + 1];

    *outkey = lk->out_keys_buffer + start;

    return (end - start);
}


int LuaGetValue (struct luakit_t * lk, int index, char **outvalue)
{
    int start = lk->out_values_offset[index];
    int end = lk->out_values_offset[index + 1];

    *outvalue = lk->out_values_buffer + start;

    return (end - start);
}


int LuaFindKey (struct luakit_t * lk, const char *key, int keylen)
{
    int index;

    for (index = 0; index < lk->out_kv_pairs; index++) {
        char *k;
        int kcb = LuaGetKey(lk, index, &k);

        if (kcb == keylen + 1) {
            if (! strncmp(key, k, keylen)) {
                return index;
            }
        }
    }

    return (-1);
}