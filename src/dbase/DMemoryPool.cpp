#include "stdafx.h"
#include "DMemoryPool.h"
#include "DAppLog.h"

DMemoryPool::DMemoryPool()
{
	m_VecPool.clear();
	m_pFreeChunkHead = m_pFreeChunkTail = nullptr;
	m_mapChunkUsed.clear();
	m_PoolChunk.Clear();

	m_ullIncSize = 0;
}

DMemoryPool::~DMemoryPool()
{
	Clear();
}

DMemoryPool* DMemoryPool::Instance()
{
	static DMemoryPool s_inst;
	return &s_inst;
}

BOOL DMemoryPool::StartPool(ULONG64 ullIncSize)
{
	DAutoLock autolock(m_Locker);

	Clear();
	
	if(ullIncSize > MEMP_MAXINCSIZE)
	{
		DAppLog::Instance()->Info(FALSE,"DMemoryPool::StartPool.����,�����õ�incsize����384M,ǿ��תΪ384M");
		ullIncSize = MEMP_MAXINCSIZE;
	}
	else if(ullIncSize < MEMP_MININCSIZE)
	{
		DAppLog::Instance()->Info(FALSE,"DMemoryPool::StartPool.����,�����õ�incsizeС��1024�ֽ�,ǿ��תΪ1024�ֽ�");
		ullIncSize = MEMP_MININCSIZE;
	}
	AlignNumForMinSize(ullIncSize);

	m_ullIncSize = ullIncSize;
	m_PoolChunk.StartPool();
	
	return AddMemoPool();
}

char* DMemoryPool::GetMemory(ULONG64 ullSize)
{
	AlignNumForMinSize(ullSize);
	if(ullSize >= m_ullIncSize)
	{
		DAppLog::Instance()->Info(FALSE,"DMemoryPool::GetMemory.���볬�����������ߴ���ڴ�");
		return nullptr;
	}
	//
	DAutoLock autolock(m_Locker);
	char* pRt = RealPopMemory(ullSize);
	if(!pRt)
	{
		//�ж��ڴ���Ƿ��Ѿ��������ߴ�
		ULONG64 ullCurSize = GetAllSize();
		if((ullCurSize + m_ullIncSize) > MEMP_MAXMEMORYSIZE)
		{
			DAppLog::Instance()->Info(TRUE,"DMemoryPool::GetMemory.�ٷ������ڴ潫�����4G");
			return pRt;
		}
		//�������ڴ��
		if(!AddMemoPool())
		{
			DAppLog::Instance()->Info(TRUE,"DMemoryPool::GetMemory.AddMemoPoolʧ��");
			return pRt;
		}
		pRt = RealPopMemory(ullSize);
		if(!pRt)
		{
			DAppLog::Instance()->Info(TRUE,"DMemoryPool::GetMemory.��ȡ�ڴ�ʧ��");
			return pRt;
		}
	}
	
	return pRt;
}

BOOL DMemoryPool::BackMemory(char** ppMem)
{
	if((*ppMem) == nullptr)
	{
		DAppLog::Instance()->Info(TRUE,"DMemoryPool::BackMemory.map�������Ĳ���Ϊnullptr");
		return FALSE;
	}

	DAutoLock autolock(m_Locker);

	ULONG64 ullLeft = (ULONG64)(*ppMem);
	ChunkMapIter itFind = m_mapChunkUsed.find(ullLeft);
	if(itFind == m_mapChunkUsed.end())
	{
		DAppLog::Instance()->Info(TRUE,"DMemoryPool::BackMemory.map���Ҳ���Ҫ���յ��ڴ�����");
		return FALSE;
	}
	stMemoryChunk* pChunk = itFind->second;
	pChunk->InitMyMemory();				//��ջ��չ������ڴ�
	InsertChunkToFree(pChunk);

	m_mapChunkUsed.erase(itFind);
	(*ppMem) = nullptr;	//��Ѵ������Ĳ�����Ϊ��
		
	return TRUE;
}

ULONG64 DMemoryPool::GetAllSize()
{
	return (m_ullIncSize * m_VecPool.size());
}

