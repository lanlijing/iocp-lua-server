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
	m_dwID = ++s_LinkerID;						//�˴����õ��Ķ��߳�
	
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
	//��Ϊ�Ƕ��̣߳��п���������߳����Ѿ��ر���SOCKET������һ���̻߳��ڴ�������
	//���ԣ��������ڴ˺������������
	m_ConnIoData.nOpType = IoClose;

	//
	::shutdown(m_socket,SD_SEND);		//����shutdown����Ϊ������Դй¶,��MSDN��Ҫ��ֻ��Ҫ����SD_SEND
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
	//����IOCPServer�ص��Լ�
	m_pIOCPServer->CloseLinker(m_dwID,LinkerCloseReason::eCloseFromClientError);
}

void DIOCPLinker::OnTttpRequest(const char* szMsg,int nMsgLen)
{
	SendAppThreadMsg(DBufType::eNetServerHttpRequestMsg,szMsg,nMsgLen);
}

void DIOCPLinker::OnIOCPAccept()	//�˺�����iocp�����߳��е���
{
	m_ConnectStatus = eConnected;	
	
	sockaddr_in addr;
	memset(&addr,0,sizeof(sockaddr_in));
	int nNameLen = sizeof(sockaddr_in);
	::getpeername(m_socket,(sockaddr*)(&(addr)),&nNameLen);
	m_AddrInfo.Init();
	m_AddrInfo.m_strAddr = m_AddrInfo.m_strIP = (::inet_ntoa(addr.sin_addr));
	m_AddrInfo.m_nFarPort = addr.sin_port;
	
	//����Ӧ�ò㣬����������
	SendAppThreadMsg(DBufType::eNetServerAcceptMsg);

	//����һ���µ�read����
	IOCPReadRequest();
}

/*
����ɶ˿��߳��е��ã���onread��
��ʱ�����յ���Ϣ

��������ӽ�����Ϣ����
1��Accept�ɹ��󣬻�Ͷ��һ��IoRead������
2������ɶ˿ڹ����߳��У���������ݹ���������IoRead���¼���������YIOCPLinker::OnIoRead
3����YIOCPLinker::OnIoRead�����У������NNetStream��������ճ��
4������ճ��ʱ���������������Ϣ����ֱ�ӷ�����Ӧ�ò��߳�
5��YIOCPLinker������һ��OnIoRead���ᷢ���µ�IoRead����
*/
void DIOCPLinker::OnIOCPRead(const char* szInBuf,DWORD dwRecv)
{
	if(m_ConnectStatus == eNotConnected)	//�����Ѿ��ر�
	{
/*		char szLog[512] = {0};
		_snprintf(szLog,sizeof(szLog),"DIOCPLinker::OnIOCPRead.socket �Ѿ��رգ������յ�IoRead�¼�");
		PrintLogInfo(szLog,FALSE);*/
		return;
	}
	//���߳�����£���������һ��SOCKET�Ѿ��ر�
	m_NetStream.DealWithSocketMsg(szInBuf,dwRecv);	
	//����һ���µ�read����
	IOCPReadRequest();
}

void DIOCPLinker::OnIOCPWrite(DWORD dwRecv)
{	
	if(m_ConnectStatus == eNotConnected)	//�����Ѿ��ر�
	{
	/*	char szLog[512] = {0};
		_snprintf(szLog,sizeof(szLog),"DIOCPLinker::OnIOCPWrite.socket �Ѿ��رգ������յ�IoWrite�¼�.");
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
	if(m_ConnectStatus == eNotConnected)	//�����Ѿ��ر�
		return FALSE;

	ZeroMemory(&(m_RecvIoData.overlapped),sizeof(OVERLAPPED));
	m_RecvIoData.nOpType = IoRead;
	ZeroMemory(m_RecvIoData.databuf,NETIOBUFLEN);
	m_RecvIoData.wsabuf.buf = m_RecvIoData.databuf;
	m_RecvIoData.wsabuf.len = NETIOBUFLEN;

	//Ͷ����һ����ȡ����,һ����ɶ˿��������ݵ���ͻᴥ��ioread��Ϣ
	DWORD dwFlag = 0,dwRecvBytes =0;
	if(WSARecv(m_socket,&(m_RecvIoData.wsabuf),1,&dwRecvBytes,&dwFlag,&(m_RecvIoData.overlapped),nullptr)==SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();			//�������10038��Ϣ������SOCKET�Ѿ��رգ��ǿͻ��˷����˷Ƿ���Ϣ����OnErrorMessage�������Ѿ��ر�SOCKET
		if(dwErr != ERROR_IO_PENDING)
		{
			char szLog[512] = {0};
			_snprintf(szLog,sizeof(szLog),"DIOCPLinker::IoReadRequest.WSARecv.ERRORID:%d.",dwErr);
			PrintLogInfo(szLog,FALSE);
			//������ִ���Ӧ�ùرմ�����
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
		//�˴���д��־����Ϊ�˴���־�ܶ࣬ռ��CPU�ܸߡ���Ҫ�������߼����ͬ������
		//_snprintf(szLog,sizeof(szLog),"YIOCPLinker::SendNetMsg.socket �Ѿ��رգ������յ���Ϣ��������.����ID:%d",m_dwID);	
		//DAppLog::Instance()->AddLog(szLog);
		return FALSE;
	}

	if((!szMsg) || (nMsgLen <= 0))
	{
		PrintLogInfo("DIOCPLinker::SendNetMsg.Ҫ�͵���Ϣ����Ϊ��");
		return FALSE;
	}
	if(nMsgLen >= MAX_NETMSG_LEN)
	{
		PrintLogInfo("DIOCPLinker::SendNetMsg.Ҫ���͵���Ϣ���ڶ������󳤶�.");
		return FALSE;
	}
	
	//ÿ��ֻ����NETIOBUFLEN����ĳ��� - 1
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
	DBuf* pNewBuf = DBuf::TakeNewDBuf();	//��DAppThread::Stop()�л���
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
	m_bSendFlag = FALSE;		//����������������ں��棬����������߳�ͬʱӿ��������Ϣ������
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
		m_bSendFlag = TRUE;				//��Ϊ���ã���Ȼ��һ������Ϣ���Ͳ��ᷢ��ȥ
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
			//������ִ���Ӧ�ùرմ�����
			m_pIOCPServer->CloseLinker(m_dwID,LinkerCloseReason::eCloseFromProgramError);
			return;
		}
	}
}

void DIOCPLinker::PrintLogInfo(const char* szInfo,BOOL bViewInWnd)
{
	DAppLog::Instance()->Info(bViewInWnd, "DIOCPLinker.IP:%s.ID:%d.%s", m_AddrInfo.m_strIP.c_str(), m_dwID, szInfo);
}
 