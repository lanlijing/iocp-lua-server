#include "stdafx.h"
#include "DNetStream.h"
#include "DMemoryPool.h"
#include "INetStreamObj.h"
#include "DBuf.h"
#include "DNetMsgBase.h"

#define TEXTSAFESANDBOX "<policy-file-request/>"
#define LENSAFESADNBOX 22

DNetStream::DNetStream()
{
	m_pObj = nullptr;
	m_pBuffer = nullptr;
	m_nBufferUse = 0;
	m_nBufRealSize = 0;

	m_pDBuf = nullptr;

	m_bSafeReqChecked = TRUE;
}

DNetStream::~DNetStream()
{
	Final();
}

BOOL DNetStream::Init(INetStreamObj* pObj)
{
	if(!pObj)
		return FALSE;
	m_pObj = pObj;

	FreeBuf();
	m_pDBuf = DBuf::TakeNewDBuf();
	m_bSafeReqChecked = TRUE;

	return TRUE;
}

void DNetStream::Final()
{
	FreeBuf();
	if (m_pDBuf != nullptr)
		DBuf::BackDBuf(m_pDBuf);
	m_pObj = nullptr;	
}

/*
处理粘包的函数
参数：
szInBuf:IOCP线程中收到的消息包
dwRecv:IOCP线程中收到的消息包长度

处理方法
1、首先判断是否是一个完整包，如果是完整包，直接给LINKER
2、然后把包内存拷贝到缓冲中
3、从缓冲区的零开始到长度，轮询看有没有包头标识，如果有，看有没有完整数据。如果有，则把完整数据发给LINKER
*/
BOOL DNetStream::DealWithSocketMsg(const char* szInBuf,DWORD dwRecv)
{
	char szLog[1024] = {0};
	//首先判断是否是HTTP请求
	if(strncmp(szInBuf,"GET",3) == 0)
	{
		if(strstr(szInBuf,"HTTP/1.1"))
		{
			m_pObj->OnTttpRequest(szInBuf,dwRecv);
			return TRUE;
		}
	}
	
	stAddrInfo addrInfo = m_pObj->GetAddrInfo();
	DWORD dwObjID = m_pObj->GetID();
	
	m_pDBuf->FreeBuf();
	//首先判断是否是完整消息
	if (dwRecv >= NETMSG_HEADER_LEN)			
	{
		m_pDBuf->Attach((char*)szInBuf,dwRecv);
		UINT32 utRead = 0;
		if (m_pDBuf->ReadUint32(utRead))
		{
			if (utRead == NET_MSG_TAG)			//有消息包头，且可能是完整数据
			{
				m_pDBuf->ReadUint32(utRead);		//网络消息包长度（是整个包的长度，包括消息头）
				if ((utRead >= MAX_NETMSG_LEN) || (utRead < NETMSG_HEADER_LEN))			
				{
					_snprintf(szLog,sizeof(szLog),"DNetStream::DealWithSocketMsg.发送消息长度不合法.IP:%s,ID:%d消息长度:%d.消息内容:",
						addrInfo.m_strIP.c_str(), dwObjID, utRead);
					strncat(szLog,szInBuf,800);	//把收到的消息打印出来,要防止数组越界
					m_pObj->PrintLogInfo(szLog);
					m_pObj->OnErrorMessage();
					return FALSE;
				}
				else if(utRead == dwRecv)		//长度相符，正好 一个完整包，就不进缓冲区，直接发给应用层
				{
					m_pObj->OnNorMalMessage(szInBuf,dwRecv);
					return TRUE;
				}
			}
		}
	}
	
	//把收到的消息拷贝到缓冲区
	if((m_nBufferUse + dwRecv) >= MAX_NETMSG_LEN)
	{
		_snprintf(szLog, sizeof(szLog), "DNetStream::DealWithSocketMsg.处理粘包缓冲区超过最大值.IP:%s,ID:%d", addrInfo.m_strIP.c_str(), dwObjID);
		m_pObj->PrintLogInfo(szLog);
		m_pObj->OnErrorMessage();
		return FALSE;
	}
	CopyMemory(GetMem(dwRecv),szInBuf,dwRecv);
	m_nBufferUse += dwRecv;
	
	//检查是否是安全沙箱文本
	if(m_bSafeReqChecked)
	{
		if(CheckSafeSandBox())
			return TRUE;
	}

	LoopBuffer();		//轮询看是已经有完整消息包
	
	return TRUE;
}

BOOL DNetStream::CheckSafeSandBox()		
{
	if(strncmp(TEXTSAFESANDBOX,m_pBuffer,(LENSAFESADNBOX > m_nBufferUse ? m_nBufferUse : LENSAFESADNBOX)) != 0)
	{
		return m_bSafeReqChecked = FALSE;
	}

	if(m_nBufferUse >= LENSAFESADNBOX)
	{
		//检查是安全策略请求，清空缓冲区，调用on843message函数		
		ZeroMemory(m_pBuffer,m_nBufRealSize);
		m_nBufferUse = 0;	
		m_bSafeReqChecked = FALSE;

		m_pObj->On843Message();
	}

	return TRUE;
}

