-- event-task.lua
-- xsync-client 任务执行脚本
-- version: 0.1
-- create: 2018-10-16
-- update: 2018-10-16


-- "type", "time", "thread", "event", "clientid", "pathid", "file", "route", "path"
--
function on_event_task(intab)
    local outab = {
        result = "ERROR",
        kafka_topic = intab.clientid .. "_" .. intab.pathid,
        kafka_partition = "0"
    }

    io.write("event-task.lua::on_event_task: {")
    io.write("type=");
    io.write(intab.type);
    io.write(";time=");
    io.write(intab.time)
    io.write(";thread=");
    io.write(intab.thread)
    io.write(";event=");
    io.write(intab.event)
    io.write(";clientid=");
    io.write(intab.clientid)
    io.write(";pathid=");
    io.write(intab.pathid)
    io.write(";file=");
    io.write(intab.file)
    io.write(";route=");
    io.write(intab.route)
    io.write(";path=");
    io.write(intab.path)
    io.write("}");
    print()

    outab.result = "SUCCESS"
    return outab
end
