#include "stdafx.h"
#include "geneserver.h"
#include <direct.h>
#include "DAppLog.h"
#include "DMemoryPool.h"
#include "DBuf.h"
#include "DNetMsgBase.h"
#include "DNetMsg.h"
#include "DNetWorkBase.h"
#include "geneserverDlg.h"
#include "LuaMgr.h"
#include "GeneServerThread.h"

GeneServerThread::GeneServerThread()
{
	m_pMainDlg = nullptr;
	m_strCommCaption = "";

	m_byType = 0;	
	m_bLuaInit = FALSE;
	m_bReloadLua = FALSE;
	m_strLuaFile = "";

	m_ushServerPort = 0;
	m_dwGameServerConn = 0;
	
	m_addrGameServer.Init();
	
	m_ullGameServerConnTick = 0;
}

GeneServerThread::~GeneServerThread()
{

}

GeneServerThread* GeneServerThread::Instance()
{
	static GeneServerThread s_inst;
	return &s_inst;
}

string GeneServerThread::GetTypeName()
{
	string strRet;
	switch (m_byType)
	{
	case GeneServerType::eLuaTest:
		strRet = LUATESTNAME;
		break;
	case GeneServerType::eTestClient:
		strRet = TESTCLIENTNAME;
		break;	
	case GeneServerType::eGameServer:
		strRet = GAMESERVERNAME;
		break;
	}		

	return strRet;
}

BOOL GeneServerThread::StartServer(CgeneserverDlg* pMainDlg)
{
	if ((m_byType < GeneServerType::eLuaTest) || (m_byType > GeneServerType::eGameServer))
		return FALSE;

	if (pMainDlg == nullptr)
		return FALSE;
	m_pMainDlg = pMainDlg;

	//��ȡ�����ļ�
	string strConfig = "", strTypeName = GetTypeName();
	ChangeCurDir();
	GetConfigFileName(strConfig);
	char szRead[512] = { 0 }, szLogDir[512] = { 0 };
	GetPrivateProfileStringA("common", "logdir", "", szLogDir, (sizeof(szLogDir) - 1), strConfig.c_str());
	if (strlen(szLogDir) == 0)
	{
		AfxMessageBox(_T("��ȡ�����ļ�����,��������־Ŀ¼"));
		return FALSE;
	}
	GetPrivateProfileStringA("common", "caption", "", szRead, (sizeof(szRead) - 1), strConfig.c_str());
	if (strlen(szRead) == 0)
	{
		AfxMessageBox(_T("��ȡ�����ļ�����,������ͨ��caption"));
		return FALSE;
	}
	m_strCommCaption = szRead;
	memset(szRead, 0, sizeof(szRead));
	GetPrivateProfileStringA(strTypeName.c_str(), "luafile", "", szRead, (sizeof(szRead) - 1), strConfig.c_str());	
	if (strlen(szRead) == 0)
	{
		AfxMessageBox(_T("��ȡ�����ļ�����,������lua�ļ�·��"));
		return FALSE;
	}
	m_strLuaFile = szRead;
	if (m_byType == GeneServerType::eTestClient)		// ��Ϊ��ʱ�ͻ���,��Ҫ������Ϸ����˵�ַ
	{
		memset(szRead, 0, sizeof(szRead));
		GetPrivateProfileStringA(GAMESERVERNAME, "ip", "", szRead, (sizeof(szRead) - 1), strConfig.c_str());
		if (strlen(szRead) == 0)
		{
			AfxMessageBox(_T("��ȡ�����ļ�����,��������Ϸ��������ַ"));
			return FALSE;
		}
		m_addrGameServer.m_strAddr = szRead;
		m_addrGameServer.m_nFarPort = GetPrivateProfileIntA(GAMESERVERNAME, "port", 0, strConfig.c_str());
		if (m_addrGameServer.m_nFarPort == 0)
		{
			AfxMessageBox(_T("��ȡ�����ļ�����,������Ϸ�������˿�"));
			return FALSE;
		}
	}
	else if (m_byType == GeneServerType::eGameServer)		// ��Ϊ��Ϸ�����,��Ҫ��������˿�
	{
		m_ushServerPort = GetPrivateProfileIntA(strTypeName.c_str(), "port", 0, strConfig.c_str());
		if (m_ushServerPort == 0)
		{
			AfxMessageBox(_T("��ȡ�����ļ�����,�����������������˿�"));
			return FALSE;
		}
	}
	
	//
	DAppLog::Instance()->Init(szLogDir, strTypeName.c_str(), m_pMainDlg->GetSafeHwnd());
	DMemoryPool::Instance()->StartPool();
	if (m_byType == GeneServerType::eGameServer)
		DNetWorkBase::Instance()->InitNetObjectPool(16, 1600, 160000);				//��Ϊ����ˣ����linker�����ݶ�Ϊ1600
	else
		DNetWorkBase::Instance()->InitNetObjectPool(16, 16, 1600);					// ���������,linker���ý�С
	DNetWorkBase::Instance()->SocketStart();		
	
	//����IOCP������	
	if (m_byType == GeneServerType::eGameServer)
	{
		m_ushServerPort = DNetWorkBase::Instance()->CreateIOCPServer(m_ushServerPort, this);
		if (m_ushServerPort == 0)
		{
			AfxMessageBox(_T("������������������"));
			return FALSE;
		}
	}

	// ������Ϣ�����߳�
	Start();

	return TRUE;
}

