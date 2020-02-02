#include "stdafx.h"
#include "DIOCPLinker.h"
#include "DNetObjectPool.h"
#include "DIOCPServer.h"
#include "DNetMsgBase.h"
#include "DBuf.h"

DWORD DIOCPLinker::s_LinkerID = SERVERLINKERSTARTID;
DIOCPLinker::DIOCPLinker()
{
	m_AddrInfo.Init();
	m_dwID = ++s_LinkerID;						//此处不用担心多线程
	
	m_ConnIoData.pDIOCPLinker = this;
	m_RecvIoData.pDIOCPLinker = this;
	m_SendIoData.pDIOCPLinker = this;

	m_bSendFlag = TRUE;

	m_pAppThread = nullptr;
	m_pIOCPServer = nullptr;
}

DIOCPLinker::~DIOCPLinker()
{
}

void DIOCPLinker::InitLinker(DIOCPServer* pIOCPServer,DAppThread* pAppThread)
{
	m_NetStream.Init(this);	

	m_socket = INVALID_SOCKET;
	m_AddrInfo.Init();
	m_ConnectStatus = eNotConnected;	
	m_ullDestroyTick = 0;

	ZeroMemory(&(m_ConnIoData.overlapped),sizeof(OVERLAPPED));
	m_ConnIoData.nOpType = IoAccept;
	ZeroMemory(m_ConnIoData.databuf,stConnIoData::eBufLen);

	ZeroMemory(&(m_RecvIoData.overlapped),sizeof(OVERLAPPED));
	m_RecvIoData.nOpType = IoRead;
	ZeroMemory(m_RecvIoData.databuf,NETIOBUFLEN);
	m_RecvIoData.wsabuf.buf = m_RecvIoData.databuf;
	m_RecvIoData.wsabuf.len = NETIOBUFLEN;
		
	ZeroMemory(&(m_SendIoData.overlapped),sizeof(OVERLAPPED));
	m_SendIoData.nOpType = IoWrite;
	ZeroMemory(m_SendIoData.databuf,NETIOBUFLEN);
	m_SendIoData.InitWsaBuf();
	m_SendIoData.uValidwsabufNum = 0;
	
	m_bSendFlag = TRUE;

	m_pAppThread = pAppThread;
	m_pIOCPServer = pIOCPServer;

	ClearupLinkerSend();
}

void DIOCPLinker::Final()
{
	m_NetStream.Final();
}

void DIOCPLinker::ClearupLinkerSend()
{
	for(VecLinkerSend::iterator itVec = m_vecSendMsg.begin(); itVec != m_vecSendMsg.end(); ++itVec)
	{
		stLinkerSend* pSend = *itVec;
		DNetObjectPool::Instance()->BackLinkerSend(pSend);
	}
	m_vecSendMsg.clear();
}

void DIOCPLinker::CloseSocket(BYTE byReason, BOOL bTellApp)
{
	//因为是多线程，有可能在这个线程中已经关闭了SOCKET，但另一个线程还在处理数据
	//所以，基本不在此函数中清除数据
	m_ConnIoData.nOpType = IoClose;

	//
	::shutdown(m_socket,SD_SEND);		//加上shutdown，是为了怕资源泄露,按MSDN的要求，只需要加上SD_SEND
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	m_ConnectStatus = eNotConnected;	
	
	if (bTellApp)
	{		
		char byMsg = byReason;
		SendAppThreadMsg(DBufType::eNetServerCloseLinkerMsg, &byMsg, 1);
	}
}

void DIOCPLinker::OnNorMalMessage(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetServerNormalMsg,szMsg,nMsgLen);
}

void DIOCPLinker::On843Message()
{
	SendAppThreadMsg(DBufType::eNetServer843Msg);
}

void DIOCPLinker::OnErrorMessage()
{
	//调用IOCPServer关掉自己
	m_pIOCPServer->CloseLinker(m_dwID,LinkerCloseReason::eCloseFromClientError);
}

void DIOCPLinker::OnTttpRequest(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetServerHttpRequestMsg,szMsg,nMsgLen);
}

void DIOCPLinker::OnIOCPAccept()	//此函数在iocp工作线程中调用
{
	m_ConnectStatus = eConnected;	
	
	sockaddr_in addr;
	memset(&addr,0,sizeof(sockaddr_in));
	int nNameLen = sizeof(sockaddr_in);
	::getpeername(m_socket,(sockaddr*)(&(addr)),&nNameLen);
	m_AddrInfo.Init();
	m_AddrInfo.m_strAddr = m_AddrInfo.m_strIP = (::inet_ntoa(addr.sin_addr));
	m_AddrInfo.m_nFarPort = addr.sin_port;
	
	//告诉应用层，有连接连上
	SendAppThreadMsg(DBufType::eNetServerAcceptMsg);

	//发出一个新的read请求
	IOCPReadRequest();
}

