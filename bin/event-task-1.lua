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


function get_topic(fn)


    local day = nil

    local file="isurecloud.newzt2wd01.ztgame.com.cn.201810231600.log.gz"


    local a=string.find(file, ".log.")

    if (a ~= nil) then
        local b=string.find(file, "-", a + 5)
        if (b ~= nil) then
            day = string.sub(file, a+5, b - 1)
        end
    end

    if (day ~= nil) then
        topic = topic .. "_" .. day
    else
        topic = topic .. "_noday"
    end

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

    --[[ 调试输出
    print(message)
    --]]

    local topic = get_topic(intab.file)

    -- 指定输出的消息
    outab.message = message

    outab.kafka_topic = string.format("%s_%s_%s", intab.clientid, intab.pathid, topic);

    print(outab.kafka_topic)

    outab.result = "SUCCESS"
    return outab
end