void GeneServerThread::StopServer()
{
	Stop();

	DNetWorkBase::Instance()->CleanAllServerClient();
	DNetWorkBase::Instance()->CleanUpNetObjectPool();
	DNetWorkBase::Instance()->SocketCleanup();

	DAppLog::Instance()->StopLog();
}

void GeneServerThread::AddLog(const char* szLog)
{
	DAppLog::Instance()->Info(TRUE, szLog);
}

void GeneServerThread::SetCaption(const char* szCaption)
{
	char buf[512] = { 0 };
	_snprintf(buf, sizeof(buf) - 1, "%s %s ���̺�:%d", m_strCommCaption.c_str(), szCaption, GetCurrentProcessId());
	USES_CONVERSION;
	CString strTemp = A2W(buf);
	m_pMainDlg->SetWindowText(strTemp);
}

int GeneServerThread::SendDBufToGameServer(DBuf* pSendDBuf)
{
	if (m_byType != GeneServerType::eTestClient) {
		DAppLog::Instance()->Info(TRUE, "GeneServerThread::SendDBufToGameServer,���ͱ���ΪeTestClient��");
		return 1;
	}

	return DNetWorkBase::Instance()->SendConnMsg(m_dwGameServerConn, pSendDBuf);
}

int GeneServerThread::SendDBufToClient(DWORD dwLinkerId, DBuf* pSendDBuf)
{
	if (m_byType != GeneServerType::eGameServer) {
		DAppLog::Instance()->Info(TRUE, "GeneServerThread::SendDBufToClient,���ͱ���ΪeGameServer��");
		return 1;
	}

	DNetWorkBase::Instance()->SendNetMsg(m_ushServerPort, dwLinkerId, pSendDBuf->GetBuf(), pSendDBuf->GetLength());
	return 0;
}

void GeneServerThread::ProcessMsg(DBuf* pMsgBuf)
{
	// ��ʽ��ʼ������Ϣ
	BYTE byMsgType = pMsgBuf->m_dwBufType;
	DWORD dwFrom = pMsgBuf->m_dwBufFrom;
	UINT32 unDBufLen = 0,unUserID = 0, unMsgID = 0;
	if (byMsgType == DBufType::eAppDispatchMsg)
	{
		if (dwFrom != 0)
			return;
		pMsgBuf->ReadUint32(unDBufLen);
		pMsgBuf->ReadUint32(unDBufLen);
		pMsgBuf->ReadUint32(unUserID);
		pMsgBuf->ReadUint32(unMsgID);
		if (unUserID != 0)
			return;
		if (unMsgID == NetMsgID::SYSTEM_MSG_LUADEBUG)
		{
			string strScript = "";
			pMsgBuf->ReadString(strScript);
			if (strScript.length() == 0)
				return;
			LuaMgr::Instance()->DoString(strScript.c_str());
		}
		else if (unMsgID == NetMsgID::SYSTEM_MSG_CLOSE)
		{
			LuaFunc::CallLuaOnGameExit();
			//�������ڷ��˳���Ϣ
			m_pMainDlg->SendMessage(WM_CLOSE);
		}
	}
	else if (byMsgType == DBufType::eNetClientDisConnected)  // ֻ��TestClientʱ��
	{
		if ((m_byType == GeneServerType::eTestClient) && (dwFrom == m_dwGameServerConn))
		{
			DNetWorkBase::Instance()->CloseClientConn(m_dwGameServerConn);		//��ʽ���ã�������Դй¶
			m_dwGameServerConn = 0;

			LuaFunc::CallLuaOnGameSvrDisConnect();
		}
	}
	else if (byMsgType == DBufType::eNetClientNormalMsg)		// ֻ��TestClientʱ��
	{
		if ((m_byType == GeneServerType::eTestClient) && (dwFrom == m_dwGameServerConn))
			LuaFunc::CallLuaOnGameSvrMsg(pMsgBuf);
	}
	else if (byMsgType == DBufType::eNetServerNormalMsg)		// ֻ��GameServerʱ��
	{
		if (m_byType == GeneServerType::eGameServer)
		{
			LuaFunc::CallLuaOnClientMsg(pMsgBuf, pMsgBuf->m_dwBufFrom);
		}
	}
	else if (byMsgType == DBufType::eNetServerAcceptMsg)			// ֻ��GameServerʱ��
	{
		if (m_byType == GeneServerType::eGameServer)
		{
			LuaFunc::CallLuaOnClientConnect(pMsgBuf->m_dwBufFrom);
		}
	}
	else if (byMsgType == DBufType::eNetServerCloseLinkerMsg)		// ֻ��GameServerʱ��
	{
		if (m_byType == GeneServerType::eGameServer)
		{
			LuaFunc::CallLuaOnClientDisConnect(pMsgBuf->m_dwBufFrom);
		}
	}
}

