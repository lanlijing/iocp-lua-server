/*******************************************************
1、应该层需要重载此类，用来作为程序的逻辑主线程
2、消息及逻辑处理流程：
A、先调用ProcessClientMsg,处理作为客户端连接的消息
B、再调用ProcessServerMsg，处理作为服务端收到的消息
C、再调用ProcessLogic函数，处理日常逻辑
(因为都在同一线程里顺序化处理，所以，主线程数据区甚至可以不加锁)
*******************************************************/
#ifndef _DAPPTHREAD_H_
#define _DAPPTHREAD_H_

#pragma once

#include "DLockerBuff.h"

class DBuf;
class DAppThread
{
public:
	typedef vector<DBuf*> VecThreadMsgPtr;

public:
	DAppThread();
	virtual ~DAppThread();

	BOOL Start();
	void Stop();
	static DWORD WINAPI MyThreadProcess(void* pParam);
	DWORD RealThreadProc();

public:
	void SetFrameTimePara(int nFrameMinTime,int nFrameWarnTime,int nFrameAbortTime);	//一般不设置，保持默认值
	void PushMsgToList(DBuf* pMsgBuf,BOOL bInHead = FALSE);	//第二个参数表示是否添加到队列头,一般添加到队列尾
	int GetMsgListNum();
	
public:	
	virtual void ProcessMsg(DBuf* pMsgBuf) = 0;	//处理消息队列消息，客户端需要继承
	virtual void ProcessLogic() = 0;			//处理逻辑，子类需要重载
		
protected:
	DWORD m_dwThreadID;
	HANDLE m_hThread;
	BOOL m_bRunFlag;

	int m_nFrameMinTime;		//最小帧时间,默认5毫秒.当一帧时间消耗不足这个数字时，会Sleep这个数字减去消耗
	int m_nFrameWarnTime;		//当一帧消耗时间大于这个数字时，会打印警告.默认为1000毫秒
	int m_nFrameAbortTime;		//当一帧消耗时间大于这个数字时，会放弃处理剩下的消息.默认为30秒

private:	
	VecThreadMsgPtr m_vecMsg;
	DLockerBuff m_LockMsgVec;
};

#endif