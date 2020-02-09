#ifndef _LUAMGR_H_
#define _LUAMGR_H_

#pragma once

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class DBuf;
class LuaMgr
{
private:
	LuaMgr();
	~LuaMgr();

public:
	static LuaMgr* Instance();
	void ReLoadLua();

	BOOL DoString(const char* szScript);
	BOOL DoFile(const char* szFile);

	lua_State* GetLState(){ return m_L; };
	DBuf* GetRecvDBuf(){ return m_pRecvDBuf; };
	DBuf* GetSendDBuf(){ return m_pSendDBuf; };
		
private:
	lua_State* m_L;

	DBuf* m_pRecvDBuf;
	DBuf* m_pSendDBuf;
};

class LuaFunc
{
public:
	// c++ ���� lua
	static BOOL CallLuaOnGameInit();
	static BOOL CallLuaOnGameExit();
	static BOOL CallLuaOnGameFrame();
	static BOOL CallLuaOnGameSvrMsg(DBuf* pRecvDBuf);					// ������Ϸ������Ϣ,������eTestClient
	static BOOL CallLuaOnGameSvrConnect();								// ��Ϸ��������,������eTestClient
	static BOOL CallLuaOnGameSvrDisConnect();							// ��Ϸ���Ͽ�,������eTestClient
	static BOOL CallLuaOnClientMsg(DBuf* pRecvDBuf, DWORD dwLinkerid);						// ���Կͻ��˵���Ϣ,������eGameServer
	static BOOL CallLuaOnClientConnect(DWORD dwLinkerid);					// �ͻ���������,������eGameServer
	static BOOL CallLuaOnClientDisConnect(DWORD dwLinkerid);				// �ͻ��˹ر�,������eGameServer
	
	// lua ���� C++
	static int GetTickCount(lua_State* L);
	static int SetCaption(lua_State* L);
	static int ShowMsg(lua_State* L);

	static int SetRecvDBufPos(lua_State* L);
	static int ReadRecvDBufByte(lua_State* L);
	static int ReadRecvDBufShort(lua_State* L);
	static int ReadRecvDBufInt(lua_State* L);
	static int ReadRecvDBufUINT32(lua_State* L);
	static int ReadRecvDBufDouble(lua_State* L);
	static int ReadRecvDBufString(lua_State* L);
	static int WriteSendDBufByte(lua_State* L);
	static int WriteSendDBufShort(lua_State* L);
	static int WriteSendDBufInt(lua_State* L);
	static int WriteSendDBufUint32(lua_State* L);
	static int WriteSendDBufDouble(lua_State* L);
	static int WriteSendDBufString(lua_State* L);
	static int BeginSendDBuf(lua_State* L);
	static int EndSendDBuf(lua_State* L);
	static int CloneRecvDBufToSend(lua_State* L);

	static int SendDBufToGameSvr(lua_State* L);		// ������eTestClient
	static int SendDBufToClient(lua_State* L);		// ������eGameServer
	
	// ����
	static void ShowCallLuaError(lua_State* L, const char* szFuncName);
};

#endif