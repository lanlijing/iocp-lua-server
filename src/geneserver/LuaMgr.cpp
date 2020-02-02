#include "stdafx.h"
#include "desam.h"
#include "DMemoryPool.h"
#include "DBuf.h"
#include "DNetMsgBase.h"
#include "GeneServerThread.h"
#include "StrConvert.h"
#include "LuaMgr.h"

static const struct luaL_reg Buflib[] =
{	
	{ "getTickCount", LuaFunc::GetTickCount },
	{ "setCaption", LuaFunc::SetCaption },
	{ "showMsg", LuaFunc::ShowMsg },
	{ "setReadPos", LuaFunc::SetRecvDBufPos },
	{ "readRecvByte", LuaFunc::ReadRecvDBufByte },
	{ "readRecvShort", LuaFunc::ReadRecvDBufShort },
	{ "readRecvInt", LuaFunc::ReadRecvDBufInt },
	{ "readRecvUint32", LuaFunc::ReadRecvDBufUINT32 },
	{ "readRecvDouble", LuaFunc::ReadRecvDBufDouble },
	{ "readRecvString", LuaFunc::ReadRecvDBufString },
	{ "writeSendByte", LuaFunc::WriteSendDBufByte },
	{ "writeSendShort", LuaFunc::WriteSendDBufShort },
	{ "writeSendInt", LuaFunc::WriteSendDBufInt },
	{ "writeSendUint32", LuaFunc::WriteSendDBufUint32 },
	{ "writeSendDouble", LuaFunc::WriteSendDBufDouble },
	{ "writeSendString", LuaFunc::WriteSendDBufString },
	{ "beginSendBuf", LuaFunc::BeginSendDBuf },
	{ "endSendBuf", LuaFunc::EndSendDBuf },
	{ "cloneRecvBufToSend", LuaFunc::CloneRecvDBufToSend },
	{ "sendProxySvrBuf", LuaFunc::SendDBufToProxySvr },
	{ "sendGameSvrBuf", LuaFunc::SendDBufToGameSvr },
	{ "sendDBSvrBuf", LuaFunc::SendDBufToDBSvr },
	{ 0, 0 }
};

// LuaMgr类
LuaMgr::LuaMgr()
{
	// 获取接收及发送DBUF
	m_pRecvDBuf = DBuf::TakeNewDBuf();
	m_pSendDBuf = DBuf::TakeNewDBuf();

	// 初始化LUA
	m_L = lua_open();
	lua_gc(m_L, LUA_GCSTOP, 0);  /* stop collector during initialization */

	luaopen_base(m_L);
	luaopen_table(m_L);
	luaopen_string(m_L);
	luaopen_math(m_L);
#ifdef _DEBUG
	luaopen_debug(m_L);
#endif
	luaL_openlibs(m_L);  /* open libraries  以下是注册给LUA脚本使用的C++函数*/
	luaL_openlib(m_L, "WSLuaCpp", Buflib, 0);

	lua_gc(m_L, LUA_GCRESTART, 0);
}

LuaMgr::~LuaMgr()
{
	lua_close(m_L);
	m_L = nullptr;
}

LuaMgr* LuaMgr::Instance()
{
	static LuaMgr s_inst;
	return &s_inst;
}

void LuaMgr::ReLoadLua()
{
	lua_close(m_L);
	m_L = nullptr;

	// 初始化LUA
	m_L = lua_open();	
	lua_gc(m_L, LUA_GCSTOP, 0);  /* stop collector during initialization */

	luaopen_base(m_L);
	luaopen_table(m_L);
	luaopen_string(m_L);
	luaopen_math(m_L);
#ifdef _DEBUG
	luaopen_debug(m_L);
#endif
	luaL_openlibs(m_L);  /* open libraries  以下是注册给LUA脚本使用的C++函数*/
	luaL_openlib(m_L, "WSLuaCpp", Buflib, 0);

	lua_gc(m_L, LUA_GCRESTART, 0);
}

BOOL LuaMgr::DoString(const char* szScript)
{
	if (luaL_dostring(m_L, szScript))
	{
		const char* szError = lua_tostring(m_L, -1);
		GeneServerThread::Instance()->AddLog(szError);
		return false;
	}
	else
		return true;
}

