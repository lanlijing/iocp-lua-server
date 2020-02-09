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

	//读取配置文件
	string strConfig = "", strTypeName = GetTypeName();
	ChangeCurDir();
	GetConfigFileName(strConfig);
	char szRead[512] = { 0 }, szLogDir[512] = { 0 };
	GetPrivateProfileStringA("common", "logdir", "", szLogDir, (sizeof(szLogDir) - 1), strConfig.c_str());
	if (strlen(szLogDir) == 0)
	{
		AfxMessageBox(_T("读取配置文件出错,读不到日志目录"));
		return FALSE;
	}
	GetPrivateProfileStringA("common", "caption", "", szRead, (sizeof(szRead) - 1), strConfig.c_str());
	if (strlen(szRead) == 0)
	{
		AfxMessageBox(_T("读取配置文件出错,读不到通用caption"));
		return FALSE;
	}
	m_strCommCaption = szRead;
	memset(szRead, 0, sizeof(szRead));
	GetPrivateProfileStringA(strTypeName.c_str(), "luafile", "", szRead, (sizeof(szRead) - 1), strConfig.c_str());	
	if (strlen(szRead) == 0)
	{
		AfxMessageBox(_T("读取配置文件出错,读不到lua文件路径"));
		return FALSE;
	}
	m_strLuaFile = szRead;
	if (m_byType == GeneServerType::eTestClient)		// 作为临时客户端,需要保存游戏服务端地址
	{
		memset(szRead, 0, sizeof(szRead));
		GetPrivateProfileStringA(GAMESERVERNAME, "ip", "", szRead, (sizeof(szRead) - 1), strConfig.c_str());
		if (strlen(szRead) == 0)
		{
			AfxMessageBox(_T("读取配置文件出错,读不到游戏服务器地址"));
			return FALSE;
		}
		m_addrGameServer.m_strAddr = szRead;
		m_addrGameServer.m_nFarPort = GetPrivateProfileIntA(GAMESERVERNAME, "port", 0, strConfig.c_str());
		if (m_addrGameServer.m_nFarPort == 0)
		{
			AfxMessageBox(_T("读取配置文件出错,读不游戏服务器端口"));
			return FALSE;
		}
	}
	else if (m_byType == GeneServerType::eGameServer)		// 作为游戏服务端,需要保存监听端口
	{
		m_ushServerPort = GetPrivateProfileIntA(strTypeName.c_str(), "port", 0, strConfig.c_str());
		if (m_ushServerPort == 0)
		{
			AfxMessageBox(_T("读取配置文件出错,读不到服务器监听端口"));
			return FALSE;
		}
	}
	
	//
	DAppLog::Instance()->Init(szLogDir, strTypeName.c_str(), m_pMainDlg->GetSafeHwnd());
	DMemoryPool::Instance()->StartPool();
	if (m_byType == GeneServerType::eGameServer)
		DNetWorkBase::Instance()->InitNetObjectPool(16, 1600, 160000);				//作为服务端，最大linker承载暂定为1600
	else
		DNetWorkBase::Instance()->InitNetObjectPool(16, 16, 1600);					// 不作服务端,linker设置较小
	DNetWorkBase::Instance()->SocketStart();		
	
	//创建IOCP服务器	
	if (m_byType == GeneServerType::eGameServer)
	{
		m_ushServerPort = DNetWorkBase::Instance()->CreateIOCPServer(m_ushServerPort, this);
		if (m_ushServerPort == 0)
		{
			AfxMessageBox(_T("创建服务器监听出错"));
			return FALSE;
		}
	}

	// 开启消息处理线程
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
	_snprintf(buf, sizeof(buf) - 1, "%s %s 进程号:%d", m_strCommCaption.c_str(), szCaption, GetCurrentProcessId());
	USES_CONVERSION;
	CString strTemp = A2W(buf);
	m_pMainDlg->SetWindowText(strTemp);
}

int GeneServerThread::SendDBufToGameServer(DBuf* pSendDBuf)
{
	if (m_byType != GeneServerType::eTestClient) {
		DAppLog::Instance()->Info(TRUE, "GeneServerThread::SendDBufToGameServer,类型必须为eTestClient型");
		return 1;
	}

	return DNetWorkBase::Instance()->SendConnMsg(m_dwGameServerConn, pSendDBuf);
}

