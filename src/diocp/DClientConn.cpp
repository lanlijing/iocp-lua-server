#include "stdafx.h"
#include "DClientConn.h"
#include "DNetObjectPool.h"
#include "DBuf.h"
#include "DAppThread.h"
#include "DAppLog.h"
#include "DNetMsgBase.h"
#include "DBuf.h"

DWORD DClientConn::s_ConnectID = CLIENTCONNSTARTID;
DClientConn::DClientConn()
{
	m_AddrInfo.Init();
	m_dwID = ++s_ConnectID;
	
	m_socket = INVALID_SOCKET;
	m_bConnected = FALSE;
	m_hSocketThread = nullptr;
	m_hSocketEvent = nullptr;

	m_pAppThread = nullptr;
}

DClientConn::~DClientConn()
{
	if(m_bConnected)
		Close();
}

BOOL DClientConn::Connect(const char* szFarAddr,USHORT uPort,DAppThread* pAppThread)
{
	//���ȹر�
	Close();
	//�жϲ���
	if(!szFarAddr || (!pAppThread) || (uPort == 0))
		return FALSE;
		
	//����SOCKET
	int nResult = 0,nRet = 0;
	char szErr[512] = {0};
	m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(m_socket == INVALID_SOCKET)
	{
		nResult = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr),"����socket����,�������%d",nResult);
		PrintLogInfo(szErr);
		goto errhandle;
	}
	
	//����
	SOCKADDR_IN saServer;
	memset(&saServer,0,sizeof(SOCKADDR_IN));

	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = ::inet_addr(szFarAddr);
	if (saServer.sin_addr.s_addr == INADDR_NONE) // ����תIP����
	{
		LPHOSTENT lphost;
		lphost = ::gethostbyname(szFarAddr);
		if (lphost != nullptr)
			saServer.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			::WSASetLastError(WSAEINVAL);			//����SOCKET�������
			_snprintf(szErr,sizeof(szErr),"�����IP���Ϸ�%s",szFarAddr);
			PrintLogInfo(szErr);
			goto errhandle;
		}
	}
	saServer.sin_port = ::htons(uPort);
	nRet = ::connect(m_socket,(sockaddr*)&saServer, sizeof(saServer));
	nResult = ::WSAGetLastError();
	if((nRet == SOCKET_ERROR) && (nResult != WSAEWOULDBLOCK))
	{
		_snprintf(szErr,sizeof(szErr),"���ӷ���������,���ܷ�����û������,�������%d",nResult);
		PrintLogInfo(szErr);
		goto errhandle;
	}
	m_bConnected = TRUE;		//��ʾ�Ѿ�������
	m_AddrInfo.Init();
	m_AddrInfo.m_strAddr = szFarAddr;
	m_AddrInfo.m_strIP = (::inet_ntoa(saServer.sin_addr));
	m_AddrInfo.m_nFarPort = uPort;
	m_pAppThread = pAppThread;
	m_NetStream.Init(this);

	//������EVENT
	m_hSocketEvent = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
	if(m_hSocketEvent == nullptr)
	{
		PrintLogInfo("����SELECT EVENT����");
		goto errhandle;
	}

	//��EVENT��������Ϣ(��һ�����SOCKET����˷�����)
	nRet = WSAEventSelect(m_socket,m_hSocketEvent,FD_READ|FD_CLOSE);
	if(nRet == SOCKET_ERROR)
	{
		nResult = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr),"select socket event����,�������Ϊ%d",nResult);
		PrintLogInfo(szErr);
		goto errhandle;
	}

	//�����̣߳����ڼ���SOCKET���¼�
	DWORD dwThreadId = 0;
	m_hSocketThread = ::CreateThread(nullptr, 0, DClientConn::SocketThreadProc,this, 0, &dwThreadId);
	if(m_hSocketThread == nullptr)
	{
		PrintLogInfo("����SOCKET�̳߳���");
		goto errhandle;
	}
		
	return TRUE;

errhandle:
	Close();
	return FALSE;
}

/*
Close�����ǿͻ��������Ͽ�
OnClose�����Ƿ���˶Ͽ�
*/
BOOL DClientConn::Close()
{
	if(!m_bConnected)
		return TRUE;

//	PrintLogInfo("Close",FALSE);
	
	//
	m_bConnected = FALSE;

	if(m_hSocketEvent)
		WSASetEvent(m_hSocketEvent);	//����һ���¼����˳��߳�

	::closesocket(m_socket);
	m_socket = INVALID_SOCKET;

	if(m_hSocketThread != nullptr)
	{
		::WaitForSingleObject(m_hSocketThread,THREADWAITOBJECTTIME);
		::TerminateThread(m_hSocketThread,0);
		m_hSocketThread = nullptr;
	}

	::CloseHandle(m_hSocketEvent);
	m_hSocketEvent = nullptr;

	m_AddrInfo.Init();

	m_pAppThread = nullptr;
	m_NetStream.Final();

	return TRUE;
}

int DClientConn::Send(DBuf* pBuf)
{
	return Send(pBuf->GetBuf(),pBuf->GetLength());
}

