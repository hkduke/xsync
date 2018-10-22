-- path-filter.lua
-- xsync-client 目录过滤脚本
-- version: 0.1
-- create: 2018-10-16
-- update: 2018-10-16


-- 仅路径(path)过滤: 路径是绝对路径的全路径名, 以 '/' 结尾 !
--
function filter_path(intab)
    local outab = {
        result = "REJECT"
    }

    -- TODO:

    io.write("path-filter.lua::filter_path: {path=")
    io.write(intab.path);
    io.write("}");
    print()

    outab.result = "ACCEPT"

    return outab
end


-- 路径(path)+文件名(file)过滤
--
function filter_file(intab)
    local outab = {
        result = "REJECT"
    }

    -- TODO:

    io.write("path-filter.lua::filter_file: {path=")
    io.write(intab.path);
    io.write(";file=");
    io.write(intab.file);
    io.write(";mtime=");
    io.write(intab.mtime);
    io.write(";size=");
    io.write(intab.size);
    io.write("}");
    print()

    outab.result = "ACCEPT"

    return outab
end


-- inotify 询问是否添加路径监视
--
function inotify_watch_on_query(intab)
    local outab = {
        result = "REJECT"
    }

    io.write("path-filter.lua::inotify_watch_on_query: {wpath=")
    io.write(intab.wpath);
    io.write("}");
    print()

    outab.result = "ACCEPT"

    return outab
end


-- inotify 添加路径监视成功
--
function inotify_watch_on_ready(intab)
    local outab = {
        result = "ERROR"
    }

    io.write("path-filter.lua::inotify_watch_on_ready: {wpath=")
    io.write(intab.wpath);
    io.write("}");
    print()

    outab.result = "SUCCESS"

    return outab
end


-- inotify 添加路径监视失败
--
function inotify_watch_on_error(intab)
    local outab = {
        result = "ERROR"
    }

    io.write("path-filter.lua::inotify_watch_on_query: {wpath=")
    io.write(intab.wpath);
    io.write("}");
    print()

    outab.result = "SUCCESS"

    return outab
end
