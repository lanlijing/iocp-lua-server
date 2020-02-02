#ifndef _INETSTREAMOBJ_H_
#define _INETSTREAMOBJ_H_

#pragma once

#include "DNetWorkBase.h"

struct stLinkerSend
{
	char*	m_szMsg;
	int		m_nMsgLen;

	stLinkerSend()
	{
		m_szMsg = NULL;
		m_nMsgLen = 0;
	}
	~stLinkerSend()
	{
	}
};

class INetStreamObj
{
public:
	virtual void OnNorMalMessage(const char* szMsg,int nMsgLen) = 0;
	virtual void On843Message() = 0;
	virtual void OnErrorMessage() = 0;
	virtual void OnTttpRequest(const char* szMsg,int nMsgLen) = 0;
	virtual void PrintLogInfo(const char* szInfo,BOOL bViewInWnd = TRUE) = 0;
	virtual void SendAppThreadMsg(BYTE byMsgType,const char* szMsg = nullptr, int nMsgLen = 0) = 0;
	virtual DWORD GetID() = 0;
	virtual stAddrInfo GetAddrInfo() = 0;
};

#endif