--[[
  @file: __trycall.lua
    实现 lua 脚本的安全调用封装

  @create: 2018-10-26
  @update: 2018-10-26
--]]

function module_version(intab)
    return {
        version = "1.0",
        author = "zhangliang",
        update = "2018-10-26"
    }
end


--[[
-- 异常捕获封装. 以下内容禁止更改 !!
--]]
local function __try(block)
    local func = block.__func
    local catch = block.__catch
    local finally = block.__finally

    assert(func)
    assert(catch)
    assert(finally)

    -- try to call it
    local ok, errors = xpcall(func, debug.traceback)
    if not ok then
        -- run the catch function
        catch(errors)
    end

    -- run the finally function
    finally(ok, errors)

    -- ok?
    if ok then
        return errors
    end
end


--[[
-- 安全沙箱调用
--   funcname: 要调用的函数名, 返回值必须为一个表
--   intable:  要调用的函数的输入表参数
-- 返回: 函数 funcname 的返回值 (输出表)
--]]
function __trycall(funcname, intable)
    local func = _G[funcname]

    -- 定义返回值
    local ret = {
        out = {
            result = "ERROR",
            exception = string.format("error funcname: %s", funcname)
        }
    }

    if not func
    then
        return ret.out
    end

    __try {
        __func = function ()
            -- 函数 func 执行必须返回 table
            ret.out = func(intable)
        end,

        __catch = function (errors)
            -- 调用函数发生异常
            ret.out.exception = "exception: " .. tostring(errors)
        end,

        __finally = function (ok, errors)
            if not ok
            then
                -- 执行函数发生异常
                ret.out.result = "EXCEPTION"
            end
        end
    }

    -- 如果函数 funcname 无返回值 out
    if not ret.out
    then
        ret.out = {
            result = "SUCCESS",
            exception = "(none)"
        }
    end

    -- 如果函数 funcname 没有设定返回值 out.result
    if not ret.out.result
    then
        ret.out.result = "SUCCESS"
    end

    -- 如果函数 funcname 没有设定返回值 out.exception
    if not ret.out.exception
    then
        ret.out.exception = "(none)"
    end

    -- 总是返回一个表
    return ret.out;
end


--[[
    -- 安全调用用户实现的函数: module_version
    local out = __trycall("module_version")

    print("result=" .. out.result)
    print("exception=" .. out.exception)

    if out.result == "SUCCESS"
    then
        print("version=" .. out.version)
        print("author=" .. out.author)
    end
--]]
