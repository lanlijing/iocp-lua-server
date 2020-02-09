local WSLuaCpp = require "WSLuaCpp"
require "common.Functions"
local NetMsgId = require "common.NetMsgId"

local GameSvr = {}

GameSvr.onPing = function(netId)
	SendMsgToClient(netId, NetMsgId.GS_CL_PONG, "")
end

GameSvr.onTestMsg = function(netId)
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
	-- 发送返回消息
	WSLuaCpp.beginSendBuf(netId,NetMsgId.GS_CL_TEST_MSG)
	
	WSLuaCpp.writeSendByte(6)
	WSLuaCpp.writeSendInt(-33)
	WSLuaCpp.writeSendShort(224)
	WSLuaCpp.writeSendUint32(191)
	WSLuaCpp.writeSendDouble(19.223)
	WSLuaCpp.writeSendString("hello,client")
	cout = 3
	WSLuaCpp.writeSendUint32(cout)
	for i = 1, cout, 1 do
		WSLuaCpp.writeSendDouble(math.random() * 20)
	end
	
	WSLuaCpp.endSendBuf()
    
	WSLuaCpp.sendDBufToClient(netId)
end

return GameSvr