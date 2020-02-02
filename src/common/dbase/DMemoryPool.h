/***********************************************************************************
Name:DMemoryPool
comment:
1���ڴ���࣬���Զ�����
2��Ŀǰֻ֧��64λwindows
***********************************************************************************/

#ifndef _DMEMORYPOOL_H_
#define _DMEMORYPOOL_H_

#pragma once

#include "DObjectPool.h"

struct stMemoryChunk
{
	ULONG64 ullMemoAddr;		//�ڴ���ʼ��ַ(64λ)
	ULONG64 ullMemoLen;		//�ڴ泤��
	stMemoryChunk* pPre;	
	stMemoryChunk* pNext;
	
	stMemoryChunk()
	{
		Init();
	}
	//ÿ�δӶ������ȡ����ʱ��������һ��Init
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

#define MEMP_MINCHUNKSIZE		32				//��С�ڴ�飬GetMemory�������ڴ涼��������ֵ�������
#define MEMP_MININCSIZE			1024			//��С��incsize�ߴ磬��С��1024�ֽ�
#define MEMP_MAXINCSIZE			402653184		//����incsize�ߴ�,������384M
#define MEMP_MAXMEMORYSIZE		4294967295		//�ڴ�����ߴ磬��4G��һ���ֽ�(���������趨Ϊ����)

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

	BOOL StartPool(ULONG64 ullIncSize = 67108864);		//Ĭ������64��Ϊÿ�������ڴ��С
	char* GetMemory(ULONG64 ullSize);							//��ȡ�ڴ�飬��32�ֽ�Ϊ��λ��ʵ�ʸ������ڴ���32�ֽڵ�������
	BOOL BackMemory(char** ppMem);						/*�ջ��ڴ棬����Ѵ������Ĳ�����Ϊ0(��������Ϊ�˷�ֹ�����������˰�ָ����Ϊ�գ�
														���Ѿ��˻��ڴ�󻹶�ԭ����ָ����в���)������˻������ڴ治���ڴ�������FALSE*/
	ULONG64 GetAllSize();									//�����ڴ�صĴ�С
	ULONG64 GetNoUseSize();								//δʹ�ù����ڴ�ش�С
	void Clear();
private:
	BOOL AddMemoPool();
	ULONG64 AlignNumForMinSize(ULONG64& ullIn);						//�Ѵ������Ĳ������32����
	char* RealPopMemory(ULONG64 ullSize);							//ʵ�ʷ����ڴ�
	void InsertChunkToFree(stMemoryChunk* pInsertChunk);			//����һ��Chunk�����е�˫������
	void RemoveFreeChunk(stMemoryChunk* pRemoveChunk);			//һ���ڴ���Free����ʹ��map
	
private:	
	VecMemPtr m_VecPool;								//һ����ά���飬ʵ�ʵ��ڴ�����
	stMemoryChunk* m_pFreeChunkHead;					//δ������ڴ������ͷ
	stMemoryChunk* m_pFreeChunkTail;					//δ������ڴ������β
	ChunkMap m_mapChunkUsed;							//���б������ȥ���ڴ��map���Ե�ַΪkey
	DObjectPool<stMemoryChunk> m_PoolChunk;				//�ڴ��ṹ���

	ULONG64 m_ullIncSize;									//ÿ�η�����ڴ�سߴ��С
	DLockerBuff m_Locker;
};

#endif