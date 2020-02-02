#include "stdafx.h"
#include "DNetObjectPool.h"
#include "DMemoryPool.h"
#include "DAppLog.h"

DNetObjectPool::DNetObjectPool()
{
}

DNetObjectPool::~DNetObjectPool()
{
	CleanUp();
}

DNetObjectPool* DNetObjectPool::Instance()
{
	static DNetObjectPool s_inst;
	
	return &s_inst;
}

void DNetObjectPool::Init(int nClientConnIncSize,int nServerLinkerIncSize,int nLinkerSendIncSize)
{
	CleanUp();

	m_ClientConnPool.StartPool(nClientConnIncSize);
	m_ServerLinkerPool.StartPool(nServerLinkerIncSize);
	m_LinkerSendPool.StartPool(nLinkerSendIncSize);
}

void DNetObjectPool::CleanUp()
{
	m_ClientConnPool.Clear();
	m_ServerLinkerPool.Clear();	
	m_LinkerSendPool.Clear();
}

DClientConn* DNetObjectPool::TakeClientConn()
{
	DClientConn* pRet = m_ClientConnPool.TakeObject();
	if(!pRet)
		DAppLog::Instance()->Info(TRUE,"DNetObjectPool::TakeClientConn.发生错误,取出来的对象为空");

	return pRet;
}

void DNetObjectPool::BackClientConn(DClientConn* pClient)
{
	m_ClientConnPool.BackObject(pClient);
}

DIOCPLinker* DNetObjectPool::TakeServerLinker()
{
	DIOCPLinker* pRet = m_ServerLinkerPool.TakeObject();
	if(!pRet)
		DAppLog::Instance()->Info(TRUE,"DNetObjectPool::TakeServerLinker.发生错误,取出来对象为空");

	return pRet;
}

void DNetObjectPool::BackServerLinker(DIOCPLinker* pLinker)
{
	pLinker->Final();
	m_ServerLinkerPool.BackObject(pLinker);
}

stLinkerSend* DNetObjectPool::TakeLinkerSend(const char* szMsg,int nMsgLen)
{
	stLinkerSend* pRetSend = m_LinkerSendPool.TakeObject();
	if(pRetSend)
	{
		pRetSend->m_nMsgLen = 0;
		pRetSend->m_szMsg = nullptr;
		if(szMsg)
		{
			pRetSend->m_nMsgLen = nMsgLen;
			pRetSend->m_szMsg = DMemoryPool::Instance()->GetMemory(pRetSend->m_nMsgLen);
			memcpy(pRetSend->m_szMsg,szMsg,nMsgLen);
		}
	}
	else
		DAppLog::Instance()->Info(TRUE,"DNetObjectPool::TakeLinkerSend.发生错误,取出来对象为空");

	return pRetSend;
}

void DNetObjectPool::BackLinkerSend(stLinkerSend* pLinkerSend)
{
	//首先把内存归还到内存池
	if(pLinkerSend->m_szMsg)
		DMemoryPool::Instance()->BackMemory(&(pLinkerSend->m_szMsg));

	m_LinkerSendPool.BackObject(pLinkerSend);
}
