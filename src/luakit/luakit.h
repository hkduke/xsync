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
 * luakit.h
 *   lua with C interop helper
 *
 *   http://troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.5
 *
 * @create: 2018-10-15
 *
 * @update: 2018-10-15 17:49:26
 *
 */
#ifndef LUAKIT_H_INCLUDED
#define LUAKIT_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * lua
 */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define LUAKIT_SUCCESS   0
#define LUAKIT_ERROR   (-1)

#define LUAKIT_OUT_PAIRS_MAXNUM      254
#define LUAKIT_OUT_KEYS_BUFSIZE     4096
#define LUAKIT_OUT_VALUES_BUFSIZE  16384


/**
 * Creating a single lua_State per thread is a good solution
 *  to having multiple threads of Lua execution.
 */
struct luakit_t
{
    /* initialize Lua */
    lua_State * L;

    /* 保存错误信息 */
    char error[256];

    /**
     * static outputs table
     */

    /* 输出的 key-value 对数目: 默认为 0 */
    int out_kv_pairs;

    /* 输出的 key 名称偏移: out_keys_buffer */
    int out_keys_offset[LUAKIT_OUT_PAIRS_MAXNUM + 2];

    /* 输出的 value 偏移: out_values_buffer */
    int out_values_offset[LUAKIT_OUT_PAIRS_MAXNUM + 2];

    /* 存储所有输出的 key 名称 */
    char out_keys_buffer[LUAKIT_OUT_KEYS_BUFSIZE];

    /* 存储所有输出的 value 值 */
    char out_values_buffer[LUAKIT_OUT_VALUES_BUFSIZE];
};


extern int LuaInitialize (struct luakit_t * lk, const char *luafile);

extern void LuaFinalize (struct luakit_t * lk);

extern const char * LuaGetError (struct luakit_t * lk);

extern int LuaCall (struct luakit_t * lk, const char *funcname, const char *key, const char *value);

extern int LuaCallMany (struct luakit_t * lk, const char *funcname, const char *keys[], const char *values[], int kv_pairs);

extern int LuaNumPairs (struct luakit_t * lk);

extern int LuaGetKey (struct luakit_t * lk, int index, char **outkey);

extern int LuaGetValue (struct luakit_t * lk, int index, char **outvalue);

extern int LuaFindKey (struct luakit_t * lk, const char *key, int keylen);

#if defined(__cplusplus)
}
#endif

#endif /* LUAKIT_H_INCLUDED */
