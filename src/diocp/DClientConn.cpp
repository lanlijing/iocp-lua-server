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
	//首先关闭
	Close();
	//判断参数
	if(!szFarAddr || (!pAppThread) || (uPort == 0))
		return FALSE;
		
	//创建SOCKET
	int nResult = 0,nRet = 0;
	char szErr[512] = {0};
	m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(m_socket == INVALID_SOCKET)
	{
		nResult = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr),"创建socket出错,错误代码%d",nResult);
		PrintLogInfo(szErr);
		goto errhandle;
	}
	
	//连接
	SOCKADDR_IN saServer;
	memset(&saServer,0,sizeof(SOCKADDR_IN));

	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = ::inet_addr(szFarAddr);
	if (saServer.sin_addr.s_addr == INADDR_NONE) // 域名转IP代码
	{
		LPHOSTENT lphost;
		lphost = ::gethostbyname(szFarAddr);
		if (lphost != nullptr)
			saServer.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			::WSASetLastError(WSAEINVAL);			//设置SOCKET错误代码
			_snprintf(szErr,sizeof(szErr),"服务端IP不合法%s",szFarAddr);
			PrintLogInfo(szErr);
			goto errhandle;
		}
	}
	saServer.sin_port = ::htons(uPort);
	nRet = ::connect(m_socket,(sockaddr*)&saServer, sizeof(saServer));
	nResult = ::WSAGetLastError();
	if((nRet == SOCKET_ERROR) && (nResult != WSAEWOULDBLOCK))
	{
		_snprintf(szErr,sizeof(szErr),"连接服务器出错,可能服务器没有启动,错误代码%d",nResult);
		PrintLogInfo(szErr);
		goto errhandle;
	}
	m_bConnected = TRUE;		//表示已经连接上
	m_AddrInfo.Init();
	m_AddrInfo.m_strAddr = szFarAddr;
	m_AddrInfo.m_strIP = (::inet_ntoa(saServer.sin_addr));
	m_AddrInfo.m_nFarPort = uPort;
	m_pAppThread = pAppThread;
	m_NetStream.Init(this);

	//创建个EVENT
	m_hSocketEvent = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
	if(m_hSocketEvent == nullptr)
	{
		PrintLogInfo("创建SELECT EVENT出错");
		goto errhandle;
	}

	//绑定EVENT到网络消息(这一步会把SOCKET变成了非阻塞)
	nRet = WSAEventSelect(m_socket,m_hSocketEvent,FD_READ|FD_CLOSE);
	if(nRet == SOCKET_ERROR)
	{
		nResult = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr),"select socket event出错,错误代码为%d",nResult);
		PrintLogInfo(szErr);
		goto errhandle;
	}

	//创建线程，用于监听SOCKET的事件
	DWORD dwThreadId = 0;
	m_hSocketThread = ::CreateThread(nullptr, 0, DClientConn::SocketThreadProc,this, 0, &dwThreadId);
	if(m_hSocketThread == nullptr)
	{
		PrintLogInfo("创建SOCKET线程出错");
		goto errhandle;
	}
		
	return TRUE;

errhandle:
	Close();
	return FALSE;
}

/*
Close函数是客户端主动断开
OnClose函数是服务端断开
*/
BOOL DClientConn::Close()
{
	if(!m_bConnected)
		return TRUE;

//	PrintLogInfo("Close",FALSE);
	
	//
	m_bConnected = FALSE;

	if(m_hSocketEvent)
		WSASetEvent(m_hSocketEvent);	//产生一个事件，退出线程

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
	// 连接被关闭
	if(!m_bConnected )
		return 1;
	//首先进行消息合法判断
	if(nMsgLen >= MAX_NETMSG_LEN)
	{
		PrintLogInfo("发送的网络消息大于最大消息长度");
		return 2;
	}
	
	//每次只发送NETIOBUFLEN定义的长度-1
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

	pThis->PrintLogInfo("DClientConn:SocketThread启动",FALSE);

	WSANETWORKEVENTS events;
	DWORD dwRet = 0;
	int nRet = 0;
	while(pThis->m_bConnected)
	{
		//等待网络事件
		dwRet = WSAWaitForMultipleEvents(1,&(pThis->m_hSocketEvent),FALSE,WSA_INFINITE,FALSE);
		if(dwRet == WSA_WAIT_FAILED)
		{
			pThis->PrintLogInfo("在socketthread线程中WSAWaitForMultipleEvents出错");
			break;
		}
		
		//枚举网络事件
		nRet = WSAEnumNetworkEvents(pThis->m_socket,pThis->m_hSocketEvent,&events);		
		if (nRet == SOCKET_ERROR)
		{
			pThis->PrintLogInfo("在socketthread线程中枚举网络事件出错,连接或关闭",FALSE);
			break;
		}
		if(events.lNetworkEvents & FD_READ)
			pThis->OnRecv();
		if(events.lNetworkEvents & FD_CLOSE)
			pThis->OnClose();		
	}

	pThis->PrintLogInfo("DClientConn:SocketThread关闭",FALSE);
	
	return 0;
}

/*
服务端断开
Close是自己主动断开
*/
void DClientConn::OnClose()
{	
	SendAppThreadMsg(DBufType::eNetClientDisConnected);	//必须先发消息再执行CLOSE，否则在CLOSE中已经把线程退出。未执行的代码就不会再执行。
//	Close();	主逻辑线程调用才有效。在此调用，因为其实是在本SOCKET的消息处理线程中自己销毁自己，并不保证成功
}

void DClientConn::OnRecv()
{
	//每次最多接收NETIOBUFLEN - 1
	char bufRecv[NETIOBUFLEN] = {0};
	int nRecvLen = 0,nAllLen = 0;
	do
	{
		nRecvLen = ::recv(m_socket, bufRecv + nAllLen, (NETIOBUFLEN - 1 - nAllLen), 0);
		if(nRecvLen > 0)
			nAllLen += nRecvLen;

		::Sleep(2);	//暂定值
	}while((nRecvLen > 0) && (nAllLen < NETIOBUFLEN));
	//进行粘包处理
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

		::Sleep(2);	//暂定值
	}

	return -1;
}

void DClientConn::OnNorMalMessage(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetClientNormalMsg,szMsg,nMsgLen);
}

void DClientConn::OnErrorMessage()
{
//暂时的处理是把粘包缓冲区清空
	m_NetStream.Init(this);
}

void DClientConn::OnTttpRequest(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetClientHttpMsg,szMsg,nMsgLen);	
}

void DClientConn::SendAppThreadMsg(BYTE byMsgType,const char* szMsg,int nMsgLen)
{
	DBuf* pNewBuf = DBuf::TakeNewDBuf();	//在DAppThread::Stop()中回收
	if(pNewBuf->SetNetMsg(byMsgType,m_dwID,szMsg,nMsgLen))
		m_pAppThread->PushMsgToList(pNewBuf);
	else
		DBuf::BackDBuf(pNewBuf);
}

void DClientConn::PrintLogInfo(const char* szInfo,BOOL bViewInWnd)
{
	DAppLog::Instance()->Info(bViewInWnd,"DClientConn.IP:%s.ID:%d.%s",m_AddrInfo.m_strIP.c_str(),m_dwID,szInfo);
}