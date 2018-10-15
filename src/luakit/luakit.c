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
 *   lua C kit
 *
 * @author: master@pepstack.com
 *
 * @version: 0.1.2
 *
 * @create: 2018-10-15
 *
 * @update:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "luakit.h"


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

    /* load the script */
    err = luaL_loadfile(L, luafile);
    if (err) {
        snprintf(lk->error, sizeof(lk->error), "luaL_loadfile fail: %s", luafile);
        lua_close(L);
        return (-1);
    }

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