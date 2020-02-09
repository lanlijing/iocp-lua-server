local WSLuaCpp = require "WSLuaCpp" -- 引用C++库,放在main.lua的最上面
require "common.Functions"
local NetMsgId = require("common.NetMsgId")
local UserDataClient = require "TestClient.UserDataClient"

local MsgDispatch = {
	[NetMsgId.GS_CL_PONG] = UserDataClient.onPong,
	[NetMsgId.GS_CL_TEST_MSG] = UserDataClient.onTestMsg,
	[NetMsgId.GS_CL_LOGIN_RESULT] = UserDataClient.onLoginResult,
	[NetMsgId.GS_CL_WORLD_CHAT] = UserDataClient.onWorldChat,
}

local g_netId = 0
local g_timeLast = 0
local g_svrConnected = false
function OnGameInit()
    cclog('TestClient.lua 初始化...')
    WSLuaCpp.setCaption("TestClient")
    
    --设置随机数种子
    math.randomseed((os.time()) * 1000) 
end

function OnGameFrame()
	local timeNow = os.clock()
	if (g_netId ~= 0) and (g_svrConnected == true) then
		if (timeNow - g_timeLast) > 50 then
           g_timeLast = timeNow
		   SendMsgToGameSvr(g_netId, NetMsgId.CL_GS_PING, "")
		end
	end
end

function OnGameExit()
	cclog("TestClient.OnGameExit")
end

function OnGameSvrConnect()
	cclog("OnGameSvrConnec")
	g_svrConnected = true
end

function OnGameSvrDisConnect()
    cclog("OnGameSvrDisConnect")
	g_svrConnected = false
end

function OnGameSvrMsg()
	local nTag , nBufLen ,nNetId ,nMsgId = 0,0,0,0
	nTag = WSLuaCpp.readRecvUint32()
    nBufLen = WSLuaCpp.readRecvUint32()
    nNetId = WSLuaCpp.readRecvUint32()
    nMsgId = WSLuaCpp.readRecvUint32()
	
	cclog("OnGameSvrMsg. buflen:%d, netid:%d, msgid:%d", nBufLen, nNetId, nMsgId)
	if nMsgId == NetMsgId.GS_CL_SETNETID then
		g_netId = nNetId
		UserDataClient.sendTestMsg(nNetId)
	else
        local func = MsgDispatch[nMsgId]
        if (func ~= nil) then
            xpcall(function() func(g_netId) end,Throw)     
        else
            cclog("[OnGameSvrMsg]msg not have dispatch function:%d",nMsgId)
        end
    end
end
