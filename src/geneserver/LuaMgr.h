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
	static BOOL CallLuaOnProxySvrMsg(DBuf* pRecvDBuf);					// �յ����������ط�����Ϣ
	static BOOL CallLuaOnDBSvrMsg(DBuf* pRecvDBuf);						// �յ�������DB������Ϣ
	static BOOL CallLuaOnGameSvrMsg(DBuf* pRecvDBuf);					// ������Ϸ������Ϣ
	static BOOL CallLuaOnGameSvrConnect();								// ��Ϸ��������
	static BOOL CallLuaOnGameSvrDisConnect();							// ��Ϸ���Ͽ�
	static BOOL CallLuaOnClientMsg(DBuf* pRecvDBuf);						// ���Կͻ��˵���Ϣ
	static BOOL CallLuaOnClientConnect(DWORD dwLinkerid);					// �ͻ���������
	static BOOL CallLuaOnClientDisConnect(DWORD dwLinkerid);				// �ͻ��˹ر�
	
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

	static int SendDBufToProxySvr(lua_State* L);
	static int SendDBufToGameSvr(lua_State* L);
	static int SendDBufToDBSvr(lua_State* L);
	
	// ����
	static void ShowCallLuaError(lua_State* L, const char* szFuncName);
};

#endif