-- event-task.lua
-- xsync-client 任务执行脚本
-- version: 0.1
-- create: 2018-10-16
-- update: 2018-10-16

--[[
 设置 kafka 连接参数
--]]
function kafka_config(intab)
    io.write("event-task.lua::kafka_config::intab: {kafkalib=")
    io.write(intab.kafkalib);
    io.write("}");
    print()

    local outab = {
        result = "SUCCESS",

        bootstrap_servers = "localhost:9092",
        socket_timeout_ms = "1000"
    }

    return outab
end


-- {type|time|clientid|thread|sid|event|pathid|path|file|route}
--
function on_event_task(intab)
    local outab = {
        result = "ERROR",
        loglevel = "INFO",
        kafka_topic = intab.clientid .. "_" .. intab.pathid,
        kafka_partition = "0"
    }

    message = string.format("{%s|%s|%s|%s|%s|%s|%s|%s|%s|%s}",
        intab.type,
        intab.time,
        intab.clientid,
        intab.thread,
        intab.sid,
        intab.event,
        intab.pathid,
        intab.path,
        intab.file,
        intab.route)

    ---[[ 调试输出
    print(message)
    --]]

    -- 指定输出的消息
    outab.message = message

    outab.result = "SUCCESS"
    return outab
end
