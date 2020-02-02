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
	void CloseServer();				//�رշ�����
	BOOL GetLinkerAddr(DWORD dwID,stAddrInfo& addr);
	void CloseLinker(DWORD dwID,BYTE byReason,BOOL bTellApp = TRUE);		//�ر�һ������
	void SendNetMsg(DWORD dwID,const char* szMsg,int nMsgLen);			//�ӵ�Ӧ�ò�һ������������Ϣ������

public:
	enum{eMaxAcceptEx = 60,};

protected:
	BOOL CreateIOCP();			//����IOCP
	BOOL CreateListenSocket();	//��������SOCKET
	
	static DWORD WINAPI ListenThreadProc(void* pParam);			//�����߳�
	static DWORD WINAPI IOCPWorkThreadProc(void* pParam);			//IOCP�����߳�
	
	DIOCPLinker* GetLinker();
	void CloseAllLinker();
	void OnIoAccept(DIOCPLinker* pAccept);

protected:
	int		m_nListenPort;		//�����˿�

	HANDLE	m_hIOCP;			//IOCP��� 
	SOCKET	m_ListenSocket;		//����SOCKET
	BOOL	m_bServerStart;			//�Ƿ��Ѿ�����
	int		m_nIOCPWorkThreadNum;	//IOCP�߳�����

	DAppThread* m_pAppThread;		//Ӧ�ò��߼��߳�
	DBackupLinkerThread* m_pBackLinkerThread;	//ר�Ż��ղ���ʹ�õ����ӵ��߳�

	HANDLE	m_hLinstenThread;
	HANDLE* m_hIOCPWorkThreads;
	HANDLE  m_hAcceptEvent;			//��Ϣ�¼������ų���Acceptex�Ѿ�����󣬻ᴥ�������µ�acceptex

	LPFN_ACCEPTEX	m_lpAcceptEx;			//WSAAccept�ĺ���ָ��

	MapLinkerPtr		m_mapLinker;		//���е�����
	MapLinkerPtr		m_mapTempLinker;	//���淢��ȥACCEPT������

	DLockerBuff	m_LockTempMap;		//����m_nAcceptExTick����
	DLockerBuff	m_LockMap;				//����ȫ������map
};

#endif