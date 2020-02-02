#ifndef _GENESERVERTHREAD_H_
#define _GENESERVERTHREAD_H_

#pragma once

#include "DAppThread.h"
#include "DNetWorkBase.h"

#define LUATESTNAME				"LuaTest"
#define TESTCLIENTNAME			"TestClient"
#define GAMESERVERNAME			"GameServer"

struct GeneServerType
{
	enum
	{
		eLuaTest = 0,
		eTestClient = 1,
		eGameServer = 2,
	};
};

class CgeneserverDlg;
class GeneServerThread : public DAppThread
{
private:
	GeneServerThread();
	~GeneServerThread();

public:	
	static GeneServerThread* Instance();

	void SetType(BYTE byType){ m_byType = byType; };
	BYTE GetType(){ return m_byType;};
	string GetTypeName();

public:
	BOOL StartServer(CgeneserverDlg* pMainDlg);
	void StopServer();
	void ReloadLua(){ m_bReloadLua = TRUE; };

public:			//lua 相关函数
	void AddLog(const char* szLog);
	void SetCaption(const char* szCaption);
	int SendDBufToGameServer(DBuf* pSendDBuf);		//仅在类型是TestClient时有效
	int SendDBufToClient(DWORD dwLinkerId, DBuf* pSendDBuf);			// 仅在类型为GameServer时有效
	
public:
	virtual void ProcessMsg(DBuf* pMsgBuf);	//处理消息队列消息，客户端需要继承
	virtual void ProcessLogic();			//处理逻辑，子类需要重载

private:
	void GameServerReConn();					// 游戏服断线重连,类型eTestClient用到
	
private:
	string GetConfigFileName(string& strConfig);
	void ChangeCurDir();

private:	
	CgeneserverDlg* m_pMainDlg;
	string m_strCommCaption;					// 配置文件中定义的通用caption
	BOOL m_bLuaInit;							// 是否已经加载了lua
	BOOL m_bReloadLua;							// 是否重载LUA脚本
	string m_strLuaFile;						// 脚本文件路径
	
	BYTE m_byType;								//类型。对应GeneServerType枚举
	USHORT m_ushServerPort;						//作为服务器的监听端口
	DWORD m_dwGameServerConn;					//对游戏服的连接，类型eTestClient用到	
	stAddrInfo m_addrGameServer;				//游戏服地址信息，类型eTestClient用到
	
	ULONGLONG m_ullGameServerConnTick;
};

#endif