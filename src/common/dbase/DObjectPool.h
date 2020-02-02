/***********************************************************************************
Name:DObjectPool
comment:一个对象池的模版类，能自动增长。
***********************************************************************************/

#ifndef _DOBJECTPOOL_H_
#define _DOBJECTPOOL_H_

#pragma once

#include "stdafx.h"
#include "DLockerBuff.h"
#include "DAppLog.h"

template <typename T>
class DObjectPool
{
private:
	typedef vector<T*> VecPointerT;

public:
	DObjectPool();
	~DObjectPool();

public:
	BOOL StartPool(int nIncSize = 1024);
	T* TakeObject();				//申请一个对象
	void BackObject(T* pObj);			//归还一个对象
	int GetAllSize();
	int GetNoUseSize();
	void Clear();

private:
	BOOL AddPool();
	
private:
	int m_nIncSize;		//每次增加的数量，即每个对象池的大小
	
	VecPointerT m_VecPool;	//一个二维数组，每次当原来申请的对象池不够用时，会自动新申请一块
	VecPointerT m_VecUse;		//实际可用的数组，一维数组
	DLockerBuff m_Locker;
};

template <typename T>
DObjectPool<T>::DObjectPool()
{
	m_VecPool.clear();
	m_VecUse.clear();

	m_nIncSize = 0;
}

template <typename T>
DObjectPool<T>::~DObjectPool()
{
	Clear();
}

template <typename T>
BOOL DObjectPool<T>::StartPool(int nIncSize)
{
	DAutoLock autolock(m_Locker);

	Clear();
	if(nIncSize < 1)
		return FALSE;

	m_nIncSize = nIncSize;
	
	return AddPool();
}

template <typename T>
T* DObjectPool<T>::TakeObject()
{
	DAutoLock autolock(m_Locker);

	T* pRt = nullptr;

	if(m_VecUse.size() == 0)
	{
		if((AddPool() == FALSE) || (m_VecUse.size() == 0))
			return pRt;
	}
	
	pRt = m_VecUse.back();
	m_VecUse.pop_back();

	return pRt;
}

template <typename T>
void DObjectPool<T>::BackObject(T* pObj)
{
	DAutoLock autolock(m_Locker);

	m_VecUse.push_back(pObj);
}

template <typename T>
int DObjectPool<T>::GetAllSize()
{
	return m_nIncSize * m_VecPool.size();
}

template <typename T>
int DObjectPool<T>::GetNoUseSize()
{
	return m_VecUse.size();
}

template <typename T>
void DObjectPool<T>::Clear()
{
	T* pT = nullptr;
	for(int i = 0; i < m_VecPool.size(); i++)
	{
		pT = m_VecPool[i];
		if(pT)
		{
			delete[] pT;		//在AddPool中申请
			pT = nullptr;
		}
	}
	m_VecPool.clear();

	m_VecUse.clear();

	m_nIncSize = 0;
}

template <typename T>
BOOL DObjectPool<T>::AddPool()
{
	//此函数不加锁，因为它的两个调用函数SetIncSize和GetObject都已经加锁
	T* pnewPool = new T[m_nIncSize];	//在Clear中释放
	if(!pnewPool)
	{
		DAppLog::Instance()->Info(TRUE,"DObjectPool<T>::AddPool.new 新对象池失败");
		return FALSE;
	}
	m_VecPool.push_back(pnewPool);

	for(int i = 0; i < m_nIncSize; i++)
		m_VecUse.push_back(&(pnewPool[i]));

	return TRUE;
}

#endif