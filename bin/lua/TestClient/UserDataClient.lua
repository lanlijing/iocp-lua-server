local WSLuaCpp = require "WSLuaCpp"
require "common.Functions"
local NetMsgId = require "common.NetMsgId"

local UserDataClient = {}

local g_userName = ""

UserDataClient.sendPing = function(netId)
	SendMsgToGameSvr(netId, NetMsgId.CL_GS_PING, "")
end

UserDataClient.onPong = function(netId)
	cclog("recv pong msg")
end

UserDataClient.sendTestMsg = function(netId)
	WSLuaCpp.beginSendBuf(netId,NetMsgId.CL_GS_TEST_MSG)
	
	WSLuaCpp.writeSendByte(1)
	WSLuaCpp.writeSendInt(-11)
	WSLuaCpp.writeSendShort(22)
	WSLuaCpp.writeSendUint32(121)
	WSLuaCpp.writeSendDouble(9.328)
	WSLuaCpp.writeSendString("hello,gamesvr")
	cout = 3
	WSLuaCpp.writeSendUint32(cout)
	for i = 1, cout, 1 do
		WSLuaCpp.writeSendDouble(math.random())
	end
	
	WSLuaCpp.endSendBuf()
    
	WSLuaCpp.sendDBufToGameSvr()
end

UserDataClient.onTestMsg = function(netId)
	cclog("byte: %d", WSLuaCpp.readRecvByte())
	cclog("int: %d", WSLuaCpp.readRecvInt())
	cclog("short: %d", WSLuaCpp.readRecvShort())
	cclog("uint32: %d", WSLuaCpp.readRecvUint32())
	cclog("double: %.4f", WSLuaCpp.readRecvDouble())
	cclog("string: %s", WSLuaCpp.readRecvString())
	cout = WSLuaCpp.readRecvUint32()
	cclog("cout: %d", cout)
	for i = 1, cout, 1 do
		cclog("double %d, %.4f", i, WSLuaCpp.readRecvDouble())
	end
	
	UserDataClient.sendLogin(netId)
end

UserDataClient.sendLogin = function(netId)
	SendMsgToGameSvr(netId, NetMsgId.CL_GS_LOGIN, "")
end

UserDataClient.onLoginResult = function(netId)
	g_userName = WSLuaCpp.readRecvString()
	sex = WSLuaCpp.readRecvByte()
	cclog("onLoginResult. userName:"..g_userName..",sex:"..sex)
	
	UserDataClient.sendWorldChat(netId, "world say"..math.random(0, 100))
end

UserDataClient.sendWorldChat = function(netId, chat)
	SendMsgToGameSvr(netId, NetMsgId.CL_GS_WORLD_CHAT, "s", chat)
end

UserDataClient.onWorldChat = function(netId)
	userName = WSLuaCpp.readRecvString()
	chat = WSLuaCpp.readRecvString()
	cclog("worldchat: %s:%s", userName, chat)
end

return UserDataClient

