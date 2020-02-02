#ifndef _DLOCKERBUFF_H_
#define _DLOCKERBUFF_H_

#pragma once

class DLockerBuff
{
	CRITICAL_SECTION m_lockBuff ;
public:
	DLockerBuff()
	{ 
		InitializeCriticalSection(&m_lockBuff); 
	};

	~DLockerBuff()
	{
		DeleteCriticalSection(&m_lockBuff); 
	};

	void Lock()
	{ 
		EnterCriticalSection(&m_lockBuff); 
	};

	void Unlock()
	{ 
		LeaveCriticalSection(&m_lockBuff);
	};
};

class DAutoLock
{
public:
	DAutoLock(DLockerBuff& rLock)
	{
		m_pLock = &rLock;
		m_pLock->Lock();
	}
	~DAutoLock()
	{
		m_pLock->Unlock();
	}
private:
	DAutoLock();
	DLockerBuff* m_pLock;
};

#endif