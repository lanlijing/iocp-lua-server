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

	void InitLinker(DIOCPServer* pIOCPServer,DAppThread* pAppThread);		//�Ӷ������ȡ������Ҫ�������ô˺�������������
	void Final();													//����ػ���ʱ���ã���Ҫ�����˻�m_NetStream������ڴ�
	void ClearupLinkerSend();
	void CloseSocket(BYTE byReason,BOOL bTellApp = TRUE);

public:
	virtual void SendAppThreadMsg(BYTE byMsgType,const char* szMsg = nullptr, int nMsgLen = 0);
	virtual void OnNorMalMessage(const char* szMsg,int nMsgLen);		//DNetStream����
	virtual void On843Message();
	virtual void OnErrorMessage();										//DNetStream����
	virtual void OnTttpRequest(const char* szMsg,int nMsgLen);			//DNetStream����
	virtual void PrintLogInfo(const char* szInfo,BOOL bViewInWnd = TRUE);
	virtual DWORD GetID(){ return m_dwID; };
	virtual stAddrInfo GetAddrInfo(){ return m_AddrInfo; };
	
public:
	void OnIOCPAccept();
	void OnIOCPRead(const char* szInBuf,DWORD dwRecv);
	void OnIOCPWrite(DWORD dwRecv);
	BOOL IOCPReadRequest();

	BOOL SendNetMsg(const char* szMsg,int nMsgLen);
	void RealSendMsg();			//�����Ӷ����з�����Ϣ

public:
	enum eConnectStatus
	{
		eNotConnected,			//��ʼ����ֵ
		eConnected,				//������
	};

public:
	eConnectStatus GetConnectStatus(){return m_ConnectStatus;};
	void SetConnectStatus(eConnectStatus eStatus){m_ConnectStatus = eStatus;};
	ULONG64 GetDestroyTick(){ return m_ullDestroyTick; };
	void SetDestroyTick(ULONG64 ullTick){ m_ullDestroyTick = ullTick; };
		
public:
	SOCKET		m_socket;		//����ΪPUBLIC����������ֱ��ʹ��		//SOCKET��Ա����Ӧ�÷������г�Ա�����ĵ�һλ���ڴ���ǰ��
	stConnIoData m_ConnIoData;		//����״̬��
	stRecvIoData m_RecvIoData;		//���ջ���
	stSendIoData m_SendIoData;		//���ͻ���
	
protected:
	static DWORD s_LinkerID;					//ÿһ�����Ӷ���һ��ID

	eConnectStatus		m_ConnectStatus;		//����״̬
	ULONG64				m_ullDestroyTick;		//���ټ�ʱ

	BOOL	m_bSendFlag;
	VecLinkerSend	m_vecSendMsg;

	DLockerBuff m_SendFlagLock;
	DLockerBuff m_vecMsgLock;
	
	DWORD m_dwID;				//ID��Ϣ
	stAddrInfo m_AddrInfo;		//Addr��Ϣ

	DAppThread* m_pAppThread;				//Ӧ�ò��߳���ָ��
	DIOCPServer* m_pIOCPServer;				//��Ӧ��IOCPServer
	DNetStream m_NetStream;					//ճ������ṹ
};



#endif