/*
所有的消息都合法的情况下，缓冲区的前四个字符必须是消息包头,如果不是，即是非法
*/
BOOL DNetStream::LoopBuffer()			
{
	stAddrInfo addrInfo = m_pObj->GetAddrInfo();
	DWORD dwObjID = m_pObj->GetID();
	char szLog[1024] = {0};
	//以下两行代码仅测试时使用,正式发布要注释掉.因为是LOG日志会太多
	//_snprintf(szLog,sizeof(szLog),"DNetStream::LoopBuffer,IP:%s,ID:%d发生粘包",SocketIden.m_szIP,SocketIden.m_nInneID);//(使用对象池后，粘包现象变频繁)
	//DAppLog::Instance()->Info(FALSE,szLog);

	if (m_nBufferUse < NETMSG_HEADER_LEN)
		return TRUE;

	m_pDBuf->FreeBuf();
	m_pDBuf->Attach(m_pBuffer, m_nBufferUse);
	UINT32 utRead = 0;
	BOOL bRet = m_pDBuf->ReadUint32(utRead);		//包头
	if ((!bRet) || (utRead != NET_MSG_TAG))
	{
		_snprintf(szLog,sizeof(szLog),"DNetStream::LoopBuffer.发送消息包头不合法.IP:%s,ID:%d.BuffUse长度:%d.消息内容:",
			addrInfo.m_strIP.c_str(), dwObjID, m_nBufferUse);
		strncat(szLog,m_pBuffer,(m_nBufferUse > 800 ? 800 : m_nBufferUse));	//要防止数组越界
		m_pObj->PrintLogInfo(szLog);
		m_pObj->OnErrorMessage();
		return FALSE;
	}

	m_pDBuf->ReadUint32(utRead);		//网络消息包长度（是整个包的长度，包括消息头）
	if ((!bRet) || (utRead >= MAX_NETMSG_LEN) || (utRead < NETMSG_HEADER_LEN))
	{
		_snprintf(szLog,sizeof(szLog),"DNetStream::LoopBuffer.发送消息包头不合法.IP:%s,ID:%d.BuffUse长度:%d.消息内容:",
			addrInfo.m_strIP.c_str(), dwObjID, m_nBufferUse);
		strncat(szLog,m_pBuffer,(m_nBufferUse > 800 ? 800 : m_nBufferUse));	//要防止数组越界
		m_pObj->PrintLogInfo(szLog);
		m_pObj->OnErrorMessage();
		return FALSE;
	}

	if (utRead < m_nBufferUse)
	{
		m_pObj->OnNorMalMessage(m_pBuffer, utRead);

		//内存移动
		memcpy(m_pBuffer, (m_pBuffer + utRead), (m_nBufferUse - utRead));
		m_nBufferUse -= utRead;

		LoopBuffer();
	}
	else if (utRead == m_nBufferUse)
	{
		m_pObj->OnNorMalMessage(m_pBuffer, utRead);

		ZeroMemory(m_pBuffer,m_nBufRealSize);
		m_nBufferUse = 0;

		//如果已经申请了超大内存。退还内存，下次有需要再申请(否则会出现一次超长数据发送，就会让缓冲区变大再不会变小)
		if(m_nBufRealSize > eMinBufferSize)
			FreeBuf();
	}

	return TRUE;
}

void DNetStream::FreeBuf()
{
	if(m_pBuffer)
		DMemoryPool::Instance()->BackMemory(&m_pBuffer);
	m_pBuffer = nullptr;
	m_nBufferUse = 0;
	m_nBufRealSize = 0;
}

char* DNetStream::GetMem(int nAllocSize)
{
	int nNewSize = (m_nBufferUse + nAllocSize);
	if(nNewSize > m_nBufRealSize)		//原有内存已经不够用
	{
		AlignNumForMinSize(nNewSize);		
		char* pNewBuf = DMemoryPool::Instance()->GetMemory(nNewSize);
		if(!pNewBuf)
		{
			DAppLog::Instance()->Info(TRUE,"Error:DNetStream::GetMem.得不到新内存");
			return nullptr;
		}
		memset(pNewBuf,0,nNewSize);
		if(m_pBuffer)		//原来就有数据
		{
			memcpy(pNewBuf,m_pBuffer,m_nBufferUse);
			DMemoryPool::Instance()->BackMemory(&m_pBuffer);
		}
		m_pBuffer = pNewBuf;
		m_nBufRealSize = nNewSize;
	}

    return (m_pBuffer + m_nBufferUse);
}

int DNetStream::AlignNumForMinSize(int& nIn)
{
	if(nIn < eMinBufferSize)
		nIn = eMinBufferSize;

	int nTemp = nIn % eMinBufferSize;
	if(nTemp != 0)
		nIn += (eMinBufferSize - nTemp);
	
	return nIn;
}