ULONG64 DMemoryPool::GetNoUseSize()
{
	ULONG64 ullSize = 0;
	stMemoryChunk* pChunkIter = m_pFreeChunkHead;
	while(pChunkIter)
	{
		ullSize += pChunkIter->ullMemoLen;
		pChunkIter = pChunkIter->pNext;
	}

	return ullSize;
}

void DMemoryPool::Clear()
{
	char* pMem = nullptr;
	for(size_t i = 0; i < m_VecPool.size(); i++)
	{
		pMem = m_VecPool[i];
		if(pMem)
		{
			delete[] pMem;
			pMem = nullptr;
		}
	}
	m_VecPool.clear();
	m_pFreeChunkHead = m_pFreeChunkTail = nullptr;
	m_mapChunkUsed.clear();
	m_PoolChunk.Clear();

	m_ullIncSize = 0;	
}

BOOL DMemoryPool::AddMemoPool()
{
	//�˺�������������StartPool��GetMemory�м�����
	char* pMem = new char[m_ullIncSize];
	if(!pMem)
	{
		DAppLog::Instance()->Info(TRUE,"DMemoryPool::AddMemoPool.new ���ڴ�ʧ��");
		return FALSE;
	}
	memset(pMem,0,m_ullIncSize);
	m_VecPool.push_back(pMem);

	stMemoryChunk* pChunk = m_PoolChunk.TakeObject();
	pChunk->Init();
	pChunk->ullMemoAddr = (ULONG64)pMem;
	pChunk->ullMemoLen = m_ullIncSize;
	
	InsertChunkToFree(pChunk);
	
	return TRUE;
}

ULONG64 DMemoryPool::AlignNumForMinSize(ULONG64& ullIn)
{
	if(ullIn < MEMP_MINCHUNKSIZE)
		ullIn = MEMP_MINCHUNKSIZE;

	int nTemp = ullIn % MEMP_MINCHUNKSIZE;
	if(nTemp != 0)
		ullIn += (MEMP_MINCHUNKSIZE - nTemp);
	
	return ullIn;
}

