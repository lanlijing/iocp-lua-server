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
	//判断是否有配置对象
	if(!pAppThread)
	{
		::MessageBoxA(nullptr,"启动IOCP服务器参数缺失","DIOCPServer",MB_OK);
		return FALSE;
	}
	m_pAppThread = pAppThread;
	m_nListenPort = nPort;

	//关闭服务
	CloseServer();	

	//创建实际回收ServerLinker线程
	m_pBackLinkerThread = new DBackupLinkerThread();
	m_pBackLinkerThread->Start();
		
	//创建IOCP相关
	BOOL bRet = CreateIOCP();
	if(!bRet)
		return FALSE;
	
	//创建监听SOCKET相关
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

	//向IOCP线程发出退出消息
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.向IOCP线程发出退出消息");	
	for(int i = 0; i < m_nIOCPWorkThreadNum; i++)
		PostQueuedCompletionStatus(m_hIOCP, 0, 0, nullptr);
		
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.等待关闭IOCP线程");
	//线程清空	
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
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseServer.关闭监听线程");
	::WaitForSingleObject(m_hLinstenThread,THREADWAITOBJECTTIME);
	::TerminateThread(m_hLinstenThread,0);
	CloseHandle(m_hLinstenThread);
	m_hLinstenThread = nullptr;

	::CloseHandle(m_hAcceptEvent);
	m_hAcceptEvent = nullptr;
			
	//关闭所有连接
	CloseAllLinker();
	if(m_pBackLinkerThread)
	{
		m_pBackLinkerThread->Stop();
		delete m_pBackLinkerThread;
		m_pBackLinkerThread = nullptr;
	}
	//关闭监听SOCKET
	::closesocket(m_ListenSocket);
	m_ListenSocket = INVALID_SOCKET;
	//销毁完成端口
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
			pDLinker->CloseSocket(byReason,bTellApp);				//关闭SOCKET,ACCEPT前出错，不需要通知应用层
			pDLinker->SetDestroyTick(GetTickCount64());	//设置销毁计时
			m_mapLinker.erase(itFind);		//从使用队列中删除

			stAddrInfo addrInfo = pDLinker->GetAddrInfo();
			DAppLog::Instance()->Info(FALSE, "DIOCPServer::CloseLinker.IP:%s.ID:%d的连接因%d原因关闭", addrInfo.m_strIP.c_str(), dwID, byReason);

			m_pBackLinkerThread->PushBackedLinker(pDLinker);
		}
		else	//关闭一个不在队列中的消息。这种情况发生的可能性有：网络层已经关闭了该连接，但应用层或IOCP线程还发来关闭的消息。
		{
			DAppLog::Instance()->Info(FALSE,"DIOCPServer::CloseLinker.企图关闭一个不在队列中的连接:ID:%d,来源:%d",dwID,byReason);
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
	else	//这种情况一般出在网络引擎中来自客户端的连接已经断开,但应用层尚未清除与该连接关联的对象
	{
	//	DAppLog::Instance()->Info(FALSE,"DIOCPServer::SendNetMsg.要求不在队列中的Linker发送消息.可能是底层连接已经断开,应用层关联对象尚未设置:%s %d",socketInfo.m_szIP,socketInfo.m_nInneID);
	}
}