BOOL LuaMgr::DoFile(const char* szFile)
{
	if (luaL_dofile(m_L, szFile))
	{
		const char* szError = lua_tostring(m_L, -1);
		GeneServerThread::Instance()->AddLog(szError);
		return false;
	}
	else
		return true;
}

// 以下是LUA调用的C++接口
int LuaFunc::GetTickCount(lua_State* L)
{
	ULONG64 ullTick = GetTickCount64();
	lua_pushnumber(L, ullTick);
	return 1;
}
int LuaFunc::SetCaption(lua_State* L)
{
	const char* szCaption = luaL_checkstring(L, 1);
	string sGBK = StrConvert::u2a(szCaption);
	GeneServerThread::Instance()->SetCaption(sGBK.c_str());
	return 1;
}
int LuaFunc::ShowMsg(lua_State* L)
{
	const char* szTrace = luaL_checkstring(L, 1);
	string sGBK = StrConvert::u2a(szTrace);
	GeneServerThread::Instance()->AddLog(sGBK.c_str());
	return 1;
}

int LuaFunc::SetRecvDBufPos(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	int n = luaL_checkint(L, 1);
	p->SetCurPos(n);
	return 0;
}

int LuaFunc::ReadRecvDBufByte(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	BYTE n = 0;
	p->ReadByte(n);
	lua_pushnumber(L, n);
	return 1;
}
int LuaFunc::ReadRecvDBufShort(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	short n = 0;
	p->ReadShort(n);
	lua_pushnumber(L, n);
	return 1;
}
int LuaFunc::ReadRecvDBufInt(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	int n = 0;
	p->ReadInteger(n);
	lua_pushnumber(L, n);
	return 1;
}
int LuaFunc::ReadRecvDBufUINT32(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	UINT32 n = 0;
	p->ReadUint32(n);
	lua_pushnumber(L, n);
	return 1;
}
int LuaFunc::ReadRecvDBufDouble(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	double dbl = 0;
	p->ReadDouble(dbl);
	lua_pushnumber(L, dbl);
	return 1;
}

int LuaFunc::ReadRecvDBufString(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetRecvDBuf();
	string s = "";
	p->ReadString(s);
	lua_pushstring(L,s.c_str());
	
	return 1;
}

int LuaFunc::WriteSendDBufByte(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	BYTE by = (BYTE)luaL_checkint(L, 1);
	lua_pushboolean(L, p->WriteByte(by));
	return 1;
}
int LuaFunc::WriteSendDBufShort(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	short sht = (short)luaL_checkint(L, 1);
	lua_pushboolean(L, p->WriteShort(sht));
	return 1;
}
int LuaFunc::WriteSendDBufInt(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	int n = luaL_checkint(L, 1);
	lua_pushboolean(L, p->WriteInteger(n));
	return 1;
}
int LuaFunc::WriteSendDBufUint32(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	UINT32 un = (UINT32)luaL_checkint(L, 1);
	lua_pushboolean(L, p->WriteUint32(un));
	return 1;
}
int LuaFunc::WriteSendDBufDouble(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	double dbl = (float)luaL_checknumber(L, 1);
	lua_pushboolean(L, p->WriteDouble(dbl));
	return 1;
}

int LuaFunc::WriteSendDBufString(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	const char* pLuaStr = luaL_checkstring(L, 1);
	lua_pushboolean(L, p->WriteString(pLuaStr));
	
	return 1;
}

int LuaFunc::BeginSendDBuf(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	UINT32 unUserId = (UINT32)luaL_checkint(L, 1);
	UINT32 unMsgId = (UINT32)luaL_checkint(L, 2);
	p->BeginNetDBuf(NET_MSG_TAG,unUserId,unMsgId);
	return 0;
}

int LuaFunc::EndSendDBuf(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	p->EndNetDBuf();
	return 0;
}

int LuaFunc::CloneRecvDBufToSend(lua_State* L)
{
	DBuf* pSend = LuaMgr::Instance()->GetSendDBuf();
	DBuf* pRecv = LuaMgr::Instance()->GetRecvDBuf();
	pSend->Clone(pRecv);
	return 0;
}