void GeneServerThread::ProcessLogic()
{
	if (!m_bLuaInit)							// ���ڴ˴���ʼ����Ϊ��LUA���������̴߳���
	{
		LuaMgr::Instance()->DoFile(m_strLuaFile.c_str());
		LuaFunc::CallLuaOnGameInit();
		m_bLuaInit = TRUE;
	}
	// �Ƿ�Ҫ���¼���LUA�ű�
	if (m_bReloadLua)
	{
		LuaFunc::CallLuaOnGameExit();
		if (m_byType == GeneServerType::eTestClient)							// ���Կͻ��˻�Ͽ������ط�����
		{
			DNetWorkBase::Instance()->CloseClientConn(m_dwGameServerConn);
			m_dwGameServerConn = 0;
		}
		
		LuaMgr::Instance()->ReLoadLua();
		m_bLuaInit = FALSE;
		m_bReloadLua = FALSE;
	}

	// �ͷ�����������
	if (m_byType == GeneServerType::eTestClient)
	{
		GameServerReConn();
	}
	
	if (m_bLuaInit)
		LuaFunc::CallLuaOnGameFrame();
}

void GeneServerThread::GameServerReConn()
{
	if (m_dwGameServerConn != 0)
		return;

	ULONGLONG ullNow = GetTickCount64();
	if ((ullNow - m_ullGameServerConnTick) < 5000)		//5������һ��
		return;

	m_dwGameServerConn = DNetWorkBase::Instance()->CreateClientConn(m_addrGameServer.m_strAddr.c_str(), m_addrGameServer.m_nFarPort, this);
	m_ullGameServerConnTick = GetTickCount64();

	if (m_dwGameServerConn == 0)
		DAppLog::Instance()->Info(TRUE, "������Ϸ��ʧ��,���ڳ�������");
	else {
		DAppLog::Instance()->Info(TRUE, "��Ϸ�����ӳɹ�");
		LuaFunc::CallLuaOnGameSvrConnect();
	}
}

string GeneServerThread::GetConfigFileName(string& strConfig)
{
	strConfig = "";
	char szFileName[1024] = { 0 };
	GetModuleFileNameA(GetModuleHandleA(NULL), szFileName, (sizeof(szFileName) - 1));
	char* szPos = strrchr(szFileName, '\\');
	if (szPos == NULL)
	{
		AfxMessageBox(_T("��ȡ��ǰ����·�����󣬿���·�����������ַ�"));
		return strConfig;
	}
	strcpy(szPos + 1, "config.ini");

	strConfig = szFileName;
	return strConfig;
}

void GeneServerThread::ChangeCurDir()
{
	char szCurrentDir[512] = { 0 };
	GetModuleFileNameA(GetModuleHandleA(NULL), szCurrentDir, sizeof(szCurrentDir) - 1);
	char* szPos = strrchr(szCurrentDir, '\\');
	*(szPos + 1) = 0;
	_chdir(szCurrentDir);
}