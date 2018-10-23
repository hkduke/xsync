local topic = "hello"

local day = nil

local file="beacon.data.collect.5m.log.181023-1545"


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

print(topic)
