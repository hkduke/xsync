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


/**
 * Creating a single lua_State per thread is a good solution
 *  to having multiple threads of Lua execution.
 */
struct luakit_t
{
    /* initialize Lua */
    lua_State * L;

    char error[256];
};


extern int LuaInitialize (struct luakit_t * lk, const char *luafile);

extern void LuaFinalize (struct luakit_t * lk);

extern const char * LuaGetError (struct luakit_t * lk);

extern int LuaCall (struct luakit_t * lk, const char *fnname);

#if defined(__cplusplus)
}
#endif

#endif /* LUAKIT_H_INCLUDED */
