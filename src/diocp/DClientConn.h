#ifndef _DCLIENTCONN_H_
#define _DCLIENTCONN_H_

#pragma once

#include "DNetStream.h"
#include "INetStreamObj.h"

/*
 *ע�⣬���߼��߳��ڴ���eClientDisConnected��Ϣʱ��������ʾ����Close,���򡣻�����Դй¶ 
 */
class DAppThread;
class DBuf;
class DClientConn : public INetStreamObj
{
public:
	DClientConn();
	virtual ~DClientConn();		//���������������ܻ��м̳�
	
public:
	virtual void SendAppThreadMsg(BYTE byMsgType,const char* szMsg = nullptr,int nMsgLen = 0);
	virtual void OnNorMalMessage(const char* szMsg,int nMsgLen);		//DNetStream����
	virtual void On843Message(){};
	virtual void OnErrorMessage();										//DNetStream����
	virtual void OnTttpRequest(const char* szMsg,int nMsgLen);			//DNetStream����
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
	
	int Send_N(const char* buf,int nBufLen);			//������ָ�������

protected:
	static DWORD s_ConnectID;
		
	SOCKET	m_socket;					//�����SOCKET
	BOOL	m_bConnected;				//�Ƿ��Ѿ�����
	HANDLE	m_hSocketThread;			//socket���߳̾��
	HANDLE  m_hSocketEvent;				//socket���¼�

	DWORD			m_dwID;				//�ڲ�ID
	stAddrInfo		m_AddrInfo;			//IP����˵ĵ�ַ��IP�Ͷ˿�
	
	DAppThread*	m_pAppThread;			//Ӧ�ó����߳�ID	
	DNetStream	m_NetStream;			//ճ������Ľṹ��
};

#endif