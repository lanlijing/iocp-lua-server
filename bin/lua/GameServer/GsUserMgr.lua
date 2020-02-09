local WSLuaCpp = require "WSLuaCpp"
require "common.Functions"
require "GameServer.GsUser"
local NetMsgId = require "common.NetMsgId"

GsUserMgr = {
    userlist = {},
    usercount = 0,
}

function GsUserMgr.addUser(netId)
    local user = GsUser:new(netId)
    GsUserMgr.userlist[netId] = user
    GsUserMgr.usercount = GsUserMgr.usercount + 1
	user:printInfo()
	SendMsgToClient(netId, NetMsgId.GS_CL_LOGIN_RESULT, 'sb', user.actorName, user.sex)
end

function GsUserMgr.removeUser(netId)
    local user = GsUserMgr.userlist[netId]
    if user ~= nil then
        GsUserMgr.userlist[netId] = nil
		GsUserMgr.usercount = GsUserMgr.usercount - 1
    end    
end

function GsUserMgr.onWorldChat(netId) 
	chat = WSLuaCpp.readRecvString()
	local user = GsUserMgr.userlist[netId]
	if user ~= nil then
		actorName = user.actorName
		s = 'worldchat: '..actorName..': '..chat
	end
	for k, v in pairs(GsUserMgr.userlist) do
		SendMsgToClient(k, NetMsgId.GS_CL_WORLD_CHAT, 's', chat)
	end
end

return GsUserMgr