/*
在完成端口线程中调用，在onread中
此时，接收到消息

服务端连接接收消息流程
1、Accept成功后，会投递一个IoRead的请求
2、在完成端口工作线程中，如果有数据过来。触发IoRead的事件，调用了YIOCPLinker::OnIoRead
3、在YIOCPLinker::OnIoRead函数中，会调用NNetStream类来处理粘包
4、处理粘包时，如果有完整的消息包就直接发给了应用层线程
5、YIOCPLinker处理完一个OnIoRead，会发出新的IoRead请求
*/
void DIOCPLinker::OnIOCPRead(const char* szInBuf,DWORD dwRecv)
{
	if(m_ConnectStatus == eNotConnected)	//连接已经关闭
	{
/*		char szLog[512] = {0};
		_snprintf(szLog,sizeof(szLog),"DIOCPLinker::OnIOCPRead.socket 已经关闭，但还收到IoRead事件");
		PrintLogInfo(szLog,FALSE);*/
		return;
	}
	//多线程情况下，可能在这一步SOCKET已经关闭
	m_NetStream.DealWithSocketMsg(szInBuf,dwRecv);	
	//发出一个新的read请求
	IOCPReadRequest();
}

void DIOCPLinker::OnIOCPWrite(DWORD dwRecv)
{	
	if(m_ConnectStatus == eNotConnected)	//连接已经关闭
	{
	/*	char szLog[512] = {0};
		_snprintf(szLog,sizeof(szLog),"DIOCPLinker::OnIOCPWrite.socket 已经关闭，但还收到IoWrite事件.");
		PrintLogInfo(szLog,FALSE);*/
		return;
	}

	m_SendFlagLock.Lock();
	m_bSendFlag = TRUE;
	m_SendFlagLock.Unlock();

	RealSendMsg();
}

BOOL DIOCPLinker::IOCPReadRequest()		
{
	if(m_ConnectStatus == eNotConnected)	//连接已经关闭
		return FALSE;

	ZeroMemory(&(m_RecvIoData.overlapped),sizeof(OVERLAPPED));
	m_RecvIoData.nOpType = IoRead;
	ZeroMemory(m_RecvIoData.databuf,NETIOBUFLEN);
	m_RecvIoData.wsabuf.buf = m_RecvIoData.databuf;
	m_RecvIoData.wsabuf.len = NETIOBUFLEN;

	//投递下一个读取请求,一旦完成端口上有数据到达，就会触发ioread消息
	DWORD dwFlag = 0,dwRecvBytes =0;
	if(WSARecv(m_socket,&(m_RecvIoData.wsabuf),1,&dwRecvBytes,&dwFlag,&(m_RecvIoData.overlapped),nullptr)==SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();			//如果出现10038消息，那是SOCKET已经关闭，是客户端发送了非法消息，在OnErrorMessage函数中已经关闭SOCKET
		if(dwErr != ERROR_IO_PENDING)
		{
			char szLog[512] = {0};
			_snprintf(szLog,sizeof(szLog),"DIOCPLinker::IoReadRequest.WSARecv.ERRORID:%d.",dwErr);
			PrintLogInfo(szLog,FALSE);
			//程序出现错误，应该关闭此连接
			m_pIOCPServer->CloseLinker(m_dwID,LinkerCloseReason::eCloseFromProgramError);
			return FALSE;			
		}
	}

	return TRUE;
}

BOOL DIOCPLinker::SendNetMsg(const char* szMsg,int nMsgLen)
{
	if(m_ConnectStatus == eNotConnected)		
	{		
		//此处不写日志，因为此处日志很多，占用CPU很高。主要是来自逻辑层的同步请求
		//_snprintf(szLog,sizeof(szLog),"YIOCPLinker::SendNetMsg.socket 已经关闭，但还收到消息发送请求.连接ID:%d",m_dwID);	
		//DAppLog::Instance()->AddLog(szLog);
		return FALSE;
	}

	if((!szMsg) || (nMsgLen <= 0))
	{
		PrintLogInfo("DIOCPLinker::SendNetMsg.要送的消息参数为空");
		return FALSE;
	}
	if(nMsgLen >= MAX_NETMSG_LEN)
	{
		PrintLogInfo("DIOCPLinker::SendNetMsg.要发送的消息大于定义的最大长度.");
		return FALSE;
	}
	
	//每次只发送NETIOBUFLEN定义的长度 - 1
	int nSendBufLen = 0;
	while(nSendBufLen < nMsgLen)
	{
		int nLeftLen = nMsgLen - nSendBufLen;
		int nNeedSendLen = nLeftLen > (NETIOBUFLEN - 1) ? (NETIOBUFLEN - 1) : nLeftLen;
		stLinkerSend* pSend = DNetObjectPool::Instance()->TakeLinkerSend(szMsg + nSendBufLen,nNeedSendLen);
		m_vecMsgLock.Lock();
		m_vecSendMsg.push_back(pSend);
		m_vecMsgLock.Unlock();
		nSendBufLen += nNeedSendLen;	
	}
	
	RealSendMsg();

	return TRUE;
}

