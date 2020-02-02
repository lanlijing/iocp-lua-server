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
		CLIENT_SERVER_BEGIN = 1001,

		//心跳网络ID相关
		PROXY_CL_SETNETID = 1002, //网关服向客户端设置网络ID
		CL_PROXY_TIMER = 1003, //客户端向网关服发心跳包
		CL_PROXY_SERVERTIME = 1004, // 客户端向服务端请求时间
		PROXY_CL_SERVERTIME = 1005,	// 服务端向客户端请求时间
		//登陆相关
		CL_SERVER_LOGIN = 1011, //客户端请求登陆
		SERVER_CL_LOGIN = 1012, //服务端返回登陆
		CL_SERVER_REGIST = 1013, //客户端请求注册
		SERVER_CL_REGIST = 1014, //服务端返回注册
		CL_SERVER_REQBASEINFO = 1015,	// 客户端请求玩家基本信息
		SERVER_CL_REQBASEINFO = 1016,	// 服务端返回玩家基本信息
		CL_SERVER_REQRANDOMNICKNAME = 1017,		// 客户端请求随机姓名
		SERVER_CL_REQRANDOMNICKNAME = 1018,		//服务端返回随机姓名
		//登陆结束

		// PVE战斗消息相关
		CL_SERVER_REQFIGHT = 1101,			// 客户端向服务端请求战斗
		SERVER_CL_REQFIGHT = 1102,			// 服务端向客户端返回战斗请求
		CL_SERVER_FIGHTREADY = 1103,		// 客户端告诉服务端战斗开始
		SERVER_CL_FIGHTBEGIH = 1104,		// 服务端告诉客户端战斗开始
		CL_SERVER_SETFIGHTIMME = 1105,		// 客户端告诉服务端自动战斗
		SERVER_CL_SETCANINSIZE = 1106,		// 服务端告诉客户端调整可进场区域大小(0~5)
		SERVER_CL_PVEFIGHTEND = 1107,		// 服务端向客户端发送PVE战斗结束
		CL_SERVER_REQCARDINFIGHT = 1108,	// 客户端向服务端请求把一个卡牌放入战斗
		SERVER_CL_CARDINFIGHT = 1109,		// 服务端向客户端返回卡牌放入战斗请求
		SERVER_CL_CARDOUTFIGHT = 1110,		// 服务端告诉客户端某卡牌退出战斗(死亡)
		SERVER_CL_NORMALATTACK = 1111,		// 服务端告诉客户端某卡牌普通攻击
		// PVE战斗消息结束

		// 聊天消息相关
		CL_SERVER_WORLDCHAT = 1201,				// 客户端向服务端请求世界聊天
		SERVER_CL_WORLDCHAT = 1202,				// 服务端向客户端返回世界聊天
		CL_SERVER_PRIVATECHAT = 1203,			// 客户端向服务端请求私聊
		SERVER_CL_PRIVATECHAT = 1204,			// 服务端向客户端返回私聊
		// 聊天消息结束

		CLIENT_SERVER_END = 9999,

		//以下是服务端之间的消息
		SERVER_SERVER_BEGIN = 10001,
		PROXY_GAMESVR_CONNECT = 10002,			// 网关服连接游戏服
		PROXY_DBSVR_CONNECT = 10003,			// 网关服连接数据库服
		GAMESVR_DBSVR_CONNECT = 10004,				// 游戏服连接DB服

		//客户端上下线
		CLIENT_ONOFF_BEGIN = 10100,
		PROXY_GAMESVR_CLIENTOFF = 10101,		// 网关服告诉游戏服某客户端下线
		GAMESVR_DBSVR_CLIENTOFF = 10102,		// 游戏服告诉DB服某玩家下线
		GAMESVR_PROXY_CLOSECLIENT = 10103,		// 游戏服让网关服关闭某个客户端
		CLIENT_ONOFF_END = 10199,
		// 客户端上下线结束

		SERVER_SERVER_END = 19999,
	};
};


#endif