char* DMemoryPool::RealPopMemory(ULONG64 ullSize)
{
	//�������Ƿ���δʹ�õ��ڴ���У��Ƿ��к��ʵ�
	stMemoryChunk* pChunkFind = m_pFreeChunkHead;
	if(!pChunkFind)
		return nullptr;
	while(pChunkFind)
	{
		if(pChunkFind->ullMemoLen >= ullSize)
			break;

		pChunkFind = pChunkFind->pNext;
	}
	if(!pChunkFind)
		return nullptr;

	//������ں��ʵ��ڴ�飬�����������һ�����ð���һ���ڴ��ȫ���õ�����һ��������һ����
	char* pRt = (char*)(pChunkFind->ullMemoAddr);	
	if(pChunkFind->ullMemoLen > ullSize)
	{
		stMemoryChunk* pNewChunk = m_PoolChunk.TakeObject();
		pNewChunk->Init();
		pNewChunk->ullMemoAddr = (pChunkFind->ullMemoAddr + ullSize);
		pNewChunk->ullMemoLen = (pChunkFind->ullMemoLen - ullSize);
		pChunkFind->ullMemoLen = ullSize;
		//��ɾ������ӣ�����������������ڴ��������жϴ���
		RemoveFreeChunk(pChunkFind);
		m_mapChunkUsed.insert(make_pair(pChunkFind->ullMemoAddr,pChunkFind));
		InsertChunkToFree(pNewChunk);
	}
	else
	{
		RemoveFreeChunk(pChunkFind);
		m_mapChunkUsed.insert(make_pair(pChunkFind->ullMemoAddr,pChunkFind));
	}	
	
	return pRt;
}
/*
1�����ڴ����뵽�б���
2�����������ڴ��ϲ����ж��²�����ڴ���ǰһ�κͺ�һ��
*/
void DMemoryPool::InsertChunkToFree(stMemoryChunk* pInsertChunk)
{
	stMemoryChunk *pChunkIter = nullptr, *pChunkNext = nullptr, *pChunkPre = nullptr;

	if(!m_pFreeChunkHead)	//����������
	{
		(pInsertChunk->pPre) = (pInsertChunk->pNext) = nullptr;
		m_pFreeChunkHead = m_pFreeChunkTail = pInsertChunk;
	}	
	else if((pInsertChunk->ullMemoAddr) < (m_pFreeChunkHead->ullMemoAddr))		//������ڴ���ַ��HEAD��ǰ
	{
		m_pFreeChunkHead->pPre = pInsertChunk;
		pInsertChunk->pNext = m_pFreeChunkHead;
		pInsertChunk->pPre = nullptr;		
		m_pFreeChunkHead = pInsertChunk;
	}	
	else if((pInsertChunk->ullMemoAddr) > (m_pFreeChunkTail->ullMemoAddr))		//������ڴ���ַ��Tail����
	{
		m_pFreeChunkTail->pNext = pInsertChunk;
		pInsertChunk->pPre = m_pFreeChunkTail;
		pInsertChunk->pNext = nullptr;		
		m_pFreeChunkTail = pInsertChunk;
	}
	else	//��ѯ�����ڴ��ַ�����������ʵ�λ��
	{		
		pChunkIter = m_pFreeChunkHead;
		while(pChunkIter)
		{
			pChunkNext = pChunkIter->pNext;
			if(!pChunkNext)		//�������������������´��벻�ᵽ������
			{
				DAppLog::Instance()->Info(TRUE,"DMemoryPool::InsertChunkToFree.�����쳣");
				break;
			}
			if(((pChunkIter->ullMemoAddr) < (pInsertChunk->ullMemoAddr)) && ((pInsertChunk->ullMemoAddr) < (pChunkNext->ullMemoAddr)))
			{
				pChunkIter->pNext = pInsertChunk;
				pInsertChunk->pPre = pChunkIter;
				pInsertChunk->pNext = pChunkNext;
				pChunkNext->pPre = pInsertChunk;
				break;
			}
			pChunkIter = pChunkNext;
		}
	}
	
	//���²����Ԫ��ǰ���Ƿ��������ڴ�����ж�
	pChunkPre = pInsertChunk->pPre;
	pChunkNext = pInsertChunk->pNext;
	if(pChunkNext)
	{
		if((pInsertChunk->ullMemoAddr + pInsertChunk->ullMemoLen) == (pChunkNext->ullMemoAddr))
		{
			pInsertChunk->ullMemoLen += pChunkNext->ullMemoLen;
			pInsertChunk->pNext = pChunkNext->pNext;
			if(pInsertChunk->pNext)
				pInsertChunk->pNext->pPre = pInsertChunk;
			else
				m_pFreeChunkTail = pInsertChunk;

			m_PoolChunk.BackObject(pChunkNext);
		}
	}
	if(pChunkPre)
	{
		if((pChunkPre->ullMemoAddr + pChunkPre->ullMemoLen) == (pInsertChunk->ullMemoAddr))
		{
			pChunkPre->ullMemoLen += pInsertChunk->ullMemoLen;
			pChunkPre->pNext = pInsertChunk->pNext;
			if(pChunkPre->pNext)
				pChunkPre->pNext->pPre = pChunkPre;
			else
				m_pFreeChunkTail = pChunkPre;

			m_PoolChunk.BackObject(pInsertChunk);
		}
	}
}

void DMemoryPool::RemoveFreeChunk(stMemoryChunk* pRemoveChunk)
{
	stMemoryChunk* pPreChunk = pRemoveChunk->pPre;
	stMemoryChunk* pNextChunk = pRemoveChunk->pNext;

	if((!pPreChunk) && (!pNextChunk))	//�����оʹ�һ����Ա
		m_pFreeChunkHead = m_pFreeChunkTail = nullptr;
	else if(!pPreChunk)					//������ͷ
	{
		pNextChunk->pPre = nullptr;
		m_pFreeChunkHead = pNextChunk;
	}
	else if(!pNextChunk)			//����β
	{
		pPreChunk->pNext = nullptr;
		m_pFreeChunkTail = pPreChunk;
	}
	else
	{
		pPreChunk->pNext = pNextChunk;
		pNextChunk->pPre = pPreChunk;
	}

	//
	pRemoveChunk->pPre = pRemoveChunk->pNext = nullptr;		
}