BOOL DIOCPServer::CreateIOCP()
{
	//创建IOCP及相关线程
	char szErr[512] = {0};
	DWORD dwErr = 0;
	DWORD dwThreadID = 0;

	m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (nullptr == m_hIOCP)
	{
		::MessageBoxA(nullptr,"创建IOCP出错","DIOCPServer",MB_OK);
		return FALSE;
	}
	//得到IOCP线程数
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
			_snprintf(szErr,sizeof(szErr) - 1,"创建iocp线程出错%d",dwErr);
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
	//创建监听SOCKET及监听线程
	DWORD dwThreadID = 0;

	m_ListenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_ListenSocket)
	{
		dwErr = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"创建监听SOCKET出错%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//获取acceptex函数指针
	GUID GUIDAcceptEx = WSAID_ACCEPTEX;
	DWORD dwResult = 0;
	dwErr = ::WSAIoctl(m_ListenSocket,SIO_GET_EXTENSION_FUNCTION_POINTER,&GUIDAcceptEx,sizeof(GUIDAcceptEx),&m_lpAcceptEx,sizeof(m_lpAcceptEx),
		&dwResult,nullptr,nullptr);
	if(dwErr == SOCKET_ERROR)
	{ 
		_snprintf(szErr,sizeof(szErr) - 1,"获取AcceptEx函数指针出错%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//将监听SOCKET和完成端口关连起来
	HANDLE hrc = ::CreateIoCompletionPort((HANDLE)m_ListenSocket,m_hIOCP,0,0);
	if(hrc == nullptr)
	{
		dwErr = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"监听SOCKET绑定完成端口出错%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//将监听套接字m_ListenSocket绑定到本地IP地址，并置于监听模式。
	SOCKADDR_IN myAddr;
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	myAddr.sin_port = htons(m_nListenPort);
	dwErr = bind(m_ListenSocket,(sockaddr*)(&myAddr),sizeof(SOCKADDR_IN));
	if(dwErr == SOCKET_ERROR)
	{
		_snprintf(szErr,sizeof(szErr) - 1,"监听socket绑定端口出错%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	dwErr = listen(m_ListenSocket, 20);
	if(dwErr == SOCKET_ERROR)
	{
		_snprintf(szErr,sizeof(szErr) - 1,"listen出错%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}	
	//创建ListenEvent
	m_hAcceptEvent = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
	if (nullptr == m_hAcceptEvent)
	{
		dwErr = ::GetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"创建Accept事件%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
	//建立listenthread
	m_hLinstenThread  = ::CreateThread(nullptr, 0, DIOCPServer::ListenThreadProc,this, 0, &dwThreadID);
	if(m_hLinstenThread == nullptr)
	{
		dwErr = ::WSAGetLastError();
		_snprintf(szErr,sizeof(szErr) - 1,"创建监听线程出错%d",dwErr);
		::MessageBoxA(nullptr,szErr,"DIOCPServer",MB_OK);
		return FALSE;
	}
		
	return TRUE;
}

DWORD WINAPI DIOCPServer::ListenThreadProc(void* pParam)
{
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::ListenThreadProc启动");

	DIOCPServer* pThis = (DIOCPServer*)pParam;	
	::SetEvent(pThis->m_hAcceptEvent);
	while(TRUE)
	{
		::WaitForSingleObject(pThis->m_hAcceptEvent,INFINITE);	//如果发送完的ACCEPT还没有使用完，就一直等待

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

	DAppLog::Instance()->Info(FALSE,"DIOCPServer::ListenThreadProc关闭");

	return 0;
}

DWORD WINAPI DIOCPServer::IOCPWorkThreadProc(void* pParam)
{
	DAppLog::Instance()->Info(FALSE,"DIOCPServer::IOCPWorkThreadProc.启动");

	DIOCPServer* pThis = (DIOCPServer*)pParam;
	DIOCPLinker* pDLinker = nullptr;
	stIoData* pIoData = nullptr;
	LPOVERLAPPED pOverlapped;
	DWORD dwRecvBytes = 0;
	DWORD dwErrorCode = 0;
	
	while(TRUE)	
	{
		BOOL bRet = ::GetQueuedCompletionStatus(pThis->m_hIOCP,&dwRecvBytes,(PULONG_PTR)(&pDLinker),&pOverlapped,INFINITE);	
		if(bRet)	//首先判断是否需要退出
		{
			if((dwRecvBytes == 0)  && (pDLinker == nullptr) && (pOverlapped == nullptr))	
			{
				//PostQueuedCompletionStatus发过来一个空的单句柄数据，表示线程要退出了。
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
			//不管成功失败都减1
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
			DAppLog::Instance()->Info(FALSE,"GetQueuedCompletionStatus ID:%d,错误:%d",pDLinker->GetID(),dwErrorCode);
			if((dwErrorCode == ERROR_NETNAME_DELETED/*64*/) || (dwErrorCode == ERROR_CONNECTION_ABORTED/*1236*/))
				pThis->CloseLinker(pDLinker->GetID(),LinkerCloseReason::eCloseFromClientError);
		}
		else
		{
			if(dwRecvBytes == 0)	//客户端退出
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

	DAppLog::Instance()->Info(FALSE,"DIOCPServer::IOCPWorkThreadProc.关闭");

	return 0;
}

DIOCPLinker* DIOCPServer::GetLinker()
{
	DIOCPLinker* pLinker = DNetObjectPool::Instance()->TakeServerLinker();	//在CloseLinker中 及CloseAllLinker 回收
	pLinker->InitLinker(this,m_pAppThread);		//初始化
	if(!pLinker)
	{
		DAppLog::Instance()->Info(FALSE,"DIOCPServer::GetLinker.获取新的连接对象出错");
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
			DNetObjectPool::Instance()->BackServerLinker(pDLinker);					//在GetLinker中获得
		}
		m_mapLinker.clear();
	}
}

void DIOCPServer::OnIoAccept(DIOCPLinker* pAccept)
{
	//设置新连接属性	
	char szLog[512] = {0};
	int nResult = 0;
	nResult = setsockopt(pAccept->m_socket,SOL_SOCKET,SO_UPDATE_ACCEPT_CONTEXT,(char*)&m_ListenSocket,sizeof(m_ListenSocket));
	if(SOCKET_ERROR == nResult)
	{
		_snprintf(szLog,sizeof(szLog),"DIOCPServer::OnIoAccept.setsockopt SO_UPDATE_ACCEPT_CONTEXT failed error code:%d", ::WSAGetLastError()); 
		pAccept->PrintLogInfo(szLog);
		goto errHandle;
	}	
	//关连IOCP
	if(CreateIoCompletionPort((HANDLE)pAccept->m_socket,m_hIOCP,(DWORD_PTR)pAccept,0) != m_hIOCP)
	{
		_snprintf(szLog,sizeof(szLog),"DIOCPServer::OnIoAccept.CreateIoCompletionPort failed error code:%d", ::GetLastError()); 
		pAccept->PrintLogInfo(szLog);
		goto errHandle;
	}
		
	//调用DIOCPLinker的OnIoAccept
	pAccept->OnIOCPAccept();	
	return;

errHandle:
	CloseLinker(pAccept->GetID(),LinkerCloseReason::eCloseFromProgramError,FALSE);
	return;
}
