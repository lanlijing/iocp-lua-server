/***********************************************************************************
Name:DMemoryPool
comment:
1、内存池类，能自动增长
2、目前只支持64位windows
***********************************************************************************/

#ifndef _DMEMORYPOOL_H_
#define _DMEMORYPOOL_H_

#pragma once

#include "DObjectPool.h"

struct stMemoryChunk
{
	ULONG64 ullMemoAddr;		//内存起始地址(64位)
	ULONG64 ullMemoLen;		//内存长度
	stMemoryChunk* pPre;	
	stMemoryChunk* pNext;
	
	stMemoryChunk()
	{
		Init();
	}
	//每次从对象池中取出来时，都调用一次Init
	void Init()
	{
		ullMemoAddr = ullMemoLen = 0;
		pPre = pNext = nullptr;
	}

	void InitMyMemory()
	{
		char* pMemo = (char*)ullMemoAddr;
		memset(pMemo,0, ullMemoLen);
	}
};

#define MEMP_MINCHUNKSIZE		32				//最小内存块，GetMemory传出的内存都是这个数字的整数倍
#define MEMP_MININCSIZE			1024			//最小的incsize尺寸，不小于1024字节
#define MEMP_MAXINCSIZE			402653184		//最大的incsize尺寸,不超过384M
#define MEMP_MAXMEMORYSIZE		4294967295		//内存池最大尺寸，比4G少一个字节(可以自行设定为更大)

class DMemoryPool
{
private:
	typedef vector<char*>	VecMemPtr;
	typedef map<ULONG64,stMemoryChunk*> ChunkMap;
	typedef map<ULONG64,stMemoryChunk*>::iterator ChunkMapIter;
	
private:
	DMemoryPool();
	~DMemoryPool();

public:	
	static DMemoryPool* Instance();

	BOOL StartPool(ULONG64 ullIncSize = 67108864);		//默认是以64兆为每次申请内存大小
	char* GetMemory(ULONG64 ullSize);							//获取内存块，以32字节为单位，实际给出的内存是32字节的整数倍
	BOOL BackMemory(char** ppMem);						/*收回内存，并会把传进来的参数置为0(这样做是为了防止调用者忘记了把指针置为空，
														在已经退还内存后还对原来的指针进行操作)，如果退回来的内存不在内存池里，返回FALSE*/
	ULONG64 GetAllSize();									//整个内存池的大小
	ULONG64 GetNoUseSize();								//未使用过的内存池大小
	void Clear();
private:
	BOOL AddMemoPool();
	ULONG64 AlignNumForMinSize(ULONG64& ullIn);						//把传过来的参数变成32对齐
	char* RealPopMemory(ULONG64 ullSize);							//实际分配内存
	void InsertChunkToFree(stMemoryChunk* pInsertChunk);			//插入一个Chunk到空闲的双向链表
	void RemoveFreeChunk(stMemoryChunk* pRemoveChunk);			//一个内存块从Free链表到使用map
	
private:	
	VecMemPtr m_VecPool;								//一个二维数组，实际的内存区域
	stMemoryChunk* m_pFreeChunkHead;					//未分配的内存块链表头
	stMemoryChunk* m_pFreeChunkTail;					//未分配的内存块链表尾
	ChunkMap m_mapChunkUsed;							//所有被分配出去的内存块map，以地址为key
	DObjectPool<stMemoryChunk> m_PoolChunk;				//内存块结构体池

	ULONG64 m_ullIncSize;									//每次分配的内存池尺寸大小
	DLockerBuff m_Locker;
};

#endif