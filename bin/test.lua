-- test.lua

-- 定义模块路径:
package.cpath = package.cpath .. ";./?.so;../lib/lua/5.3/cjson.so;../libs/lib/lua/5.3/cjson.so"

-- 必须引入安全调用模块: trycall.lua
require("__trycall")


--[[
  intab = {
    jsonfile = "/root/Downloads/shop01-rdb_6101.rdb.json",
    jsonoutfile = "/root/Downloads/shop01-rdb_6101.rdb.json.out"
  }
--]]

function dump_qiuqiu_jsonfile(intab)
    -- 返回值
    local outab = {
        result = "ERROR"
    }

    -- 加载 cjson 模块:
    local js = require("cjson")

    local BUFSIZE = 8192

    local jsonfile = intab.jsonfile
    local jsonoutfile = intab.jsonoutfile

    local tstart = os.time()

    print(string.format("start time : %d", tstart))

    print(string.format("input file : %s", jsonfile))
    print(string.format("output file: %s", jsonoutfile))


    -- 打开输入文件
    local fin = io.input(jsonfile)

    -- 输出文件会被覆盖!!
    local fout = io.open(jsonoutfile, "w")

    -- 行计数
    local lc = 0
    local lw = 0

    while true do
        local lines, rest = fin:read(BUFSIZE, "*line")
        if not lines then
            break
        end

        if rest then
            lines = lines .. rest .. "\n"
        end

        for line in lines:gmatch("[^\r\n]+") do
            -- 计算输入行数    
            lc = lc + 1
            
            jstab = js.decode(line)

            local genid = jstab.gen_id
            local rekey = jstab.redis_key

            local rkval = jstab[rekey]
            
            if rkval ~= nil then

                if type(rkval) == "string" then
                    jsobjects = js.decode(rkval)

                    for k, v in pairs(jsobjects) do
                        message = string.format("%s|%s|%s\n", genid, v.obj, v.t)

                        fout:write(message)

                        -- 计算输出行数
                        lw = lw + 1
                    end
                else
                    for k, v in pairs(rkval) do
                        message = string.format("%s|%s|%s", genid, v.obj, v.t)

                        fout:write(message)

                        -- 计算输出行数
                        lw = lw + 1
                    end
                end

            end

        end
        
        fout:flush()
    end

    -- 关闭文件
    fin:close()
    fout:close()

    local tend = os.time()

    print(string.format("total input  lines: %d", lc))
    print(string.format("total output lines: %d", lw))
    print(string.format("elapsed seconds   : %d", tend - tstart))
    print(string.format("input lines speed : %f line per second", lc / (tend - tstart)))
    print(string.format("output lines speed: %f line per second", lw / (tend - tstart)))

    -- 返回值
    outab.result = "SUCCESS"
    return outab
end


-- main function
--  
local out = __trycall("dump_qiuqiu_jsonfile", {
        jsonfile = "/root/Downloads/shop01-rdb_6101.rdb.json",
        jsonoutfile = "/root/Downloads/shop01-rdb_6101.rdb.json.out"
    })

print("result=" .. out.result)
print("exception=" .. out.exception)