int DClientConn::Send(const char* pSendMsg,int nMsgLen)
{	
	// ���ӱ��ر�
	if(!m_bConnected )
		return 1;
	//���Ƚ�����Ϣ�Ϸ��ж�
	if(nMsgLen >= MAX_NETMSG_LEN)
	{
		PrintLogInfo("���͵�������Ϣ���������Ϣ����");
		return 2;
	}
	
	//ÿ��ֻ����NETIOBUFLEN����ĳ���-1
	int nSendBufLen = 0;
	while(nSendBufLen < nMsgLen)
	{
		int nLeftLen = nMsgLen - nSendBufLen;
		int nNeedSendLen = nLeftLen > (NETIOBUFLEN - 1) ? (NETIOBUFLEN - 1) : nLeftLen;
		Send_N(pSendMsg + nSendBufLen,nNeedSendLen);
		nSendBufLen += nNeedSendLen;	  
	}

	return 0;
}

DWORD WINAPI DClientConn::SocketThreadProc(void* pParam)
{
	DClientConn* pThis = (DClientConn*)pParam;

	pThis->PrintLogInfo("DClientConn:SocketThread����",FALSE);

	WSANETWORKEVENTS events;
	DWORD dwRet = 0;
	int nRet = 0;
	while(pThis->m_bConnected)
	{
		//�ȴ������¼�
		dwRet = WSAWaitForMultipleEvents(1,&(pThis->m_hSocketEvent),FALSE,WSA_INFINITE,FALSE);
		if(dwRet == WSA_WAIT_FAILED)
		{
			pThis->PrintLogInfo("��socketthread�߳���WSAWaitForMultipleEvents����");
			break;
		}
		
		//ö�������¼�
		nRet = WSAEnumNetworkEvents(pThis->m_socket,pThis->m_hSocketEvent,&events);		
		if (nRet == SOCKET_ERROR)
		{
			pThis->PrintLogInfo("��socketthread�߳���ö�������¼�����,���ӻ�ر�",FALSE);
			break;
		}
		if(events.lNetworkEvents & FD_READ)
			pThis->OnRecv();
		if(events.lNetworkEvents & FD_CLOSE)
			pThis->OnClose();		
	}

	pThis->PrintLogInfo("DClientConn:SocketThread�ر�",FALSE);
	
	return 0;
}

/*
����˶Ͽ�
Close���Լ������Ͽ�
*/
void DClientConn::OnClose()
{	
	SendAppThreadMsg(DBufType::eNetClientDisConnected);	//�����ȷ���Ϣ��ִ��CLOSE��������CLOSE���Ѿ����߳��˳���δִ�еĴ���Ͳ�����ִ�С�
//	Close();	���߼��̵߳��ò���Ч���ڴ˵��ã���Ϊ��ʵ���ڱ�SOCKET����Ϣ�����߳����Լ������Լ���������֤�ɹ�
}

void DClientConn::OnRecv()
{
	//ÿ��������NETIOBUFLEN - 1
	char bufRecv[NETIOBUFLEN] = {0};
	int nRecvLen = 0,nAllLen = 0;
	do
	{
		nRecvLen = ::recv(m_socket, bufRecv + nAllLen, (NETIOBUFLEN - 1 - nAllLen), 0);
		if(nRecvLen > 0)
			nAllLen += nRecvLen;

		::Sleep(2);	//�ݶ�ֵ
	}while((nRecvLen > 0) && (nAllLen < NETIOBUFLEN));
	//����ճ������
	m_NetStream.DealWithSocketMsg(bufRecv,nAllLen);
}

int DClientConn::Send_N(const char* buf,int nBufLen)
{
	if((!buf) || (nBufLen < 1))
		return -1;

	for(int s = 0, t = 0 ; ;)
	{
		t = ::send(m_socket, buf + s, nBufLen - s, 0);
		if (t < 1) 
			return -1;
		s += t;
		if (nBufLen == s) 
			return s;

		::Sleep(2);	//�ݶ�ֵ
	}

	return -1;
}

void DClientConn::OnNorMalMessage(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetClientNormalMsg,szMsg,nMsgLen);
}

void DClientConn::OnErrorMessage()
{
//��ʱ�Ĵ����ǰ�ճ�����������
	m_NetStream.Init(this);
}

void DClientConn::OnTttpRequest(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetClientHttpMsg,szMsg,nMsgLen);	
}

void DClientConn::SendAppThreadMsg(BYTE byMsgType,const char* szMsg,int nMsgLen)
{
	DBuf* pNewBuf = DBuf::TakeNewDBuf();	//��DAppThread::Stop()�л���
	if(pNewBuf->SetNetMsg(byMsgType,m_dwID,szMsg,nMsgLen))
		m_pAppThread->PushMsgToList(pNewBuf);
	else
		DBuf::BackDBuf(pNewBuf);
}

void DClientConn::PrintLogInfo(const char* szInfo,BOOL bViewInWnd)
{
	DAppLog::Instance()->Info(bViewInWnd,"DClientConn.IP:%s.ID:%d.%s",m_AddrInfo.m_strIP.c_str(),m_dwID,szInfo);
}