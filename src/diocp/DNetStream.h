#ifndef _DNETSTREAM_H_
#define _DNETSTREAM_H_

#pragma once

class INetStreamObj;
class DBuf;
class DNetStream
{
public:
	DNetStream();
	~DNetStream();

	BOOL Init(INetStreamObj* pObj);		//初始化，分配内存
	void Final();							//释放资源

	BOOL DealWithSocketMsg(const char* szInBuf,DWORD dwRecv);	//最重要的函数，处理粘包
	BOOL CheckSafeSandBox();									//检查是否有安全沙箱消息，此函数不能合并到loopbuffer中，因为loopbuffer是循环调用,用于网页游戏
	BOOL LoopBuffer();											//轮询整个BUFFER，查找有没有完整的消息包
	void FreeBuf();

private:
	char* GetMem(int nAllocSize);
	int AlignNumForMinSize(int& nIn);

public:
	enum{eMinBufferSize = 1024,};							//每次最小申请内存
	
private:
	INetStreamObj* m_pObj;					//对应的连接
	char* m_pBuffer;	
	int m_nBufferUse;							//已经使用了多少BUFFER，其实是STREAM的长度
	int m_nBufRealSize;							//buf的实际长度
	DBuf* m_pDBuf;								//用于处理网络消息的DBUF类

	BOOL m_bSafeReqChecked;						//是否需要检查安全策略请求,用于网页游戏
};


#endif