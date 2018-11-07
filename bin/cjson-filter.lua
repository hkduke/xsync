--[[
  @file: cjson-filter.lua
    lua cjson module:
       https://www.kyne.com.au/~mark/software/lua-cjson-manual.html

 @create: 2018-11-07
 @update: 2018-11-07
--]]


-- 定义脚本路径:
package.path = package.path .. ";./?.lua;../bin/?.lua"

-- 定义模块路径:
package.cpath = package.cpath .. ";./?.so;../lib/lua/5.3/cjson.so;../libs/lib/lua/5.3/cjson.so"

-- 必须引入安全调用模块: trycall.lua
require("__trycall")

---[[
-- 加载 cjson 模块:

require("cjson")

--]]
