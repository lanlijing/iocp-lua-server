#include "stdafx.h"
#include "DAppLog.h"
#include "DMemoryPool.h"
#include "DBuf.h"
#include "StrConvert.h"

DObjectPool<DBuf>* DBuf::s_poolPtr = nullptr;
DBuf::DBuf()
{
	m_pBuf = nullptr;
	m_nBufLen = 0;
    m_nBufRealSize = 0;
    m_nCurPos = 0;
	m_bAttached = FALSE;

	InitTypeFrom();
}

DBuf::~DBuf()
{
}

void DBuf::FreeBuf()
{
	if(m_bAttached)
		DAttach();
	else
	{
		if(m_pBuf)
			DMemoryPool::Instance()->BackMemory(&m_pBuf);
		m_pBuf = nullptr;
		m_nBufLen = 0;
		m_nBufRealSize = 0;
		m_nCurPos = 0;
		m_bAttached = FALSE;
	}
}

void DBuf::InitTypeFrom()
{
	m_dwBufType = DBufType::eUser;
	m_dwBufFrom = 0;
}

BOOL DBuf::Attach(char* pBuf,int nBufLen)
{
	if(m_pBuf)
		FreeBuf();

	if(!pBuf)
		return FALSE;

	m_pBuf = pBuf;
	m_nBufLen = nBufLen;
	m_nBufRealSize = m_nBufLen;
	m_nCurPos = 0;
	m_bAttached = TRUE;

	return TRUE;
}

void DBuf::DAttach()
{
	m_pBuf = nullptr;
	m_nBufLen = 0;
    m_nBufRealSize = 0;
    m_nCurPos = 0;
	m_bAttached = FALSE;
}

void DBuf::Reset()
{	
	m_nBufLen = 0;
	m_nCurPos = 0;
}

void DBuf::Clone(DBuf* pSrc)
{
	FreeBuf();
	InitTypeFrom();

	m_dwBufType = pSrc->m_dwBufType;
	m_dwBufFrom = pSrc->m_dwBufFrom;
	WriteBuffer(pSrc->GetBuf(), pSrc->GetLength());
}

BOOL DBuf::ReadByte(BYTE& byValue)
{
	if(GetRemainLen() < sizeof(BYTE))
		return FALSE;

	byValue = *((BYTE*)(GetCurPos()));
    Shift(sizeof(BYTE));
    return TRUE;
}

BOOL DBuf::ReadShort(short& shtValue)
{
	if(GetRemainLen() < sizeof(short))
		return FALSE;

	shtValue = *((short*)(GetCurPos()));
    Shift(sizeof(short));
    return TRUE;
}

BOOL DBuf::ReadInteger(int& intValue)
{
	if(GetRemainLen() < sizeof(int))
		return FALSE;

	intValue = *(int*)(GetCurPos());
    Shift(sizeof(int));
    return TRUE;
}

BOOL DBuf::ReadUint32(UINT32& utValue)
{
	if (GetRemainLen() < sizeof(UINT32))
		return FALSE;

	utValue = *(UINT32*)(GetCurPos());
	Shift(sizeof(UINT32));
	return TRUE;
}

BOOL DBuf::ReadDouble(double& dblValue)
{
	if (GetRemainLen() < sizeof(double))
		return FALSE;

	dblValue = *(double*)(GetCurPos());
	Shift(sizeof(double));
	return TRUE;
}

BOOL DBuf::ReadString(string& strValue)
{
	if(GetRemainLen() < sizeof(short))
		return FALSE;

	strValue = "";
    short nLen = 0;
    if (ReadShort(nLen) && nLen > 0)
    {
		if(GetRemainLen() < nLen)
			return FALSE;

		strValue.append((char*)GetCurPos(), nLen);
		strValue += "\0";
        Shift(nLen);
        if (strValue.c_str() == nullptr)  //����Ҫ����Ϊ���ַ���������luaȡ�����ͱ����nil
        {
            strValue = "";
        }
    }
    return TRUE;
}

BOOL DBuf::ReadStringU2A(string& strValue)
{
	ReadString(strValue);
	if (strValue.length() > 0)
	{
		strValue = StrConvert::u2a(strValue);
	}

	return TRUE;
}

BOOL DBuf::WriteByte(BYTE byValue)
{
	BYTE* pCur = (BYTE*)(GetMem(sizeof(BYTE)));
	if (pCur)
	{
		*pCur = byValue;
		Shift(sizeof(BYTE));
		return TRUE;
	}
	return FALSE;
}

BOOL DBuf::WriteShort(short shtValue)
{
	short* pCur = (short*)(GetMem(sizeof(short)));
	if (pCur)
	{
		*pCur = shtValue;
		Shift(sizeof(short));
		return TRUE;
	}
	return FALSE;
}

BOOL DBuf::WriteInteger(int intValue)
{
	int* pCur = (int*)(GetMem(sizeof(int)));
	if (pCur)
	{
		*pCur = intValue;
		Shift(sizeof(int));
		return TRUE;
	}
	return FALSE;
}

BOOL DBuf::WriteUint32(UINT32 utValue)
{
	UINT32* pCur = (UINT32*)(GetMem(sizeof(UINT32)));
	if (pCur)
	{
		*pCur = utValue;
		Shift(sizeof(UINT32));
		return TRUE;
	}
	return FALSE;
}

