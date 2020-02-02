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
����ճ���ĺ���
������
szInBuf:IOCP�߳����յ�����Ϣ��
dwRecv:IOCP�߳����յ�����Ϣ������

������
1�������ж��Ƿ���һ�����������������������ֱ�Ӹ�LINKER
2��Ȼ��Ѱ��ڴ濽����������
3���ӻ��������㿪ʼ�����ȣ���ѯ����û�а�ͷ��ʶ������У�����û���������ݡ�����У�����������ݷ���LINKER
*/
BOOL DNetStream::DealWithSocketMsg(const char* szInBuf,DWORD dwRecv)
{
	char szLog[1024] = {0};
	//�����ж��Ƿ���HTTP����
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
	//�����ж��Ƿ���������Ϣ
	if (dwRecv >= NETMSG_HEADER_LEN)			
	{
		m_pDBuf->Attach((char*)szInBuf,dwRecv);
		UINT32 utRead = 0;
		if (m_pDBuf->ReadUint32(utRead))
		{
			if (utRead == NET_MSG_TAG)			//����Ϣ��ͷ���ҿ�������������
			{
				m_pDBuf->ReadUint32(utRead);		//������Ϣ�����ȣ����������ĳ��ȣ�������Ϣͷ��
				if ((utRead >= MAX_NETMSG_LEN) || (utRead < NETMSG_HEADER_LEN))			
				{
					_snprintf(szLog,sizeof(szLog),"DNetStream::DealWithSocketMsg.������Ϣ���Ȳ��Ϸ�.IP:%s,ID:%d��Ϣ����:%d.��Ϣ����:",
						addrInfo.m_strIP.c_str(), dwObjID, utRead);
					strncat(szLog,szInBuf,800);	//���յ�����Ϣ��ӡ����,Ҫ��ֹ����Խ��
					m_pObj->PrintLogInfo(szLog);
					m_pObj->OnErrorMessage();
					return FALSE;
				}
				else if(utRead == dwRecv)		//������������� һ�����������Ͳ�����������ֱ�ӷ���Ӧ�ò�
				{
					m_pObj->OnNorMalMessage(szInBuf,dwRecv);
					return TRUE;
				}
			}
		}
	}
	
	//���յ�����Ϣ������������
	if((m_nBufferUse + dwRecv) >= MAX_NETMSG_LEN)
	{
		_snprintf(szLog, sizeof(szLog), "DNetStream::DealWithSocketMsg.����ճ���������������ֵ.IP:%s,ID:%d", addrInfo.m_strIP.c_str(), dwObjID);
		m_pObj->PrintLogInfo(szLog);
		m_pObj->OnErrorMessage();
		return FALSE;
	}
	CopyMemory(GetMem(dwRecv),szInBuf,dwRecv);
	m_nBufferUse += dwRecv;
	
	//����Ƿ��ǰ�ȫɳ���ı�
	if(m_bSafeReqChecked)
	{
		if(CheckSafeSandBox())
			return TRUE;
	}

	LoopBuffer();		//��ѯ�����Ѿ���������Ϣ��
	
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
		//����ǰ�ȫ����������ջ�����������on843message����		
		ZeroMemory(m_pBuffer,m_nBufRealSize);
		m_nBufferUse = 0;	
		m_bSafeReqChecked = FALSE;

		m_pObj->On843Message();
	}

	return TRUE;
}

/*
���е���Ϣ���Ϸ�������£���������ǰ�ĸ��ַ���������Ϣ��ͷ,������ǣ����ǷǷ�
*/
BOOL DNetStream::LoopBuffer()			
{
	stAddrInfo addrInfo = m_pObj->GetAddrInfo();
	DWORD dwObjID = m_pObj->GetID();
	char szLog[1024] = {0};
	//�������д��������ʱʹ��,��ʽ����Ҫע�͵�.��Ϊ��LOG��־��̫��
	//_snprintf(szLog,sizeof(szLog),"DNetStream::LoopBuffer,IP:%s,ID:%d����ճ��",SocketIden.m_szIP,SocketIden.m_nInneID);//(ʹ�ö���غ�ճ�������Ƶ��)
	//DAppLog::Instance()->Info(FALSE,szLog);

	if (m_nBufferUse < NETMSG_HEADER_LEN)
		return TRUE;

	m_pDBuf->FreeBuf();
	m_pDBuf->Attach(m_pBuffer, m_nBufferUse);
	UINT32 utRead = 0;
	BOOL bRet = m_pDBuf->ReadUint32(utRead);		//��ͷ
	if ((!bRet) || (utRead != NET_MSG_TAG))
	{
		_snprintf(szLog,sizeof(szLog),"DNetStream::LoopBuffer.������Ϣ��ͷ���Ϸ�.IP:%s,ID:%d.BuffUse����:%d.��Ϣ����:",
			addrInfo.m_strIP.c_str(), dwObjID, m_nBufferUse);
		strncat(szLog,m_pBuffer,(m_nBufferUse > 800 ? 800 : m_nBufferUse));	//Ҫ��ֹ����Խ��
		m_pObj->PrintLogInfo(szLog);
		m_pObj->OnErrorMessage();
		return FALSE;
	}

	m_pDBuf->ReadUint32(utRead);		//������Ϣ�����ȣ����������ĳ��ȣ�������Ϣͷ��
	if ((!bRet) || (utRead >= MAX_NETMSG_LEN) || (utRead < NETMSG_HEADER_LEN))
	{
		_snprintf(szLog,sizeof(szLog),"DNetStream::LoopBuffer.������Ϣ��ͷ���Ϸ�.IP:%s,ID:%d.BuffUse����:%d.��Ϣ����:",
			addrInfo.m_strIP.c_str(), dwObjID, m_nBufferUse);
		strncat(szLog,m_pBuffer,(m_nBufferUse > 800 ? 800 : m_nBufferUse));	//Ҫ��ֹ����Խ��
		m_pObj->PrintLogInfo(szLog);
		m_pObj->OnErrorMessage();
		return FALSE;
	}

	if (utRead < m_nBufferUse)
	{
		m_pObj->OnNorMalMessage(m_pBuffer, utRead);

		//�ڴ��ƶ�
		memcpy(m_pBuffer, (m_pBuffer + utRead), (m_nBufferUse - utRead));
		m_nBufferUse -= utRead;

		LoopBuffer();
	}
	else if (utRead == m_nBufferUse)
	{
		m_pObj->OnNorMalMessage(m_pBuffer, utRead);

		ZeroMemory(m_pBuffer,m_nBufRealSize);
		m_nBufferUse = 0;

		//����Ѿ������˳����ڴ档�˻��ڴ棬�´�����Ҫ������(��������һ�γ������ݷ��ͣ��ͻ��û���������ٲ����С)
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
	if(nNewSize > m_nBufRealSize)		//ԭ���ڴ��Ѿ�������
	{
		AlignNumForMinSize(nNewSize);		
		char* pNewBuf = DMemoryPool::Instance()->GetMemory(nNewSize);
		if(!pNewBuf)
		{
			DAppLog::Instance()->Info(TRUE,"Error:DNetStream::GetMem.�ò������ڴ�");
			return nullptr;
		}
		memset(pNewBuf,0,nNewSize);
		if(m_pBuffer)		//ԭ����������
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