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


function tweaktable(tab_in)
	local tab_out = {numfields=1}
	for k,v in pairs(tab_in) do
		tab_out.numfields = tab_out.numfields + 1
		tab_out[tostring(k)] = string.upper(tostring(v))
	end
	tab_out.numfields = tostring(tab_out.numfields)
	io.write("At bottom of callfuncscript.lua tweaktable(), numfields=")
	io.write(tab_out.numfields)
	print()
	return tab_out
end