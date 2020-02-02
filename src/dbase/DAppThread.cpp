#include "stdafx.h"
#include "DAppThread.h"
#include "DBuf.h"

DAppThread::DAppThread()
{
	m_dwThreadID = 0;
	m_hThread = nullptr;
	m_bRunFlag = FALSE;

	m_nFrameMinTime = 5;
	m_nFrameWarnTime = 1000;
	m_nFrameAbortTime = 30000;
}

DAppThread::~DAppThread()
{
	Stop();
}

BOOL DAppThread::Start()
{
	if(!m_hThread)
	{
		m_hThread = ::CreateThread(nullptr, 0, DAppThread::MyThreadProcess, this, 0, &m_dwThreadID);
		if(!m_hThread)
			return FALSE;

		m_bRunFlag = TRUE;

		Sleep(25);
	}

	return TRUE;
}

void DAppThread::Stop()
{
	if(m_hThread)
	{
		m_bRunFlag = FALSE;
		::WaitForSingleObject(m_hThread, 2500);
		::TerminateThread(m_hThread,0);
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
		m_dwThreadID = 0;

		//删除队列，退回对象
		for(int i = 0; i < m_vecMsg.size(); i++)
		{
			DBuf* pThreadMsg = m_vecMsg[i];
			DBuf::BackDBuf(pThreadMsg);
		}
		m_vecMsg.clear();		
	}
}

DWORD WINAPI DAppThread::MyThreadProcess(void* pParam)
{
	DAppThread* pThis = (DAppThread*)pParam;
	
	return pThis->RealThreadProc();
}

DWORD DAppThread::RealThreadProc()
{
	SetThreadLocale(GetUserDefaultLCID());		//避免在英文系统下出现乱码,因为只在本线程有用，所有多个线程要增加

	ULONG64 ullNow = GetTickCount64();
	ULONG64 ullLast = ullNow;	
	int nTick = 0,nVecSize = 0;
	VecThreadMsgPtr tempVecMsg;
	while(m_bRunFlag)
	{	
		tempVecMsg.clear();
		//先拷贝消息队列出来
		{
			DAutoLock autolock(m_LockMsgVec);
			for(int i = 0; i < m_vecMsg.size(); i++)
			{
				tempVecMsg.push_back(m_vecMsg[i]);
			}
			m_vecMsg.clear();
		}
		nVecSize = tempVecMsg.size();
					
		//处理消息队列
		for(VecThreadMsgPtr::iterator iter = tempVecMsg.begin(); iter != tempVecMsg.end();)
		{
			ullNow = GetTickCount64();
			nTick = (ullNow - ullLast);
			if(nTick > m_nFrameAbortTime)
			{
				DAppLog::Instance()->Info(TRUE,"DAppThread主循环超期严重.放弃部分消息:帧耗时:%d.消息队列大小:%d.",nTick,nVecSize);
				break;
			}
			//
			DBuf* pThreadMsg = *iter;
			ProcessMsg(pThreadMsg); //子类需要继续
			DBuf::BackDBuf(pThreadMsg);
			iter = tempVecMsg.erase(iter);
		}
		//处理逻辑
		ProcessLogic();		//子类重载此函数，进行其它逻辑处理

		//
		ullNow = GetTickCount64();
		nTick = (ullNow - ullLast);
		ullLast = ullNow;
		if(nTick < m_nFrameMinTime)
		{
			Sleep(m_nFrameMinTime - nTick);
		}
		else if(nTick > m_nFrameWarnTime)
		{
			DAppLog::Instance()->Info(TRUE,"DAppThread主循环线程超期:帧耗时:%d.消息队列大小:%d",nTick,nVecSize);
		}
	}
	
	return 0;
}

void DAppThread::SetFrameTimePara(int nFrameMinTime,int nFrameWarnTime,int nFrameAbortTime)
{
	m_nFrameMinTime = nFrameMinTime;
	m_nFrameWarnTime = nFrameWarnTime;
	m_nFrameAbortTime = nFrameAbortTime;
}

void DAppThread::PushMsgToList(DBuf* pMsgBuf,BOOL bInHead)
{
	DAutoLock autolock(m_LockMsgVec);
	if(bInHead)
		m_vecMsg.insert(m_vecMsg.begin(),pMsgBuf);
	else
		m_vecMsg.push_back(pMsgBuf);
}

int DAppThread::GetMsgListNum()
{
	DAutoLock autolock(m_LockMsgVec);
	return m_vecMsg.size();
}