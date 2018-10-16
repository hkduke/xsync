-- event-task.lua
-- xsync-client 任务执行脚本
-- version: 0.1
-- create: 2018-10-16
-- update: 2018-10-16


-- event: 事件名称
-- path: 文件路径
-- file: 文件名称
function on_event_task(intab)
    local outab = {
        result = "ERROR"
    }

    -- TODO:

	io.write("on_event_task: {event=")
    io.write(intab.event);
    io.write(",path=");
    io.write(intab.path);
    io.write(",file=");
    io.write(intab.file);
    io.write("}");
	print()

    outab.result = "SUCCESS"
	return outab
end
