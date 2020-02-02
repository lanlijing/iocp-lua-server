#ifndef _STIODATA_H_
#define _STIODATA_H_

#pragma once

#include "DNetworkBase.h"

#define IoNull		0
#define IoRead		101
#define IoWrite		102
#define IoAccept	103
#define IoClose		104

/**************************************************************
1�����ֽڶ���
2���Ⱥ�˳��Ҫ�����޸�
**************************************************************/
struct stIoData
{
	OVERLAPPED		overlapped;						//�ص�IO
	int				nOpType;						//�������ͣ��Զ��壩
	void*			pDIOCPLinker;					//��Ӧ������ָ��

	stIoData()
	{
		ZeroMemory(&overlapped,sizeof(OVERLAPPED));
		nOpType = IoAccept;
		pDIOCPLinker = nullptr;
	}
};

struct stConnIoData : public stIoData
{
	enum{eBufLen = (sizeof(SOCKADDR_IN) + 16) * 2,};
	char databuf[eBufLen];
	stConnIoData()
	{
		nOpType = IoAccept;
		ZeroMemory(databuf,eBufLen);
	}
};

struct stRecvIoData : public stIoData
{
	char		databuf[NETIOBUFLEN];
	WSABUF		wsabuf;

	stRecvIoData()
	{
		nOpType = IoRead;
		ZeroMemory(databuf,NETIOBUFLEN);
		wsabuf.buf = databuf;
		wsabuf.len = NETIOBUFLEN;
	}

	~stRecvIoData()
	{	
	}
};

struct stSendIoData : public stIoData
{
	char		databuf[NETIOBUFLEN];
	WSABUF		wsabuf[MAXSENDWSABUF];
	u_int		uValidwsabufNum;					//��Ч����������

	stSendIoData()
	{
		nOpType = IoWrite;
		ZeroMemory(databuf,NETIOBUFLEN);
		uValidwsabufNum = 0;
		InitWsaBuf();
	}

	void InitWsaBuf()
	{
		for(int i = 0; i < MAXSENDWSABUF; i++)
		{
			wsabuf[i].buf = 0;
			wsabuf[i].len = 0;
		}
	}

	~stSendIoData()
	{
	}
};

#endif