int LuaFunc::SendDBufToProxySvr(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	int nRet = GeneServerThread::Instance()->SendDBufToFarServer(GeneServerType::eProxyServer, p);
	lua_pushnumber(L, nRet);
	return 1;
}

int LuaFunc::SendDBufToGameSvr(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	int nRet = GeneServerThread::Instance()->SendDBufToFarServer(GeneServerType::eGameServer, p);
	lua_pushnumber(L, nRet);
	return 1;
}

int LuaFunc::SendDBufToDBSvr(lua_State* L)
{
	DBuf* p = LuaMgr::Instance()->GetSendDBuf();
	int nRet = GeneServerThread::Instance()->SendDBufToFarServer(GeneServerType::eDBServer, p);
	lua_pushnumber(L, nRet);
	return 1;
}

BOOL LuaFunc::CallLuaOnGameInit()
{
	lua_State* L = LuaMgr::Instance()->GetLState();
	lua_getglobal(L,"OnGameInit");
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		ShowCallLuaError(L,"OnGameInit");
		return FALSE;
	}

	return TRUE;
}

BOOL LuaFunc::CallLuaOnGameExit()
{
	lua_State* L = LuaMgr::Instance()->GetLState();
	lua_getglobal(L, "OnGameExit");
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		ShowCallLuaError(L,"OnGameExit");
		return FALSE;
	}

	return TRUE;
}

BOOL LuaFunc::CallLuaOnGameFrame()
{
	lua_State* L = LuaMgr::Instance()->GetLState();
	lua_getglobal(L, "OnGameFrame");
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		ShowCallLuaError(L,"OnGameFrame");
		return FALSE;
	}

	return TRUE;
}

BOOL LuaFunc::CallLuaOnProxySvrMsg(DBuf* pRecvDBuf)
{
	lua_State* L = LuaMgr::Instance()->GetLState();
	DBuf* pDBuf = LuaMgr::Instance()->GetRecvDBuf();
	pDBuf->Attach(pRecvDBuf->GetBuf(),pRecvDBuf->GetLength());
	lua_getglobal(L, "OnProxySvrMsg");
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		ShowCallLuaError(L,"OnProxySvrMsg");
		return FALSE;
	}

	return TRUE;
}

BOOL LuaFunc::CallLuaOnDBSvrMsg(DBuf* pRecvDBuf)
{
	lua_State* L = LuaMgr::Instance()->GetLState();
	DBuf* pDBuf = LuaMgr::Instance()->GetRecvDBuf();
	pDBuf->Attach(pRecvDBuf->GetBuf(), pRecvDBuf->GetLength());
	lua_getglobal(L, "OnDBSvrMsg");
	if (lua_pcall(L, 0, 0, 0) != 0)
	{
		ShowCallLuaError(L, "OnDBSvrMsg");
		return FALSE;
	}

	return TRUE;
}

BOOL LuaFunc::CallLuaOnGameSvrMsg(DBuf* pRecvDBuf)
{
	return TRUE;
}

BOOL LuaFunc::CallLuaOnGameSvrConnect()
{
	return TRUE;
}

BOOL LuaFunc::CallLuaOnGameSvrDisConnect()
{
	return TRUE;
}

BOOL LuaFunc::CallLuaOnClientMsg(DBuf* pRecvDBuf)
{
	return TRUE;
}

BOOL LuaFunc::CallLuaOnClientConnect(DWORD dwLinkerid)
{
	return TRUE;
}

BOOL LuaFunc::CallLuaOnClientDisConnect(DWORD dwLinkerid)
{
	return TRUE;
}

static int s_nShowSameCount = 0;		// 同一错误消息，只连续打印三次
static string s_strLuaErr = "";
void LuaFunc::ShowCallLuaError(lua_State* L, const char* szFuncName)
{
	string s = "c++ call lua error.function name:";
	s += szFuncName;
	s += " ";
	s += lua_tostring(L, -1);

	if (s_strLuaErr == s)
	{
		++s_nShowSameCount;
		if (s_nShowSameCount > 3)
			return;
	}
	else
	{
		s_strLuaErr = s;
		s_nShowSameCount = 1;
	}

	GeneServerThread::Instance()->AddLog(s.c_str());
}
