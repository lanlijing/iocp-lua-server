#include "stdafx.h"
#include "DNetObjectPool.h"
#include "DIOCPServer.h"
#include "DBackupLinkerThread.h"
#include "DAppLog.h"
#include <MSWSock.h>

#pragma comment(lib, "ws2_32.lib")

DIOCPServer::DIOCPServer()
{
	m_nListenPort = 0;

	m_hIOCP = nullptr;
	m_ListenSocket = INVALID_SOCKET;
	m_bServerStart = FALSE;
	m_nIOCPWorkThreadNum = 0;

	m_hLinstenThread = nullptr;
	m_hIOCPWorkThreads = nullptr;
	m_hAcceptEvent = nullptr; 

	m_lpAcceptEx = nullptr;
	m_pBackLinkerThread = nullptr; 

	m_pAppThread = nullptr;
}

DIOCPServer::~DIOCPServer()
{
	if(m_bServerStart)
		CloseServer();
}

BOOL DIOCPServer::CreateServer(int nPort,DAppThread* pAppThread)
{
	//�ж��Ƿ������ö���
	if(!pAppThread)
	{
		::MessageBoxA(nullptr,"����IOCP����������ȱʧ","DIOCPServer",MB_OK);
		return FALSE;
	}
	m_pAppThread = pAppThread;
	m_nListenPort = nPort;

	//�رշ���
	CloseServer();	

	//����ʵ�ʻ���ServerLinker�߳�
	m_pBackLinkerThread = new DBackupLinkerThread();
	m_pBackLinkerThread->Start();
		
	//����IOCP���
	BOOL bRet = CreateIOCP();
	if(!bRet)
		return FALSE;
	
	//��������SOCKET���
	bRet = CreateListenSocket();	
	if(!bRet)
		return FALSE;
	
	m_bServerStart = TRUE;
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CreateServer.IOCP Server Start");

	return TRUE;
}

void DIOCPServer::CloseServer()
{
	if(!m_bServerStart)
		return;

	m_bServerStart = FALSE;

	//��IOCP�̷߳����˳���Ϣ
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.��IOCP�̷߳����˳���Ϣ");	
	for(int i = 0; i < m_nIOCPWorkThreadNum; i++)
		PostQueuedCompletionStatus(m_hIOCP, 0, 0, nullptr);
		
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.�ȴ��ر�IOCP�߳�");
	//�߳����	
	for(int i = 0; i < m_nIOCPWorkThreadNum; i++)
	{
		::WaitForSingleObject(m_hIOCPWorkThreads[i],THREADWAITOBJECTTIME);
		::TerminateThread(m_hIOCPWorkThreads[i],0);
		::CloseHandle(m_hIOCPWorkThreads[i]);
		m_hIOCPWorkThreads[i] = nullptr;
	}
	delete[] m_hIOCPWorkThreads;
	m_hIOCPWorkThreads = nullptr;
	m_nIOCPWorkThreadNum = 0;
	//	
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.�رռ����߳�");
	::WaitForSingleObject(m_hLinstenThread,THREADWAITOBJECTTIME);
	::TerminateThread(m_hLinstenThread,0);
	CloseHandle(m_hLinstenThread);
	m_hLinstenThread = nullptr;

	::CloseHandle(m_hAcceptEvent);
	m_hAcceptEvent = nullptr;
			
	//�ر���������
	CloseAllLinker();
	if(m_pBackLinkerThread)
	{
		m_pBackLinkerThread->Stop();
		delete m_pBackLinkerThread;
		m_pBackLinkerThread = nullptr;
	}
	//�رռ���SOCKET
	::closesocket(m_ListenSocket);
	m_ListenSocket = INVALID_SOCKET;
	//������ɶ˿�
	::CloseHandle(m_hIOCP);
	m_hIOCP = nullptr;
	//
	m_pAppThread = nullptr;	
	m_nListenPort = 0;
	//
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.IOCP Server Close");
}