BOOL DBuf::WriteDouble(double dblValue)
{
	double* pCur = (double*)(GetMem(sizeof(double)));
	if (pCur)
	{
		*pCur = dblValue;
		Shift(sizeof(double));
		return TRUE;
	}
	return FALSE;
}

BOOL DBuf::WriteString(const string& strValue)
{
	short nLen = (short)strValue.length();
    WriteShort(nLen);
    char* pCur = (char*)(GetMem(nLen));
    if (pCur)
    {
        memcpy(pCur, strValue.c_str(), nLen);
        Shift(nLen);
        return TRUE;
    }
    return FALSE;
}

BOOL DBuf::WriteStringA2U(const string& strValue)
{
	string strWrite = StrConvert::a2u(strValue);
	return WriteString(strWrite);
}

BOOL DBuf::WriteBuffer(const char* szBuf,int nLen)
{
	char* pCur = (char*)(GetMem(nLen));
	if(pCur)
	{
		memcpy(pCur,szBuf,nLen);
		return TRUE;
	}
	return FALSE;
}

BOOL DBuf::SetNetMsg(BYTE byType, DWORD dwFrom, const char* szMsg, int nBufLen)
{
	if (m_pBuf)
		FreeBuf();

	Reset();
	m_dwBufType = byType;
	m_dwBufFrom = dwFrom;
	if (szMsg && (nBufLen > 0))			//�����������͵���Ϣ������Ϣ����
	{
		GetMem(nBufLen);
		memcpy(m_pBuf, szMsg, nBufLen);
	}
	return TRUE;
}

void DBuf::BeginNetDBuf(UINT32 utTag, UINT32 utNetObjID, UINT32 utMsgID)
{
	if (m_pBuf)
		FreeBuf();

	WriteUint32(utTag);					//������Ϣ��ͷ
	WriteUint32(0);						//������Ϣ������(ռλ)
	WriteUint32(utNetObjID);			//�������ID
	WriteUint32(utMsgID);				//������ϢID	
}

void DBuf::EndNetDBuf()
{
	//���´�����
	UINT32* utLen = (UINT32*)SetCurPos(sizeof(UINT32));
	*utLen = m_nBufLen;
	SetCurPos(0);
}

char* DBuf::GetMem(int nAllocSize)
{
	if((m_nCurPos + nAllocSize) > m_nBufRealSize)		//ԭ���ڴ��Ѿ�������
	{
		if(m_bAttached)
		{
			DAppLog::Instance()->Info(TRUE,"Error:DBuf::GetMem.m_bAttachedΪTRUEʱ�������������ڴ�");
			return nullptr;		//�����attached�����ܳ����ڴ�
		}

		int nNewSize = (m_nCurPos + nAllocSize);
		AlignNumForMinSize(nNewSize);
		char* pNewBuf = DMemoryPool::Instance()->GetMemory(nNewSize);
		if(!pNewBuf)
		{
			DAppLog::Instance()->Info(TRUE,"Error:DBuf::GetMem.�ò������ڴ�");
			return nullptr;
		}
		memset(pNewBuf,0,nNewSize);
		if(m_pBuf)		//ԭ����������
		{
			memcpy(pNewBuf,m_pBuf,m_nBufLen);
			DMemoryPool::Instance()->BackMemory(&m_pBuf);
		}
		m_pBuf = pNewBuf;
		m_nBufRealSize = nNewSize;
	}

	int nOff = (m_nCurPos + nAllocSize) - m_nBufLen;
    if (nOff > 0)
        m_nBufLen += nOff;

    return (m_pBuf + m_nCurPos);
}

int DBuf::AlignNumForMinSize(int& nIn)
{
	if(nIn < eMinAllocSize)
		nIn = eMinAllocSize;

	int nTemp = nIn % eMinAllocSize;
	if(nTemp != 0)
		nIn += (eMinAllocSize - nTemp);
	
	return nIn;
}

//todo:����Ӧ��Ҫ�ж�һ�£������ƶ����Ϸ����������棬������ɷǷ�����
char* DBuf::Shift(int nSize)
{
	m_nCurPos += nSize;

	if (m_nCurPos > m_nBufLen) //ע�⣬��Э�鲻һ�£��ᵼ�������Ƴ������������棬Ҫ�׳��쳣
	{
		char szMsg[200] = {0};
		_snprintf(szMsg,sizeof(szMsg)-1,"DBuf::shift����Խ���д����ֹ��������д��m_nCurPos=%d m_nBufLen=%d", m_nCurPos, m_nBufLen);
		DAppLog::Instance()->Info(TRUE,szMsg);
		throw new exception(szMsg);
	}
	return m_pBuf + m_nCurPos;
}

DBuf* DBuf::TakeNewDBuf()
{
	static DObjectPool < DBuf > s_pool;
	if (s_poolPtr == nullptr)
	{
		s_poolPtr = &s_pool;
		s_poolPtr->StartPool(eInitPoolSize);
	}

	DBuf* pret = s_poolPtr->TakeObject();
	return pret;
}

void DBuf::BackDBuf(DBuf* pBack)
{
	pBack->FreeBuf();
	pBack->InitTypeFrom();
	if (s_poolPtr)
		s_poolPtr->BackObject(pBack);
}