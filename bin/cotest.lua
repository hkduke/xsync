-- test.lua
-- http://www.lua.org/manual/5.3/manual.html
-- https://www.cnblogs.com/chenny7/p/3634529.html

-- 定义 lua 脚本的路径
package.path = package.path .. ";./?.lua;../bin/?.lua"

-- 定义 so 模块路径
package.cpath = package.cpath .. ";./?.so;../lib/lua/5.3/cjson.so;../libs/lib/lua/5.3/cjson.so"

-- 必须引入安全调用模块: trycall.lua
require("__trycall")

-- TODO: 未完成

local function create_config_cjson ()
    -- 加载 cjson 模块:
    local cjson = require("cjson")

    -- 创建一个副本 (默认配置)
    local cjson2 = cjson.new()

    -- 设置重用 buffer (默认)
    cjson2.encode_keep_buffer(true)
     
    -- 最大解码层数 (默认 1000)
    cjson2.decode_max_depth(10)

    -- 遇到非法数字转为 null  (默认 false)
    cjson2.encode_invalid_numbers(null)

    -- 设置数字的精度 (默认 14)
    cjson2.encode_number_precision(10)

    return cjson2
end


local function process_json_line (line, js, outfd)
    local nw = 0

    local jstab = js.decode(line)

    local genid = jstab.gen_id
    local rekey = jstab.redis_key
    local rkval = jstab[rekey]
    
    if rkval ~= nil then

        if type(rkval) == "string" then
            local jsobjects = js.decode(rkval)

            for k, v in pairs(jsobjects) do
                local msg = table.concat({genid, "|", v.obj, "|", v.t, "\n"})

                outfd:write(msg)

                -- 计算输出行数
                nw = nw + 1
            end
        else
            for k, v in pairs(rkval) do
                local msg = table.concat({genid, "|", v.obj, "|", v.t, "\n"})

                outfd:write(msg)

                -- 计算输出行数
                nw = nw + 1
            end
        end
    end

    return nw
end


local function send(lines)
    print(lines)

    coroutine.yield(lines)
end


local function producer(intab)
    local infd = io.input(intab.jsonfile)

    return coroutine.create(function ()
        local lines, rest = infd:read(16384, "*line")

        if not lines then
            send(nil)
        else
            if rest then
                send(table.concat({lines, rest, "\n"}))
            else
                send(lines)
            end
        end
    end)
end


local function receive(producer)
    local status, lines = coroutine.resume(producer)
    return lines
end


local function consumer(intab, producer)
    local js = create_config_cjson()

    local jsonfile = intab.jsonfile
    local jsonoutfile = intab.jsonoutfile

    local outfd = io.open(jsonoutfile, "w")

    local lines = receive(producer)

    while lines do
        for line in lines:gmatch("[^\r\n]+") do
            process_json_line(line, js, outfd)
        end

        outfd:flush()

        local lines = receive(producer)
    end

    outfd:flush()
    outfd:close()
end



function dump_qiuqiu_jsonfile(intab)
    local tstart = os.time()

    consumer(intab, producer(intab))

    local els = os.time() - tstart + 1

    print(string.format("elapsed seconds = %d", els))
end


-- main function
--  
local out = __trycall("dump_qiuqiu_jsonfile", {
        jsonfile = "/root/Downloads/shop01-rdb_6101.rdb.json",
        jsonoutfile = "/root/Downloads/shop01-rdb_6101.rdb.json.out"
    })

print("result=" .. out.result)
print("exception=" .. out.exception)
