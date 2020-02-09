local WSLuaCpp = require "WSLuaCpp" -- 引用C++库,放在main.lua的最上面
require "common.Functions"
local NetMsgId = require("common.NetMsgId")
local GameSvr = require "GameServer.GameSvr"
local GsUserMgr = require "GameServer.GsUserMgr"

local MsgDispatch = {
	[NetMsgId.CL_GS_PING] = GameSvr.onPing,
	[NetMsgId.CL_GS_TEST_MSG] = GameSvr.onTestMsg,
	[NetMsgId.CL_GS_LOGIN] = GsUserMgr.addUser,
	[NetMsgId.CL_GS_WORLD_CHAT] = GsUserMgr.onWorldChat,
}

function OnGameInit()
    cclog("lua 初始化...")
    WSLuaCpp.setCaption("GameServer")
    
    --设置随机数种子
    math.randomseed((os.time()) * 1000) 
end

function OnGameFrame()
end

function OnGameExit()
end

function OnClientConnect(linkerid)
	cclog("OnClientConnect %d", linkerid)
	SendMsgToClient(linkerid, NetMsgId.GS_CL_SETNETID, "")
end

function OnClientDisConnect(linkerid)
    cclog("OnClientDisConnect %d", linkerid)
end

function OnClientMsg(linkerid)
	local nTag , nBufLen ,nNetId ,nMsgId = 0,0,0,0
	nTag = WSLuaCpp.readRecvUint32()
    nBufLen = WSLuaCpp.readRecvUint32()
    nNetId = WSLuaCpp.readRecvUint32()
    nMsgId = WSLuaCpp.readRecvUint32()
	
	if (nNetId ~= linkerid) then
		cclog("OnClientMsg error. nNetId:%d, linkerid:%d", nNetId, linkerid)
		return
	end
	cclog("OnClientMsg. buflen:%d, netid:%d, msgid:%d", nBufLen, nNetId, nMsgId)
	
	local func = MsgDispatch[nMsgId]
    if (func ~= nil) then
        xpcall(function() func(nNetId) end,Throw)     
    else
        cclog("[OnClientMsg]msg not have dispatch function:%d",nMsgId)
    end 
end

