/*******************************************************
1��Ӧ�ò���Ҫ���ش��࣬������Ϊ������߼����߳�
2����Ϣ���߼��������̣�
A���ȵ���ProcessClientMsg,������Ϊ�ͻ������ӵ���Ϣ
B���ٵ���ProcessServerMsg��������Ϊ������յ�����Ϣ
C���ٵ���ProcessLogic�����������ճ��߼�
(��Ϊ����ͬһ�߳���˳�򻯴������ԣ����߳��������������Բ�����)
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
	void SetFrameTimePara(int nFrameMinTime,int nFrameWarnTime,int nFrameAbortTime);	//һ�㲻���ã�����Ĭ��ֵ
	void PushMsgToList(DBuf* pMsgBuf,BOOL bInHead = FALSE);	//�ڶ���������ʾ�Ƿ���ӵ�����ͷ,һ����ӵ�����β
	int GetMsgListNum();
	
public:	
	virtual void ProcessMsg(DBuf* pMsgBuf) = 0;	//������Ϣ������Ϣ���ͻ�����Ҫ�̳�
	virtual void ProcessLogic() = 0;			//�����߼���������Ҫ����
		
protected:
	DWORD m_dwThreadID;
	HANDLE m_hThread;
	BOOL m_bRunFlag;

	int m_nFrameMinTime;		//��С֡ʱ��,Ĭ��5����.��һ֡ʱ�����Ĳ����������ʱ����Sleep������ּ�ȥ����
	int m_nFrameWarnTime;		//��һ֡����ʱ������������ʱ�����ӡ����.Ĭ��Ϊ1000����
	int m_nFrameAbortTime;		//��һ֡����ʱ������������ʱ�����������ʣ�µ���Ϣ.Ĭ��Ϊ30��

private:	
	VecThreadMsgPtr m_vecMsg;
	DLockerBuff m_LockMsgVec;
};

#endif