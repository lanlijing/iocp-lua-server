/***********************************************************************************
Name:DObjectPool
comment:һ������ص�ģ���࣬���Զ�������
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
	T* TakeObject();				//����һ������
	void BackObject(T* pObj);			//�黹һ������
	int GetAllSize();
	int GetNoUseSize();
	void Clear();

private:
	BOOL AddPool();
	
private:
	int m_nIncSize;		//ÿ�����ӵ���������ÿ������صĴ�С
	
	VecPointerT m_VecPool;	//һ����ά���飬ÿ�ε�ԭ������Ķ���ز�����ʱ�����Զ�������һ��
	VecPointerT m_VecUse;		//ʵ�ʿ��õ����飬һά����
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
			delete[] pT;		//��AddPool������
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
	//�˺�������������Ϊ�����������ú���SetIncSize��GetObject���Ѿ�����
	T* pnewPool = new T[m_nIncSize];	//��Clear���ͷ�
	if(!pnewPool)
	{
		DAppLog::Instance()->Info(TRUE,"DObjectPool<T>::AddPool.new �¶����ʧ��");
		return FALSE;
	}
	m_VecPool.push_back(pnewPool);

	for(int i = 0; i < m_nIncSize; i++)
		m_VecUse.push_back(&(pnewPool[i]));

	return TRUE;
}

#endif