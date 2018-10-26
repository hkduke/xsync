--[[
  @file: trycall.lua
    实现 lua 脚本的安全调用封装

  @create: 2018-10-26
  @update: 2018-10-26
--]]

-- 异常捕获封装
function __try(block)
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


-- 安全沙箱调用
--    funcname: 要调用的函数名, 返回值必须为一个表
--    intable:  要调用的函数的输入表参数
--  返回: 函数 funcname 的返回值 (输出表)

local function __trycall(funcname, intable)
    -- 定义返回值
    local ret = {
        otable = {
            result = "ERROR"
        }
    }

    __try {
        __func = function ()
            -- 函数 func 执行必须返回 table
            ret.otable = funcname(intable)
        end,

        __catch = function (errors)
            -- 调用函数发生异常
            -- print("catch exception : " .. errors)
            ret.otable.exception = "exception: " .. tostring(errors)
        end,

        __finally = function (ok, errors)
            if not ok
            then
                -- 执行函数发生异常
                ret.otable.result = "EXCEPTION"
            end
        end
    }

    if not ret.otable
    then
        ret.otable = {
            result = "SUCCESS"
        }
    end

    if not ret.otable.result
    then
        ret.otable.result = "SUCCESS"
    end    

    return ret.otable;
end


-- ======================================================================
-- 用户实现的函数:
--   intab: 输入参数表
--   outab: 输出参数表
local function user_test(intab)
    local outab = {
        age = 89
    }

    -- 下面这句会抛出异常
    print(intab.hello + 100)

    return outab
end


-- 安全调用用户实现的函数
local outab = __trycall(user_test, {hello = "woorld"})

print(outab.result)
print(outab.exception)

