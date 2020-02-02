#include "stdafx.h"
#include "DBackupLinkerThread.h"
#include "DNetObjectPool.h"
#include "DAppLog.h"

DBackupLinkerThread::DBackupLinkerThread()
{
	m_dwThreadID = 0;
	m_hThread = nullptr;
	m_bRunFlag = FALSE;
}

DBackupLinkerThread::~DBackupLinkerThread()
{
	Stop();
}

BOOL DBackupLinkerThread::Start()
{
	if(!m_hThread)
	{
		m_hThread = ::CreateThread(nullptr, 0, DBackupLinkerThread::MyThreadProcess, this, 0, &m_dwThreadID);
		if(!m_hThread)
			return FALSE;

		m_bRunFlag = TRUE;

		DAppLog::Instance()->Info(FALSE,"DBackupLinkerThread::Start.Linker回收线程启动");
	}

	return TRUE;
}

void DBackupLinkerThread::Stop()
{
	if(m_hThread)
	{
		m_bRunFlag = FALSE;
		::WaitForSingleObject(m_hThread, THREADWAITOBJECTTIME);
		::TerminateThread(m_hThread,0);
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
		m_dwThreadID = 0;

		//删除队列，退回对象
		BackAllLinker();
		//
		DAppLog::Instance()->Info(FALSE,"Linker回收线程关闭");
	}
}

DWORD WINAPI DBackupLinkerThread::MyThreadProcess(void* pParam)
{
	DBackupLinkerThread* pThis = (DBackupLinkerThread*)pParam;

	return pThis->RealThreadProc();
}

DWORD DBackupLinkerThread::RealThreadProc()
{
	const UINT c_SleepTick = 3000;			//每3秒循环查询
	ULONG64 ullNow = 0;
	VecLinkerPtr VecProcess;
	VecProcess.clear();		

	while(m_bRunFlag)
	{
		//先拷贝到处理队列		
		{
			DAutoLock autolock(m_LockRecv);
			for(int i = 0; i < m_vecLinker.size(); i++)
			{
				VecProcess.push_back(m_vecLinker[i]);
			}
			m_vecLinker.clear();
		}

		ullNow = GetTickCount64();
		for(VecLinkerPtr::iterator itVec = VecProcess.begin(); itVec != VecProcess.end();)
		{
			DIOCPLinker* pLinker = *itVec;
			if((ullNow - pLinker->GetDestroyTick()) > LINKERDESTROYWAITTIME)		
			{
				DAppLog::Instance()->Info(FALSE,"真实回收一个连接%d",pLinker->GetID());	

				DNetObjectPool::Instance()->BackServerLinker(pLinker);
				itVec = VecProcess.erase(itVec);
			}
			else
				++itVec;
		}

		Sleep(c_SleepTick);
	}

	return 0;
}

void DBackupLinkerThread::PushBackedLinker(DIOCPLinker* pLinker)
{
	DAutoLock autolock(m_LockRecv);
	m_vecLinker.push_back(pLinker);
}

void DBackupLinkerThread::BackAllLinker()
{
	VecLinkerPtr::iterator iter;
	DIOCPLinker* pLinker = nullptr;
	for(iter = m_vecLinker.begin(); iter != m_vecLinker.end(); ++iter)
	{
		pLinker= *iter;
		DNetObjectPool::Instance()->BackServerLinker(pLinker);
	}
	m_vecLinker.clear();
}