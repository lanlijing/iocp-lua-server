#ifndef _DIOCPSERVER_H_
#define _DIOCPSERVER_H_

#pragma once

#include "DIOCPLinker.h"
#include "DAppThread.h"
#include "DLockerBuff.h"

class DBackupLinkerThread;
class DIOCPServer
{
public:
	typedef BOOL (WINAPI *LPFN_ACCEPTEX)(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
	typedef map<DWORD,DIOCPLinker*> MapLinkerPtr;
public:
	DIOCPServer();
	virtual ~DIOCPServer();

	BOOL CreateServer(int nPort,DAppThread* pAppThread);
	void CloseServer();				//关闭服务器
	BOOL GetLinkerAddr(DWORD dwID,stAddrInfo& addr);
	void CloseLinker(DWORD dwID,BYTE byReason,BOOL bTellApp = TRUE);		//关闭一个连接
	void SendNetMsg(DWORD dwID,const char* szMsg,int nMsgLen);			//接到应用层一个发送网络消息的请求

public:
	enum{eMaxAcceptEx = 60,};

protected:
	BOOL CreateIOCP();			//创建IOCP
	BOOL CreateListenSocket();	//创建监听SOCKET
	
	static DWORD WINAPI ListenThreadProc(void* pParam);			//监听线程
	static DWORD WINAPI IOCPWorkThreadProc(void* pParam);			//IOCP工作线程
	
	DIOCPLinker* GetLinker();
	void CloseAllLinker();
	void OnIoAccept(DIOCPLinker* pAccept);

protected:
	int		m_nListenPort;		//监听端口

	HANDLE	m_hIOCP;			//IOCP句柄 
	SOCKET	m_ListenSocket;		//监听SOCKET
	BOOL	m_bServerStart;			//是否已经启动
	int		m_nIOCPWorkThreadNum;	//IOCP线程数量

	DAppThread* m_pAppThread;		//应用层逻辑线程
	DBackupLinkerThread* m_pBackLinkerThread;	//专门回收不再使用的连接的线程

	HANDLE	m_hLinstenThread;
	HANDLE* m_hIOCPWorkThreads;
	HANDLE  m_hAcceptEvent;			//消息事件，当放出的Acceptex已经用完后，会触发发出新的acceptex

	LPFN_ACCEPTEX	m_lpAcceptEx;			//WSAAccept的函数指针

	MapLinkerPtr		m_mapLinker;		//所有的连接
	MapLinkerPtr		m_mapTempLinker;	//保存发出去ACCEPT的连接

	DLockerBuff	m_LockTempMap;		//保护m_nAcceptExTick对象
	DLockerBuff	m_LockMap;				//保护全局连接map
};

#endif