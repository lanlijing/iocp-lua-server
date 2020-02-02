#ifndef _DBUF_H_
#define _DBUF_H_

#pragma once

#include "DObjectPool.h"

struct DBufType								//DBUF的类型
{
	enum
	{
		eUser,											//默认，用户自定义

		eNetServerAcceptMsg,							//作为服务端有新连接建立
		eNetServerNormalMsg,							//作为服务端收到普通网络消息
		eNetServer843Msg,								//作为服务端收到安全策略消息
		eNetServerHttpRequestMsg,						//作为服务端来自http请求的消息
		eNetServerCloseLinkerMsg,						//作为服务端关闭连接

		eNetClientNormalMsg,							//做为客户端收到普通消息
		eNetClientHttpMsg,								//做为客户端收到http消息
		eNetClientDisConnected,							//做为客户端被断开

		eAppDispatchMsg,								//应用层发出的消息
	};
};

/*
本类用于表示一个字节流
注意客户端与服务端通讯的string默认为utf8
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
	BOOL ReadStringU2A(string& strValue);				// 把里面的UFT8字符转成gbkVC里可用

	BOOL WriteByte(BYTE byValue);
	BOOL WriteShort(short shtValue);
	BOOL WriteInteger(int intValue);
	BOOL WriteUint32(UINT32 utValue);
	BOOL WriteDouble(double dblValue);
	BOOL WriteString(const string& strValue);
	BOOL WriteStringA2U(const string& strValue);		// 把gbk编码的字符串转成UTF8再写入
	BOOL WriteBuffer(const char* szBuf,int nLen);

public:
	BOOL SetNetMsg(BYTE byType, DWORD dwFrom, const char* szMsg, int nBufLen);
	void BeginNetDBuf(UINT32 utTag,UINT32 utNetObjID,UINT32 utMsgID);
	void EndNetDBuf();
		
public:
	enum{eMinAllocSize = 256,};			//每次分配的内存会是256字节的整数倍
	enum{eInitPoolSize = 102400,};

private:
	char* GetMem(int nAllocSize);
	int AlignNumForMinSize(int& nIn);
	char* Shift(int nSize);

public:
	static DBuf* TakeNewDBuf();
	static void BackDBuf(DBuf* pBack);

public:
	DWORD m_dwBufType;						//用于表示DBUF类型
	DWORD m_dwBufFrom;						//表示DBUF的来源,用于网络时是网络连接的ID

private:
	char* m_pBuf;						//真实的数据区
	int   m_nBufLen;
    int   m_nBufRealSize;
    int   m_nCurPos;					//完成一次写入后，指向下一个未写过的地方
	BOOL  m_bAttached;

	static DObjectPool<DBuf>* s_poolPtr;
};

#endif