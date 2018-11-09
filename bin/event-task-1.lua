-- event-task.lua
-- xsync-client 任务执行脚本
-- version: 0.1
-- create: 2018-10-16
-- update: 2018-10-16

--[[
 设置 kafka 连接参数
--]]
function kafka_config(intab)
    local outab = {
        result = "ERROR",
        bootstrap_servers = "localhost:9092",
        socket_timeout_ms = "1000"
    }

    ---[[
    local msg = table.concat({
            "event-task-1.lua"
            ,"::"
            ,"kafka_config("
            ,"kafkalib="
            ,intab.kafkalib
            ,")"
        })
    print(msg)
    --]]

    outab.result = "SUCCESS"
    return outab
end


local function get_topic(fn)
    local day = nil
    local errday = "errday"

    if (fn == nil)
    then
        return errday
    end

    a = string.find(fn, ".log.")
    if (a == nil)
    then
        return errday
    end

    b = string.find(fn, "-", a+5)
    if (b == nil)
    then
        return errday
    end

    day = string.sub(fn, a+5, b-1)

    if (day == nil)
    then
        return errday
    else
        return day
    end
end


-- {type|time|clientid|thread|sid|event|pathid|path|file|route}
--
function on_event_task(intab)
    local outab = {
        result = "ERROR",
        loglevel = "INFO",
        kafka_topic = table.concat({intab.clientid, "_", intab.pathid, "_", get_topic(intab.file)}),
        kafka_partition = "0"
    }

    -- 指定输出的消息
    outab.message = table.concat({
        "{"
        ,intab.type
        ,"|"
        ,intab.time
        ,"|"
        ,intab.clientid
        ,"|"
        ,intab.thread
        ,"|"
        ,intab.sid
        ,"|"
        ,intab.event
        ,"|"
        ,intab.pathid
        ,"|"
        ,intab.path
        ,"|"
        ,intab.file
        ,"|"
        ,intab.route
        ,"}"
    })
        
    ---[[ 调试输出
    print(outab.kafka_topic)
    print(outab.message)
    --]]

    dump_qiuqiu_jsonfile({
        jsonfile = intab.path .. intab.file,
        jsonoutfile = "/tmp/" .. intab.file .. ".csv"
    })

    outab.result = "SUCCESS"
    return outab
end