void DIOCPLinker::SendAppThreadMsg(BYTE byMsgType, const char* szMsg, int nMsgLen)
{
	DBuf* pNewBuf = DBuf::TakeNewDBuf();	//在DAppThread::Stop()中回收
	if (pNewBuf->SetNetMsg(byMsgType, m_dwID, szMsg, nMsgLen))
		m_pAppThread->PushMsgToList(pNewBuf);
	else
		DBuf::BackDBuf(pNewBuf);
}

void DIOCPLinker::RealSendMsg()	
{
	m_SendFlagLock.Lock();
	if(m_bSendFlag == FALSE)
	{
		m_SendFlagLock.Unlock();
		return;
	}
	m_bSendFlag = FALSE;		//必须加在这里，如果加在后面，会出现两个线程同时涌进来发消息的事情
	m_SendFlagLock.Unlock();
	 
	ZeroMemory(&(m_SendIoData.overlapped),sizeof(OVERLAPPED));
	m_SendIoData.nOpType = IoWrite;
	ZeroMemory(m_SendIoData.databuf,NETIOBUFLEN);	
	m_SendIoData.InitWsaBuf();
	m_SendIoData.uValidwsabufNum = 0;
	
	stLinkerSend* pLinkerSend = nullptr;
	VecLinkerSend::iterator itVec;
	int nBufTotalLen = 0;
	
	m_vecMsgLock.Lock();
	if(m_vecSendMsg.empty())
	{
		m_vecMsgLock.Unlock();

		m_SendFlagLock.Lock();
		m_bSendFlag = TRUE;				//置为可用，不然下一次有消息来就不会发出去
		m_SendFlagLock.Unlock();

		return;
	}
	for(itVec = m_vecSendMsg.begin(); itVec != m_vecSendMsg.end();)
	{
		if(m_SendIoData.uValidwsabufNum >= MAXSENDWSABUF)
			break;

		pLinkerSend = *itVec;
		if((nBufTotalLen + pLinkerSend->m_nMsgLen) < NETIOBUFLEN)	
		{			
			CopyMemory(m_SendIoData.databuf + nBufTotalLen,pLinkerSend->m_szMsg,pLinkerSend->m_nMsgLen);
			m_SendIoData.wsabuf[m_SendIoData.uValidwsabufNum].buf = (m_SendIoData.databuf + nBufTotalLen);
			m_SendIoData.wsabuf[m_SendIoData.uValidwsabufNum].len = pLinkerSend->m_nMsgLen;
			
			++(m_SendIoData.uValidwsabufNum);
			nBufTotalLen += pLinkerSend->m_nMsgLen;

			DNetObjectPool::Instance()->BackLinkerSend(pLinkerSend);
			itVec = m_vecSendMsg.erase(itVec);
		}
		else
			break;
	}
	m_vecMsgLock.Unlock();

	char szLog[256] = {0};	
	//
	DWORD dwTempSendNum = 0;
	int nResult = ::WSASend(m_socket,m_SendIoData.wsabuf,m_SendIoData.uValidwsabufNum,&dwTempSendNum,0,&(m_SendIoData.overlapped),nullptr);	
	if(nResult == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		if(dwErr != ERROR_IO_PENDING)
		{
			_snprintf(szLog,sizeof(szLog),"DIOCPLinker::SendNetMsg.WSASend.ErrorID:%d",dwErr);
			PrintLogInfo(szLog);
			//程序出现错误，应该关闭此连接
			m_pIOCPServer->CloseLinker(m_dwID,LinkerCloseReason::eCloseFromProgramError);
			return;
		}
	}
}

void DIOCPLinker::PrintLogInfo(const char* szInfo,BOOL bViewInWnd)
{
	DAppLog::Instance()->Info(bViewInWnd, "DIOCPLinker.IP:%s.ID:%d.%s", m_AddrInfo.m_strIP.c_str(), m_dwID, szInfo);
}
 