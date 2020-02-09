#ifndef _DNETMSG_H_
#define _DNETMSG_H_

#pragma once

#include "DNetMsgBase.h"

/*
网络消息定义
要求以下三方要保持一致：
服务端C++
服务端LUA
客户端LUA
*/
struct NetMsgID
{
	enum
	{
		//系统内部消息
		SYSTEM_MSG_BEGIN = 1,
		SYSTEM_MSG_CLOSE = 2, //程序关闭
		SYSTEM_MSG_LUADEBUG = 3, //lua调试		
		SYSTEM_MSG_END = 99,

		//以下是客户端和服务器交互
		CL_GS_BEGIN = 1001,
		CL_GS_END = 9999,
	};
};


#endif