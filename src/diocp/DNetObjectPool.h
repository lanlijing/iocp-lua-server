#ifndef _DNETOBJECTPOOL_H_
#define _DNETOBJECTPOOL_H_

#pragma once

#include "DNetWorkBase.h"
#include "DObjectPool.h"
#include "DClientConn.h"
#include "DIOCPLinker.h"
#include "DBuf.h"

class DNetObjectPool
{
private:
	DNetObjectPool();
	~DNetObjectPool();

public:		
	static DNetObjectPool* Instance();

	/*
	LinkerSend  一般是ServerLinker的100倍
	如果是纯客户端,ServerLinker可设为极小的数。如16
	如果是纯服务端 ClientConn可设为极小的数，如16。
	*/
public:
	void Init(int nClientConnIncSize,int nServerLinkerIncSize,int nLinkerSendIncSize);
	void CleanUp();

public:
	DClientConn* TakeClientConn();
	void BackClientConn(DClientConn* pClient);
	DIOCPLinker* TakeServerLinker();
	void BackServerLinker(DIOCPLinker* pLinker);
	stLinkerSend* TakeLinkerSend(const char* szMsg,int nMsgLen);
	void BackLinkerSend(stLinkerSend* pLinkerSend);
	
private:
	DObjectPool<DClientConn> m_ClientConnPool;
	DObjectPool<DIOCPLinker> m_ServerLinkerPool;	
	DObjectPool<stLinkerSend> m_LinkerSendPool;
};

#endif