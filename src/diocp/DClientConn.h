#ifndef _DCLIENTCONN_H_
#define _DCLIENTCONN_H_

#pragma once

#include "DNetStream.h"
#include "INetStreamObj.h"

/*
 *注意，主逻辑线程在处理eClientDisConnected消息时，必须显示调用Close,否则。会有资源泄露 
 */
class DAppThread;
class DBuf;
class DClientConn : public INetStreamObj
{
public:
	DClientConn();
	virtual ~DClientConn();		//虚析构函数，可能会有继承
	
public:
	virtual void SendAppThreadMsg(BYTE byMsgType,const char* szMsg = nullptr,int nMsgLen = 0);
	virtual void OnNorMalMessage(const char* szMsg,int nMsgLen);		//DNetStream调用
	virtual void On843Message(){};
	virtual void OnErrorMessage();										//DNetStream调用
	virtual void OnTttpRequest(const char* szMsg,int nMsgLen);			//DNetStream调用
	virtual void PrintLogInfo(const char* szInfo,BOOL bViewInWnd = TRUE);
	virtual DWORD GetID(){ return m_dwID; };
	virtual stAddrInfo GetAddrInfo(){ return m_AddrInfo; };

public:
	BOOL Connect(const char* szFarAddr,USHORT uPort,DAppThread* pAppThread);
	BOOL Close();				
	int Send(DBuf* pBuf);
	int Send(const char* pSendMsg,int nMsgLen);	
	BOOL IsConnect(){return m_bConnected;};		

public:
	static DWORD WINAPI SocketThreadProc(void* pParam);

protected:
	void OnClose();
	void OnRecv();
	
	int Send_N(const char* buf,int nBufLen);			//发送完指令的数据

protected:
	static DWORD s_ConnectID;
		
	SOCKET	m_socket;					//自身的SOCKET
	BOOL	m_bConnected;				//是否已经连接
	HANDLE	m_hSocketThread;			//socket的线程句柄
	HANDLE  m_hSocketEvent;				//socket的事件

	DWORD			m_dwID;				//内部ID
	stAddrInfo		m_AddrInfo;			//IP服务端的地址、IP和端口
	
	DAppThread*	m_pAppThread;			//应用程序线程ID	
	DNetStream	m_NetStream;			//粘包处理的结构体
};

#endif