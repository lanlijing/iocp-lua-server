local WSLuaCpp = require "WSLuaCpp"  -- 引用C++库,放在main.lua的最上面
require "common.Functions"
require "luatest.Singleton"

local g_timeLast = 0
function OnGameFrame()
	tick_now = WSLuaCpp.getTickCount()
	if tick_now - g_timeLast > 20000 then			-- 每20秒打印一条记录
		g_timeLast = WSLuaCpp.getTickCount()
		cclog("lua test OnGameFrame tick:%d", g_timeLast)
	end
end

function OnGameExit()
	cclog("lua test OnGameExit")
end 

function OnGameInit()
	WSLuaCpp.setCaption("luatest")
	tick = WSLuaCpp.getTickCount()
	cclog("lua test OnGameInit tick:%d", tick)
	
	Singleton:Instance():printInfo()
end