BOOL DIOCPServer::GetLinkerAddr(DWORD dwID, stAddrInfo& addr)
{
	DIOCPLinker* pDLinker = nullptr;
	MapLinkerPtr::iterator itFind;

	{
		DAutoLock autolock(m_LockMap);
		if ((itFind = m_mapLinker.find(dwID)) != m_mapLinker.end())
		{
			pDLinker = itFind->second;
			addr = pDLinker->GetAddrInfo();
		}
	}

	return (pDLinker != nullptr);
}

void DIOCPServer::CloseLinker(DWORD dwID,BYTE byReason,BOOL bTellApp)
{
	DIOCPLinker* pDLinker = nullptr;
	MapLinkerPtr::iterator itFind;

	{
		DAutoLock autolock(m_LockTempMap);
		if((itFind = m_mapTempLinker.find(dwID)) != m_mapTempLinker.end())
			m_mapTempLinker.erase(itFind);
		if(m_mapTempLinker.size() <= 0)
		{
			::SetEvent(m_hAcceptEvent);
		}
	}
	{
		DAutoLock autolock(m_LockMap);
		if((itFind = m_mapLinker.find(dwID)) != m_mapLinker.end())
		{
			pDLinker = itFind->second;
			pDLinker->CloseSocket(byReason,bTellApp);				//�ر�SOCKET,ACCEPTǰ��������Ҫ֪ͨӦ�ò�
			pDLinker->SetDestroyTick(GetTickCount64());	//�������ټ�ʱ
			m_mapLinker.erase(itFind);		//��ʹ�ö�����ɾ��

			stAddrInfo addrInfo = pDLinker->GetAddrInfo();
			DAppLog::Instance()->Info(FALSE, "DIOCPServer::CloseLinker.IP:%s.ID:%d��������%dԭ��ر�", addrInfo.m_strIP.c_str(), dwID, byReason);

			m_pBackLinkerThread->PushBackedLinker(pDLinker);
		}
		else	//�ر�һ�����ڶ����е���Ϣ��������������Ŀ������У�������Ѿ��ر��˸����ӣ���Ӧ�ò��IOCP�̻߳������رյ���Ϣ��
		{
			DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseLinker.��ͼ�ر�һ�����ڶ����е�����:ID:%d,��Դ:%d",dwID,byReason);
		}
	}
}

void DIOCPServer::SendNetMsg(DWORD dwID,const char* szMsg,int nMsgLen)
{
	DIOCPLinker* pDLinker = nullptr;
	MapLinkerPtr::iterator itFind;
	{
		DAutoLock autolock(m_LockMap);
		if((itFind = m_mapLinker.find(dwID)) != m_mapLinker.end())
			pDLinker = itFind->second;
	}
	if(pDLinker)
		pDLinker->SendNetMsg(szMsg,nMsgLen);
	else	//�������һ������������������Կͻ��˵������Ѿ��Ͽ�,��Ӧ�ò���δ���������ӹ����Ķ���
	{
	//	DAppLog::Instance()->Info(FALSE,"DIOCPServer::SendNetMsg.Ҫ���ڶ����е�Linker������Ϣ.�����ǵײ������Ѿ��Ͽ�,Ӧ�ò����������δ����:%s %d",socketInfo.m_szIP,socketInfo.m_nInneID);
	}
}

