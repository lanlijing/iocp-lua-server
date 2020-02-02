#ifndef _DBUF_H_
#define _DBUF_H_

#pragma once

#include "DObjectPool.h"

struct DBufType								//DBUF������
{
	enum
	{
		eUser,											//Ĭ�ϣ��û��Զ���

		eNetServerAcceptMsg,							//��Ϊ������������ӽ���
		eNetServerNormalMsg,							//��Ϊ������յ���ͨ������Ϣ
		eNetServer843Msg,								//��Ϊ������յ���ȫ������Ϣ
		eNetServerHttpRequestMsg,						//��Ϊ���������http�������Ϣ
		eNetServerCloseLinkerMsg,						//��Ϊ����˹ر�����

		eNetClientNormalMsg,							//��Ϊ�ͻ����յ���ͨ��Ϣ
		eNetClientHttpMsg,								//��Ϊ�ͻ����յ�http��Ϣ
		eNetClientDisConnected,							//��Ϊ�ͻ��˱��Ͽ�

		eAppDispatchMsg,								//Ӧ�ò㷢������Ϣ
	};
};

/*
�������ڱ�ʾһ���ֽ���
ע��ͻ���������ͨѶ��stringĬ��Ϊutf8
*/
class DBuf
{
private:
	DBuf();
	~DBuf();
	friend class DObjectPool < DBuf > ;

public:
	void FreeBuf();
	void InitTypeFrom();
	BOOL Attach(char* pBuf,int nBufLen);
	void DAttach();
	void Reset();
	BOOL Attached(){return m_bAttached;}
	char* GetCurPos(){return m_pBuf ? (m_pBuf + m_nCurPos) : nullptr;}
	char* SetCurPos(int nOffset){m_nCurPos = nOffset; return m_pBuf ? (m_pBuf + nOffset) : nullptr;}
	int GetCurPosOffset(){return m_nCurPos;}
	char* GetBuf(){return m_pBuf;}
	int  GetLength(){return m_nBufLen;}
	int GetRemainLen(){return (m_nBufLen - m_nCurPos);};
	void BeginAppend(){ SetCurPos(GetLength()); };
	void Clone(DBuf* pSrc);

	BOOL ReadByte(BYTE& byValue);
	BOOL ReadShort(short& shtValue);
	BOOL ReadInteger(int& intValue);
	BOOL ReadUint32(UINT32& utValue);
	BOOL ReadDouble(double& dblValue);
	BOOL ReadString(string& strValue);
	BOOL ReadStringU2A(string& strValue);				// �������UFT8�ַ�ת��gbkVC�����

	BOOL WriteByte(BYTE byValue);
	BOOL WriteShort(short shtValue);
	BOOL WriteInteger(int intValue);
	BOOL WriteUint32(UINT32 utValue);
	BOOL WriteDouble(double dblValue);
	BOOL WriteString(const string& strValue);
	BOOL WriteStringA2U(const string& strValue);		// ��gbk������ַ���ת��UTF8��д��
	BOOL WriteBuffer(const char* szBuf,int nLen);

public:
	BOOL SetNetMsg(BYTE byType, DWORD dwFrom, const char* szMsg, int nBufLen);
	void BeginNetDBuf(UINT32 utTag,UINT32 utNetObjID,UINT32 utMsgID);
	void EndNetDBuf();
		
public:
	enum{eMinAllocSize = 256,};			//ÿ�η�����ڴ����256�ֽڵ�������
	enum{eInitPoolSize = 102400,};

private:
	char* GetMem(int nAllocSize);
	int AlignNumForMinSize(int& nIn);
	char* Shift(int nSize);

public:
	static DBuf* TakeNewDBuf();
	static void BackDBuf(DBuf* pBack);

public:
	DWORD m_dwBufType;						//���ڱ�ʾDBUF����
	DWORD m_dwBufFrom;						//��ʾDBUF����Դ,��������ʱ���������ӵ�ID

private:
	char* m_pBuf;						//��ʵ��������
	int   m_nBufLen;
    int   m_nBufRealSize;
    int   m_nCurPos;					//���һ��д���ָ����һ��δд���ĵط�
	BOOL  m_bAttached;

	static DObjectPool<DBuf>* s_poolPtr;
};

#endif