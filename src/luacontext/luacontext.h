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
 * luacontext.h
 *   lua with C interop helper
 *
 *   http://troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm
 *
 * @author: master@pepstack.com
 *
 * @version: 0.2.8
 *
 * @create: 2018-10-15
 *
 * @update: 2018-10-22 10:56:41
 *
 */
#ifndef LUACTX_H_INCLUDED
#define LUACTX_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * liblua.a
 */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


/* 定义函数返回值 */
#define LUACTX_SUCCESS                0
#define LUACTX_ERROR                (-1)

#define LUACTX_OUT_MEMORY           (-4)
#define LUACTX_LOCK_ERROR           (-5)

#define LUACTX_E_L_NEWSTATE         (-11)
#define LUACTX_E_L_LOADFILE         (-12)
#define LUACTX_E_L_PCALL            (-13)


/* 无效的索引 */
#define LUACTX_BAD_INDEX     (-1)


/* 单线程模式, 禁用内部锁 */
#define LUACTX_THREAD_MODE_SINGLE  0

/* 多线程模式, 启用内部锁. 支持多线程安全 */
#define LUACTX_THREAD_MODE_MULTI   1


/* 错误消息最长字符数, 不包括 '\0' 结尾符 */
#ifndef LUACTX_ERROR_MAXLEN
#  define LUACTX_ERROR_MAXLEN      255
#endif


/* 支持的最大输出键值对数目 */
#ifndef LUACTX_PAIRS_MAXNUM
#  define LUACTX_PAIRS_MAXNUM      254
#endif


/* 支持的最大输出键名缓冲区字节大小 */
#ifndef LUACTX_KEYS_BUFSIZE
#  define LUACTX_KEYS_BUFSIZE     4096
#endif


/* 支持的最大输出值缓冲区字节大小 */
#ifndef LUACTX_VALUES_BUFSIZE
#  define LUACTX_VALUES_BUFSIZE  16384
#endif


/**
 * Creating a single lua_State per thread is a good solution
 *  to having multiple threads of Lua execution.
 */
typedef struct lua_context_t * lua_context;


extern int LuaCtxNew (const char *scriptfile, int threadmode, lua_context *outctx);

extern void LuaCtxFree (lua_context *pctx);

extern lua_State * LuaCtxLockState (lua_context ctx);

extern void LuaCtxUnlockState (lua_context ctx);

extern const char * LuaCtxGetError (lua_context ctx);

extern int LuaCtxCall (lua_context ctx, const char *funcname, const char *key, const char *value);

extern int LuaCtxCallMany (lua_context ctx, const char *funcname, const char *keys[], const char *values[], int kv_pairs);

extern int LuaCtxNumPairs (lua_context ctx);

/**
 * outkey 不成功不会设置
 */
extern int LuaCtxGetKey (lua_context ctx, int index, char **outkey);

/**
 * outvalue 不成功不会设置
 */
extern int LuaCtxGetValue (lua_context ctx, int index, char **outvalue);

extern int LuaCtxFindKey (lua_context ctx, const char *key, int keylen);

/**
 * outvalue 不成功不会设置
 */
extern int LuaCtxGetValueByKey (lua_context ctx, const char *key, int keylen, char **outvalue);

#if defined(__cplusplus)
}
#endif

#endif /* LUACTX_H_INCLUDED */