BOOL DIOCPServer::CreateIOCP()
{
	//����IOCP������߳�
	char szErr[512] = {0};
	DWORD dwErr = 0;
	DWORD dwThreadID = 0;

	m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (nullptr == m_hIOCP)
	{
		::MessageBoxA(nullptr,"����IOCP����","DIOCPServer",MB_OK);
		return FALSE;
	}
	//�õ�IOCP�߳���
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	m_nIOCPWorkThreadNum = (sysinfo.dwNumberOfProcessors * 2) + 2;
	
	m_hIOCPWorkThreads = new HANDLE[m_nIOCPWorkThreadNum];
	for(int i = 0; i < m_nIOCPWorkThreadNum; i++)
	{
		dwThreadID = 0;
		m_hIOCPWorkThreads[i] = ::CreateThread(nullptr, 0, DIOCPServer::IOCPWorkThreadProc,this, 0, &dwThreadID);
		if(!(m_hIOCPWorkThreads[i]))
		{
			dwErr = ::WSAGetLastError();
			_snprintf(szErr,sizeof(szErr) - 1,"����iocp�̳߳���%d",dwErr);
			::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL DIOCPServer::CreateListenSocket()
{
	char szErr[512] = {0};
	DWORD dwErr = 0;	
	//��������SOCKET�������߳�
	DWORD dwThreadID = 0;

	m_ListenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_ListenSocket)
	{
		dwErr = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"��������SOCKET����%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//��ȡacceptex����ָ��
	GUID GUIDAcceptEx = WSAID_ACCEPTEX;
	DWORD dwResult = 0;
	dwErr = ::WSAIoctl(m_ListenSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&GUIDAcceptEx,sizeof(GUIDAcceptEx),&m_lpAcceptEx,sizeof(m_lpAcceptEx),
		&dwResult,nullptr,nullptr);
	if(dwErr == SOCKET_ERROR)
	{ 
		_snprintf(szErr,sizeof(szErr) - 1,"��ȡAcceptEx����ָ�����%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//������SOCKET����ɶ˿ڹ�������
	HANDLE hrc = ::CreateIoCompletionPort((HANDLE)m_ListenSocket,m_hIOCP,0,0);
	if(hrc == nullptr)
	{
		dwErr = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"����SOCKET����ɶ˿ڳ���%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//�������׽���m_ListenSocket�󶨵�����IP��ַ�������ڼ���ģʽ��
	SOCKADDR_IN myAddr;
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	myAddr.sin_port = htons(m_nListenPort);
	dwErr = bind(m_ListenSocket,(sockaddr*)(&myAddr),sizeof(SOCKADDR_IN));
	if(dwErr == SOCKET_ERROR)
	{
		_snprintf(szErr,sizeof(szErr) - 1,"����socket�󶨶˿ڳ���%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	dwErr = listen(m_ListenSocket, 20);
	if(dwErr == SOCKET_ERROR)
	{
		_snprintf(szErr,sizeof(szErr) - 1,"listen����%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}	
	//����ListenEvent
	m_hAcceptEvent = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
	if (nullptr == m_hAcceptEvent)
	{
		dwErr = ::GetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"����Accept�¼�%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//����listenthread
	m_hLinstenThread  = ::CreateThread(nullptr, 0, DIOCPServer::ListenThreadProc,this, 0, &dwThreadID);
	if(m_hLinstenThread == nullptr)
	{
		dwErr = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"���������̳߳���%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
		
	return TRUE;
}

DWORD WINAPI DIOCPServer::ListenThreadProc(void* pParam)
{
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::ListenThreadProc����");

	DIOCPServer* pThis = (DIOCPServer*)pParam;	
	::SetEvent(pThis->m_hAcceptEvent);
	while(TRUE)
	{
		::WaitForSingleObject(pThis->m_hAcceptEvent,INFINITE);	//����������ACCEPT��û��ʹ���꣬��һֱ�ȴ�

		for(int i = 0; i < eMaxAcceptEx; i++)
		{
			SOCKET sNewSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP,nullptr, 0, WSA_FLAG_OVERLAPPED);			
			if(sNewSocket == INVALID_SOCKET)
			{
				DAppLog::Instance()->Info(FALSE,"DIOCPServer::ListenThreadProc.WSASocket failed with error code: %d", WSAGetLastError());
				continue;  
			}
			
			DIOCPLinker* pNewLinker = pThis->GetLinker();
			if(!pNewLinker)
				continue;
			pNewLinker->m_socket = sNewSocket;

			stConnIoData* pIoData = &(pNewLinker->m_ConnIoData);
			
			DWORD dwBytes;
			BOOL bSuccess = pThis->m_lpAcceptEx(pThis->m_ListenSocket,pNewLinker->m_socket,pIoData->databuf,0,sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16,&dwBytes,&(pIoData->overlapped));
			if(!bSuccess)
			{
				DWORD dwWSAErr = ::WSAGetLastError();
				if(dwWSAErr != ERROR_IO_PENDING)
				{
					DAppLog::Instance()->Info(FALSE,"DIOCPServer::ListenThreadProc.acceptex failed.linkerid:%d.error code: %d.",
										pNewLinker->GetID(),dwWSAErr);
					pThis->CloseLinker(pNewLinker->GetID(),LinkerCloseReason::eCloseFromProgramError,FALSE);
					break;
				}				
			}			
		}
	}

	DAppLog::Instance()->Info(FALSE,"DIOCPServer::ListenThreadProc�ر�");

	return 0;
}

DWORD WINAPI DIOCPServer::IOCPWorkThreadProc(void* pParam)
{
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::IOCPWorkThreadProc.����");

	DIOCPServer* pThis = (DIOCPServer*)pParam;
	DIOCPLinker* pDLinker = nullptr;
	stIoData* pIoData = nullptr;
	LPOVERLAPPED pOverlapped;
	DWORD dwRecvBytes = 0;
	DWORD dwErrorCode = 0;
	
	while(TRUE)	
	{
		BOOL bRet = ::GetQueuedCompletionStatus(pThis->m_hIOCP,&dwRecvBytes,(PULONG_PTR)(&pDLinker),&pOverlapped,INFINITE);	
		if(bRet)	//�����ж��Ƿ���Ҫ�˳�
		{
			if((dwRecvBytes == 0)  && (pDLinker == nullptr) && (pOverlapped == nullptr))	
			{
				//PostQueuedCompletionStatus������һ���յĵ�������ݣ���ʾ�߳�Ҫ�˳��ˡ�
				DAppLog::Instance()->Info(FALSE,"DIOCPServer::IOCPWorkThreadProc.PostQueuedCompletionStatus,IOCPWorkThread queryed quit");
				break;
			}	
		}
		if(pOverlapped != nullptr)
		{
			pIoData = CONTAINING_RECORD(pOverlapped,stIoData,overlapped);
			if(!pIoData)
			{
				DAppLog::Instance()->Info(FALSE,"DIOCPServer::IOCPWorkThreadProc.IOCPWorkThread CONTAINING_RECORD ERROR");
				break;
			}
			
			if(!pDLinker)
				pDLinker = (DIOCPLinker*)pIoData->pDIOCPLinker;
		}
		if(pIoData->nOpType == IoAccept)
		{
			//���ܳɹ�ʧ�ܶ���1
			BOOL bIsAcceptLinker = FALSE;			
			{
				MapLinkerPtr::iterator itFind;	
				DAutoLock autolock(pThis->m_LockTempMap);
				if((itFind = pThis->m_mapTempLinker.find(pDLinker->GetID())) != pThis->m_mapTempLinker.end())
				{
					bIsAcceptLinker = TRUE;
					pThis->m_mapTempLinker.erase(itFind);
				}
				if(pThis->m_mapTempLinker.size() <= 0)
				{
					::SetEvent(pThis->m_hAcceptEvent);
				}
			}

			if(bIsAcceptLinker && bRet)
			{
				pThis->OnIoAccept(pDLinker);
				continue;
			}
			else if(!bRet)		
				DAppLog::Instance()->Info(FALSE,"iocp acceptxt failed.ID:%d",pDLinker->GetID());
		}
		if(!bRet)
		{
			dwErrorCode = ::GetLastError();
			DAppLog::Instance()->Info(FALSE,"GetQueuedCompletionStatus ID:%d,����:%d",pDLinker->GetID(),dwErrorCode);
			if((dwErrorCode == ERROR_NETNAME_DELETED/*64*/) || (dwErrorCode == ERROR_CONNECTION_ABORTED/*1236*/))
				pThis->CloseLinker(pDLinker->GetID(),LinkerCloseReason::eCloseFromClientError);
		}
		else
		{
			if(dwRecvBytes == 0)	//�ͻ����˳�
			{
				if((pIoData->nOpType == IoRead) || (pIoData->nOpType == IoWrite))
					pThis->CloseLinker(pDLinker->GetID(),LinkerCloseReason::eCloseFromClient);			
			}			
			if(pIoData->nOpType == IoRead)	//ioread
			{
				stRecvIoData* pRecvData = (stRecvIoData*)pIoData;
				pDLinker->OnIOCPRead(pRecvData->wsabuf.buf,dwRecvBytes);
			}
			else if(pIoData->nOpType == IoWrite)	//ioWrite
			{
				pDLinker->OnIOCPWrite(dwRecvBytes);
			}			
			else if(pIoData->nOpType == IoClose)
			{
			}
		}
	}

	DAppLog::Instance()->Info(FALSE,"DIOCPServer::IOCPWorkThreadProc.�ر�");

	return 0;
}

DIOCPLinker* DIOCPServer::GetLinker()
{
	DIOCPLinker* pLinker = DNetObjectPool::Instance()->TakeServerLinker();	//��CloseLinker�� ��CloseAllLinker ����
	pLinker->InitLinker(this,m_pAppThread);		//��ʼ��
	if(!pLinker)
	{
		DAppLog::Instance()->Info(FALSE,"DIOCPServer::GetLinker.��ȡ�µ����Ӷ������");
		return nullptr;
	}
		
	{
		DAutoLock autolock(m_LockMap);
		m_mapLinker.insert(make_pair(pLinker->GetID(),pLinker));
	}
	{
		DAutoLock autolock(m_LockTempMap);
		m_mapTempLinker.insert(make_pair(pLinker->GetID(),pLinker));
	}
	
	return pLinker;
}

void DIOCPServer::CloseAllLinker()
{
	{
		DAutoLock autolock(m_LockTempMap);
		m_mapTempLinker.clear();
	}
	{
		DAutoLock autolock(m_LockMap);
		for(MapLinkerPtr::iterator iter = m_mapLinker.begin(); iter != m_mapLinker.end(); ++iter)
		{
			DIOCPLinker* pDLinker = iter->second;
			pDLinker->CloseSocket(LinkerCloseReason::eCloseFromApp,FALSE);
			DNetObjectPool::Instance()->BackServerLinker(pDLinker);					//��GetLinker�л��
		}
		m_mapLinker.clear();
	}
}

void DIOCPServer::OnIoAccept(DIOCPLinker* pAccept)
{
	//��������������	
	char szLog[512] = {0};
	int nResult = 0;
	nResult = setsockopt(pAccept->m_socket,SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,(char*)&m_ListenSocket,sizeof(m_ListenSocket));
	if(SOCKET_ERROR == nResult)
	{
		_snprintf(szLog,sizeof(szLog),"DIOCPServer::OnIoAccept.setsockopt SO_UPDATE_ACCEPT_CONTEXT failed error code:%d", ::WSAGetLastError()); 
		pAccept->PrintLogInfo(szLog);
		goto errHandle;
	}	
	//����IOCP
	if(CreateIoCompletionPort((HANDLE)pAccept->m_socket,m_hIOCP,(DWORD_PTR)pAccept,0) != m_hIOCP)
	{
		_snprintf(szLog,sizeof(szLog),"DIOCPServer::OnIoAccept.CreateIoCompletionPort failed error code:%d", ::GetLastError()); 
		pAccept->PrintLogInfo(szLog);
		goto errHandle;
	}
		
	//����DIOCPLinker��OnIoAccept
	pAccept->OnIOCPAccept();	
	return;

errHandle:
	CloseLinker(pAccept->GetID(),LinkerCloseReason::eCloseFromProgramError,FALSE);
	return;
}
