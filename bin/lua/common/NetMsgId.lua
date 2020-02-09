--[[
网络消息定义
要求以下三方要保持一致：
服务端C++
服务端LUA
客户端LUA
]]--
local NetMsgId = {
    -- 系统内部消息
    SYSTEM_MSG_BEGIN = 1,
    SYSTEM_MSG_CLOSE = 2,                   --程序关闭
    SYSTEM_MSG_LUADEBUG = 3,                --lua调试
    SYSTEM_MSG_END = 99,

    -- 以下是客户端和服务器交互
    CL_GS_BEGIN = 1001,
	
	GS_CL_SETNETID = 1101,   	-- 服务器向客户端发送设置网络id
	CL_GS_PING = 1102,  		-- 客户端向服务端发送ping消息
	GS_CL_PONG = 1103, 			-- 服务端向客户端返回ping消息
	CL_GS_TEST_MSG = 1104,		-- 客户端向服务端发送测试消息
	GS_CL_TEST_MSG = 1105,		-- 服务端向客户端返回测试消息
	
	CL_GS_LOGIN = 1201,		-- 客户端向服务端发送登陆消息
	GS_CL_LOGIN_RESULT = 1202,	-- 服务端返回登陆消息
	CL_GS_WORLD_CHAT = 1203,	-- 客户端向服务端发送世界聊天消息
	GS_CL_WORLD_CHAT = 1204,		-- 服务端向客户端转发世界聊天信息
	
	CL_GS_END = 9999,
}

return NetMsgId