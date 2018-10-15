-- xclient-script.lua
-- xsync-client 动态配置文件
-- version: 0.1
-- create: 2018-10-15
-- update: 2018-10-15


function showinfo()
    print("welcome to  lua world ")
end
 
function showstr(str)
    print("The string you input is " .. str)
end


--add two numbers
function add(x,y)
    return x + y
end


function watch_on_query(tab_in)
	local tab_out = {numfields=1}

	for k, v in pairs(tab_in) do
		tab_out.numfields = tab_out.numfields + 1
		tab_out[tostring(k)] = tostring(v)
	end

	tab_out.numfields = tostring(tab_out.numfields)
	io.write("At bottom of callfuncscript.lua tweaktable(), numfields=")
	io.write(tab_out.numfields)
	print()
	return tab_out
end


function watch_on_ready(tab_in)
	local tab_out = {numfields=1}

	for k, v in pairs(tab_in) do
		tab_out.numfields = tab_out.numfields + 1
		tab_out[tostring(k)] = tostring(v)
	end

	tab_out.numfields = tostring(tab_out.numfields)
	io.write("At bottom of callfuncscript.lua tweaktable(), numfields=")
	io.write(tab_out.numfields)
	print()
	return tab_out
end


function watch_on_error(tab_in)
	local tab_out = {numfields=1}

	for k, v in pairs(tab_in) do
		tab_out.numfields = tab_out.numfields + 1
		tab_out[tostring(k)] = tostring(v)
	end

	tab_out.numfields = tostring(tab_out.numfields)
	io.write("At bottom of callfuncscript.lua tweaktable(), numfields=")
	io.write(tab_out.numfields)
	print()
	return tab_out
end
