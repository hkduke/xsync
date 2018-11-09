-- path-filter.lua
-- xsync-client 目录过滤脚本
-- version: 0.1
-- create: 2018-10-16
-- update: 2018-11-09


-- filter_path
-- 仅路径(path)过滤: 路径是绝对路径的全路径名, 以 '/' 结尾 !
function filter_path(intab)
    local outab = {
        result = "ERROR"
    }

    ---[[
        local msg = table.concat({
            "path-filter-1.lua"
            ,"::"
            ,"filter_path("
            ,"path="
            ,intab.path
            ,")"
        })

        print(msg)
    --]]

    outab.result = "SUCCESS"
    return outab
end


-- filter_file
-- 路径(path)+文件名(file)过滤
function filter_file(intab)
    local outab = {
        result = "ERROR"
    }

    ---[[
    local msg = table.concat({
            "path-filter-1.lua"
            ,"::"
            ,"filter_file("
            ,"path="
            ,intab.path
            ,";file="
            ,intab.file
            ,";mtime="
            ,intab.mtime
            ,";size="
            ,intab.size
            ,")"
        })

    print(msg)
    --]]

    outab.result = "SUCCESS"
    return outab
end


-- inotify_watch_on_query
-- 询问是否添加路径监视
function inotify_watch_on_query(intab)
    local outab = {
        result = "ERROR"
    }

    ---[[
    local msg = table.concat({
            "path-filter-1.lua"
            ,"::"
            ,"inotify_watch_on_query("
            ,"wpath="
            ,intab.wpath
            ,")"
        })

    print(msg)
    --]]

    outab.result = "SUCCESS"
    return outab
end


-- inotify_watch_on_ready
-- 添加路径监视成功
function inotify_watch_on_ready(intab)
    local outab = {
        result = "ERROR"
    }

    ---[[
    local msg = table.concat({
            "path-filter-1.lua"
            ,"::"
            ,"inotify_watch_on_ready("
            ,"wpath="
            ,intab.wpath
            ,")"
        })

    print(msg)
    --]]

    outab.result = "SUCCESS"
    return outab
end


-- inotify_watch_on_error
-- 添加路径监视失败
function inotify_watch_on_error(intab)
    local outab = {
        result = "ERROR"
    }

    ---[[
    local msg = table.concat({
            "path-filter-1.lua"
            ,"::"
            ,"inotify_watch_on_error("
            ,"wpath="
            ,intab.wpath
            ,")"
        })

    print(msg)
    --]]

    outab.result = "SUCCESS"
    return outab
end
