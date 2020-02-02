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

	BOOL Init(INetStreamObj* pObj);		//��ʼ���������ڴ�
	void Final();							//�ͷ���Դ

	BOOL DealWithSocketMsg(const char* szInBuf,DWORD dwRecv);	//����Ҫ�ĺ���������ճ��
	BOOL CheckSafeSandBox();									//����Ƿ��а�ȫɳ����Ϣ���˺������ܺϲ���loopbuffer�У���Ϊloopbuffer��ѭ������,������ҳ��Ϸ
	BOOL LoopBuffer();											//��ѯ����BUFFER��������û����������Ϣ��
	void FreeBuf();

private:
	char* GetMem(int nAllocSize);
	int AlignNumForMinSize(int& nIn);

public:
	enum{eMinBufferSize = 1024,};							//ÿ����С�����ڴ�
	
private:
	INetStreamObj* m_pObj;					//��Ӧ������
	char* m_pBuffer;	
	int m_nBufferUse;							//�Ѿ�ʹ���˶���BUFFER����ʵ��STREAM�ĳ���
	int m_nBufRealSize;							//buf��ʵ�ʳ���
	DBuf* m_pDBuf;								//���ڴ���������Ϣ��DBUF��

	BOOL m_bSafeReqChecked;						//�Ƿ���Ҫ��鰲ȫ��������,������ҳ��Ϸ
};


#endif