int GeneServerThread::SendDBufToClient(DWORD dwLinkerId, DBuf* pSendDBuf)
{
	if (m_byType != GeneServerType::eGameServer) {
		DAppLog::Instance()->Info(TRUE, "GeneServerThread::SendDBufToClient,类型必须为eGameServer型");
		return 1;
	}

	DNetWorkBase::Instance()->SendNetMsg(m_ushServerPort, dwLinkerId, pSendDBuf->GetBuf(), pSendDBuf->GetLength());
	return 0;
}

void GeneServerThread::ProcessMsg(DBuf* pMsgBuf)
{
	// 正式开始处理消息
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
			//给主窗口发退出消息
			m_pMainDlg->SendMessage(WM_CLOSE);
		}
	}
	else if (byMsgType == DBufType::eNetClientDisConnected)  // 只有TestClient时有
	{
		if ((m_byType == GeneServerType::eTestClient) && (dwFrom == m_dwGameServerConn))
		{
			DNetWorkBase::Instance()->CloseClientConn(m_dwGameServerConn);		//显式调用，否则资源泄露
			m_dwGameServerConn = 0;

			LuaFunc::CallLuaOnGameSvrDisConnect();
		}
	}
	else if (byMsgType == DBufType::eNetClientNormalMsg)		// 只有TestClient时有
	{
		if ((m_byType == GeneServerType::eTestClient) && (dwFrom == m_dwGameServerConn))
			LuaFunc::CallLuaOnGameSvrMsg(pMsgBuf);
	}
	else if (byMsgType == DBufType::eNetServerNormalMsg)		// 只有GameServer时有
	{
		if (m_byType == GeneServerType::eGameServer)
		{
			LuaFunc::CallLuaOnClientMsg(pMsgBuf, pMsgBuf->m_dwBufFrom);
		}
	}
	else if (byMsgType == DBufType::eNetServerAcceptMsg)			// 只有GameServer时有
	{
		if (m_byType == GeneServerType::eGameServer)
		{
			LuaFunc::CallLuaOnClientConnect(pMsgBuf->m_dwBufFrom);
		}
	}
	else if (byMsgType == DBufType::eNetServerCloseLinkerMsg)		// 只有GameServer时有
	{
		if (m_byType == GeneServerType::eGameServer)
		{
			LuaFunc::CallLuaOnClientDisConnect(pMsgBuf->m_dwBufFrom);
		}
	}
}

void GeneServerThread::ProcessLogic()
{
	if (!m_bLuaInit)							// 放在此处初始化是为了LUA对象由主线程创建
	{
		LuaMgr::Instance()->DoFile(m_strLuaFile.c_str());
		LuaFunc::CallLuaOnGameInit();
		m_bLuaInit = TRUE;
	}
	// 是否要重新加载LUA脚本
	if (m_bReloadLua)
	{
		LuaFunc::CallLuaOnGameExit();
		if (m_byType == GeneServerType::eTestClient)							// 测试客户端会断开与网关服连接
		{
			DNetWorkBase::Instance()->CloseClientConn(m_dwGameServerConn);
			m_dwGameServerConn = 0;
		}
		
		LuaMgr::Instance()->ReLoadLua();
		m_bLuaInit = FALSE;
		m_bReloadLua = FALSE;
	}

	// 和服务器的连接
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
	if ((ullNow - m_ullGameServerConnTick) < 5000)		//5秒重连一次
		return;

	m_dwGameServerConn = DNetWorkBase::Instance()->CreateClientConn(m_addrGameServer.m_strAddr.c_str(), m_addrGameServer.m_nFarPort, this);
	m_ullGameServerConnTick = GetTickCount64();

	if (m_dwGameServerConn == 0)
		DAppLog::Instance()->Info(TRUE, "连接游戏服失败,正在尝试重连");
	else {
		DAppLog::Instance()->Info(TRUE, "游戏服连接成功");
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
		AfxMessageBox(_T("获取当前程序路径错误，可能路径包含特殊字符"));
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