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


-- http://valleu87.blog.163.com/blog/static/19670343220121111045390/
function dump_qiuqiu_jsonfile(intab)
    -- 返回值
    local outab = {
        result = "ERROR"
    }

    -- 创建 cjson 模块
    local js = create_config_cjson()
    
    local CHUNKSIZE = 16384

    local jsonfile = intab.jsonfile
    local jsonoutfile = intab.jsonoutfile

    local lc = 0
    local lw = 0

    local tstart = os.time()

    print(string.format("start time : %d", tstart))

    print(string.format("input file : %s", jsonfile))
    print(string.format("output file: %s", jsonoutfile))


    -- 以2进制读打开文件
    local infd = io.open(jsonfile, "rb")

    -- 获取文件长度
    local filesize = infd:seek("end")

    -- 重新设置文件索引为0的位置
    infd:seek("set", 0)

    -- 以2进制写打开文件
    local outfd = io.open(jsonoutfile, "wb")

    while true do
        local chunk = infd:read(CHUNKSIZE)

        if not chunk then
            break
        end

        local cblen = string.len(chunk)

        i = 0
        while i < cblen do
            i = i + 1

            local ch = string.byte(chunk, i)

            if ch == '\n' then
            
            end
        end
        
        outfd:write(chunk)            
    end

    -- 关闭文件
    io.close(infd)
    io.close(outfd)

    local tend = os.time()

    print(string.format("input file size   : %d bytes", filesize))

    print(string.format("total input lines : %d", lc))
    print(string.format("total output lines: %d", lw))
    print(string.format("elapsed seconds   : %d", tend - tstart))
    
    print(string.format("read file speed   : %d KB (%d MB) per second", math.modf(filesize / (1 + tend - tstart) / 1024 ), math.modf(filesize / (1 + tend - tstart) / 1024 / 1024 )))

    print(string.format("input lines speed : %f lines per second", math.modf(lc / (1 + tend - tstart))))
    print(string.format("output lines speed: %f lines per second", math.modf(lw / (1 + tend - tstart))))

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
