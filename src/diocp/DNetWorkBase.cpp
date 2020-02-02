#include "stdafx.h"
#include "DBuf.h"
#include "DAppThread.h"
#include "DIOCPServer.h"
#include "DClientConn.h"
#include "DNetObjectPool.h"
#include "DNetWorkBase.h"

DNetWorkBase::DNetWorkBase()
{
	CleanAllServerClient();
}

DNetWorkBase::~DNetWorkBase()
{
	CleanAllServerClient();
}

DNetWorkBase* DNetWorkBase::Instance()
{
	static DNetWorkBase s_inst;

	return &s_inst;
}

void DNetWorkBase::CleanAllServerClient()
{
	//
	for (IOCPServerMapIter itserver = m_mapIOCPServer.begin(); itserver != m_mapIOCPServer.end(); ++itserver)
	{
		DIOCPServer* pServer = itserver->second;
		pServer->CloseServer();
		delete pServer;
		pServer = nullptr;
	}
	m_mapIOCPServer.clear();
	//
	for (ClientConnMapIter itConn = m_mapClientConnMap.begin(); itConn != m_mapClientConnMap.end(); ++itConn)
	{
		DClientConn* pClientConn = itConn->second;
		pClientConn->Close();
		DNetObjectPool::Instance()->BackClientConn(pClientConn);
	}
	m_mapClientConnMap.clear();
}

BOOL DNetWorkBase::SocketStart()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		::MessageBoxA(nullptr, "ÍøÂç³õÊ¼»¯Ê§°Ü:WSAStartup", "ÍøÂç´íÎó", MB_OK);
		return FALSE;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		::MessageBoxA(nullptr, "ÍøÂç³õÊ¼»¯Ê§°Ü:°æ±¾´íÎó", "ÍøÂç´íÎó", MB_OK);
		WSACleanup();
		return FALSE;
	}

	return TRUE;
}

BOOL DNetWorkBase::SocketCleanup()
{
	::WSACleanup();
	return TRUE;
}

void DNetWorkBase::InitNetObjectPool(int nClientConnIncSize, int nServerLinkerIncSize, int nLinkerSendIncSize)
{
	DNetObjectPool::Instance()->Init(nClientConnIncSize, nServerLinkerIncSize, nLinkerSendIncSize);
}

void DNetWorkBase::CleanUpNetObjectPool()
{
	DNetObjectPool::Instance()->CleanUp();
}

USHORT DNetWorkBase::CreateIOCPServer(USHORT ushPort, DAppThread* pAppThread)
{
	DIOCPServer* pNewServer = new DIOCPServer();
	if (pNewServer == nullptr)
		return 0;
	if (pNewServer->CreateServer(ushPort, pAppThread) == FALSE)
	{
		delete pNewServer;
		pNewServer = nullptr;
		return 0;
	}

	m_mapIOCPServer.insert(make_pair(ushPort, pNewServer));
	return ushPort;
}

void DNetWorkBase::CloseIOCPServer(USHORT ushPort)
{
	IOCPServerMapIter itFind = m_mapIOCPServer.find(ushPort);
	if (itFind != m_mapIOCPServer.end())
	{
		DIOCPServer* pServer = itFind->second;
		pServer->CloseServer();
		delete pServer;
		pServer = nullptr;
		m_mapIOCPServer.erase(itFind);
	}
}

BOOL DNetWorkBase::GetALinkerAddr(USHORT ushPort, DWORD dwLinkerID, stAddrInfo& addrInfo)
{
	IOCPServerMapIter itFind = m_mapIOCPServer.find(ushPort);
	if (itFind == m_mapIOCPServer.end())
		return FALSE;

	DIOCPServer* pServer = itFind->second;

	return pServer->GetLinkerAddr(dwLinkerID, addrInfo);
}

void DNetWorkBase::CloseALinker(USHORT ushPort, DWORD dwID, BYTE byReason, BOOL bTellApp)
{
	IOCPServerMapIter itFind = m_mapIOCPServer.find(ushPort);
	if (itFind != m_mapIOCPServer.end())
	{
		DIOCPServer* pServer = itFind->second;
		pServer->CloseLinker(dwID, byReason, bTellApp);
	}
}

void DNetWorkBase::SendNetMsg(USHORT ushPort, DWORD dwID, const char* szMsg, int nMsgLen)
{
	IOCPServerMapIter itFind = m_mapIOCPServer.find(ushPort);
	if (itFind != m_mapIOCPServer.end())
	{
		DIOCPServer* pServer = itFind->second;
		pServer->SendNetMsg(dwID, szMsg, nMsgLen);
	}
}

DWORD DNetWorkBase::CreateClientConn(const char* szFarAddr, USHORT ushPort, DAppThread* pAppThread)
{
	DClientConn* pClientConn = DNetObjectPool::Instance()->TakeClientConn();
	if (pClientConn == nullptr)
		return 0;

	if (pClientConn->Connect(szFarAddr, ushPort, pAppThread) == FALSE)
	{
		DNetObjectPool::Instance()->BackClientConn(pClientConn);
		return 0;
	}

	m_mapClientConnMap.insert(make_pair(pClientConn->GetID(), pClientConn));
	return pClientConn->GetID();
}

void DNetWorkBase::CloseClientConn(DWORD dwID)
{
	ClientConnMapIter itFind = m_mapClientConnMap.find(dwID);
	if (itFind != m_mapClientConnMap.end())
	{
		DClientConn* pClientConn = itFind->second;
		pClientConn->Close();
		DNetObjectPool::Instance()->BackClientConn(pClientConn);

		m_mapClientConnMap.erase(itFind);
	}
}

int DNetWorkBase::SendConnMsg(DWORD dwID, DBuf* pDBuf)
{
	ClientConnMapIter itFind = m_mapClientConnMap.find(dwID);
	if (itFind != m_mapClientConnMap.end())
	{
		DClientConn* pClientConn = itFind->second;
		return pClientConn->Send(pDBuf);
	}

	return 0;
}

int DNetWorkBase::SendConnMsg(DWORD dwID, const char* pSendMsg, int nMsgLen)
{
	ClientConnMapIter itFind = m_mapClientConnMap.find(dwID);
	if (itFind != m_mapClientConnMap.end())
	{
		DClientConn* pClientConn = itFind->second;
		return pClientConn->Send(pSendMsg, nMsgLen);
	}

	return 0;
}
