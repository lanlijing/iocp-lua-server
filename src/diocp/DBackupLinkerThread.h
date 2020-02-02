#ifndef _DBACKUPLINKERTHREAD_H_
#define _DBACKUPLINKERTHREAD_H_

#pragma once

#include "DLockerBuff.h"

class DIOCPLinker;
class DBackupLinkerThread
{
public:
	typedef vector<DIOCPLinker*> VecLinkerPtr;
public:
	DBackupLinkerThread();
	~DBackupLinkerThread();

	BOOL Start();
	void Stop();
	static DWORD WINAPI MyThreadProcess(void* pParam);
	DWORD RealThreadProc();

public:
	void PushBackedLinker(DIOCPLinker* pLinker);
	void BackAllLinker();

private:
	DWORD m_dwThreadID;
	HANDLE m_hThread;
	BOOL m_bRunFlag;

	std::vector<DIOCPLinker*>	m_vecLinker;
	DLockerBuff m_LockRecv;
};

#endif