#ifndef _DIOCPLINKER_H_
#define _DIOCPLINKER_H_

#pragma once

#include "stIoData.h"
#include "DNetStream.h"
#include "DLockerBuff.h"
#include "INetStreamObj.h"

class DAppThread;
class DIOCPServer;
class DIOCPLinker : public INetStreamObj
{
public:
	typedef vector<stLinkerSend*> VecLinkerSend;
public:
	DIOCPLinker();
	virtual ~DIOCPLinker();

	void InitLinker(DIOCPServer* pIOCPServer,DAppThread* pAppThread);		//从对象池中取出来后，要立即调用此函数，否则会出错
	void Final();													//对象池回收时调用，主要用于退还m_NetStream申请的内存
	void ClearupLinkerSend();
	void CloseSocket(BYTE byReason,BOOL bTellApp = TRUE);

public:
	virtual void SendAppThreadMsg(BYTE byMsgType,const char* szMsg = nullptr, int nMsgLen = 0);
	virtual void OnNorMalMessage(const char* szMsg,int nMsgLen);		//DNetStream调用
	virtual void On843Message();
	virtual void OnErrorMessage();										//DNetStream调用
	virtual void OnTttpRequest(const char* szMsg,int nMsgLen);			//DNetStream调用
	virtual void PrintLogInfo(const char* szInfo,BOOL bViewInWnd = TRUE);
	virtual DWORD GetID(){ return m_dwID; };
	virtual stAddrInfo GetAddrInfo(){ return m_AddrInfo; };
	
public:
	void OnIOCPAccept();
	void OnIOCPRead(const char* szInBuf,DWORD dwRecv);
	void OnIOCPWrite(DWORD dwRecv);
	BOOL IOCPReadRequest();

	BOOL SendNetMsg(const char* szMsg,int nMsgLen);
	void RealSendMsg();			//真正从队列中发送消息

public:
	enum eConnectStatus
	{
		eNotConnected,			//初始化的值
		eConnected,				//连接上
	};

public:
	eConnectStatus GetConnectStatus(){return m_ConnectStatus;};
	void SetConnectStatus(eConnectStatus eStatus){m_ConnectStatus = eStatus;};
	ULONG64 GetDestroyTick(){ return m_ullDestroyTick; };
	void SetDestroyTick(ULONG64 ullTick){ m_ullDestroyTick = ullTick; };
		
public:
	SOCKET		m_socket;		//定义为PUBLIC方便其它类直接使用		//SOCKET成员变量应该放在所有成员变量的第一位，内存最前面
	stConnIoData m_ConnIoData;		//连接状态量
	stRecvIoData m_RecvIoData;		//接收缓冲
	stSendIoData m_SendIoData;		//发送缓冲
	
protected:
	static DWORD s_LinkerID;					//每一个连接都有一个ID

	eConnectStatus		m_ConnectStatus;		//连接状态
	ULONG64				m_ullDestroyTick;		//销毁计时

	BOOL	m_bSendFlag;
	VecLinkerSend	m_vecSendMsg;

	DLockerBuff m_SendFlagLock;
	DLockerBuff m_vecMsgLock;
	
	DWORD m_dwID;				//ID信息
	stAddrInfo m_AddrInfo;		//Addr信息

	DAppThread* m_pAppThread;				//应用层线程类指针
	DIOCPServer* m_pIOCPServer;				//对应的IOCPServer
	DNetStream m_NetStream;					//粘包处理结构
};



#endif