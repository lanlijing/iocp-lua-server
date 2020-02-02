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
		DAppLog::Instance()->Info(FALSE,"DMemoryPool::StartPool.警告,所设置的incsize大于384M,强制转为384M");
		ullIncSize = MEMP_MAXINCSIZE;
	}
	else if(ullIncSize < MEMP_MININCSIZE)
	{
		DAppLog::Instance()->Info(FALSE,"DMemoryPool::StartPool.警告,所设置的incsize小于1024字节,强制转为1024字节");
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
		DAppLog::Instance()->Info(FALSE,"DMemoryPool::GetMemory.申请超过单次增长尺寸的内存");
		return nullptr;
	}
	//
	DAutoLock autolock(m_Locker);
	char* pRt = RealPopMemory(ullSize);
	if(!pRt)
	{
		//判断内存池是否已经超过最大尺寸
		ULONG64 ullCurSize = GetAllSize();
		if((ullCurSize + m_ullIncSize) > MEMP_MAXMEMORYSIZE)
		{
			DAppLog::Instance()->Info(TRUE,"DMemoryPool::GetMemory.再分配新内存将会大于4G");
			return pRt;
		}
		//分配新内存块
		if(!AddMemoPool())
		{
			DAppLog::Instance()->Info(TRUE,"DMemoryPool::GetMemory.AddMemoPool失败");
			return pRt;
		}
		pRt = RealPopMemory(ullSize);
		if(!pRt)
		{
			DAppLog::Instance()->Info(TRUE,"DMemoryPool::GetMemory.获取内存失败");
			return pRt;
		}
	}
	
	return pRt;
}

BOOL DMemoryPool::BackMemory(char** ppMem)
{
	if((*ppMem) == nullptr)
	{
		DAppLog::Instance()->Info(TRUE,"DMemoryPool::BackMemory.map传进来的参数为nullptr");
		return FALSE;
	}

	DAutoLock autolock(m_Locker);

	ULONG64 ullLeft = (ULONG64)(*ppMem);
	ChunkMapIter itFind = m_mapChunkUsed.find(ullLeft);
	if(itFind == m_mapChunkUsed.end())
	{
		DAppLog::Instance()->Info(TRUE,"DMemoryPool::BackMemory.map中找不到要回收的内存块对象");
		return FALSE;
	}
	stMemoryChunk* pChunk = itFind->second;
	pChunk->InitMyMemory();				//清空回收过来的内存
	InsertChunkToFree(pChunk);

	m_mapChunkUsed.erase(itFind);
	(*ppMem) = nullptr;	//会把传进来的参数置为空
		
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
	//此函数不加锁，在StartPool及GetMemory中加了锁
	char* pMem = new char[m_ullIncSize];
	if(!pMem)
	{
		DAppLog::Instance()->Info(TRUE,"DMemoryPool::AddMemoPool.new 新内存失败");
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
	//遍历看是否在未使用的内存块中，是否有合适的
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

	//如果存在合适的内存块，有两种情况，一是正好把这一个内存块全部用掉，另一种是用了一部分
	char* pRt = (char*)(pChunkFind->ullMemoAddr);	
	if(pChunkFind->ullMemoLen > ullSize)
	{
		stMemoryChunk* pNewChunk = m_PoolChunk.TakeObject();
		pNewChunk->Init();
		pNewChunk->ullMemoAddr = (pChunkFind->ullMemoAddr + ullSize);
		pNewChunk->ullMemoLen = (pChunkFind->ullMemoLen - ullSize);
		pChunkFind->ullMemoLen = ullSize;
		//先删除再添加，否则可能引发空闲内存连续性判断错误
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
1、把内存块插入到列表中
2、把连续的内存块合并，判断新插入的内存块的前一段和后一段
*/
void DMemoryPool::InsertChunkToFree(stMemoryChunk* pInsertChunk)
{
	stMemoryChunk *pChunkIter = nullptr, *pChunkNext = nullptr, *pChunkPre = nullptr;

	if(!m_pFreeChunkHead)	//链表还不存在
	{
		(pInsertChunk->pPre) = (pInsertChunk->pNext) = nullptr;
		m_pFreeChunkHead = m_pFreeChunkTail = pInsertChunk;
	}	
	else if((pInsertChunk->ullMemoAddr) < (m_pFreeChunkHead->ullMemoAddr))		//插入的内存块地址比HEAD还前
	{
		m_pFreeChunkHead->pPre = pInsertChunk;
		pInsertChunk->pNext = m_pFreeChunkHead;
		pInsertChunk->pPre = nullptr;		
		m_pFreeChunkHead = pInsertChunk;
	}	
	else if((pInsertChunk->ullMemoAddr) > (m_pFreeChunkTail->ullMemoAddr))		//插入的内存块地址比Tail还后
	{
		m_pFreeChunkTail->pNext = pInsertChunk;
		pInsertChunk->pPre = m_pFreeChunkTail;
		pInsertChunk->pNext = nullptr;		
		m_pFreeChunkTail = pInsertChunk;
	}
	else	//轮询根据内存地址放入链表中适当位置
	{		
		pChunkIter = m_pFreeChunkHead;
		while(pChunkIter)
		{
			pChunkNext = pChunkIter->pNext;
			if(!pChunkNext)		//到了链表最后，正常情况下代码不会到这里来
			{
				DAppLog::Instance()->Info(TRUE,"DMemoryPool::InsertChunkToFree.代码异常");
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
	
	//就新插入的元素前后是否有连续内存进行判断
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

	if((!pPreChunk) && (!pNextChunk))	//链表中就此一个成员
		m_pFreeChunkHead = m_pFreeChunkTail = nullptr;
	else if(!pPreChunk)					//是链表头
	{
		pNextChunk->pPre = nullptr;
		m_pFreeChunkHead = pNextChunk;
	}
	else if(!pNextChunk)			//链表尾
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
