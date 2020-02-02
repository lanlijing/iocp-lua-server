#ifndef _GENESERVERTHREAD_H_
#define _GENESERVERTHREAD_H_

#pragma once

#include "DAppThread.h"
#include "DNetWorkBase.h"

#define LUATESTNAME				"LuaTest"
#define TESTCLIENTNAME			"TestClient"
#define GAMESERVERNAME			"GameServer"

struct GeneServerType
{
	enum
	{
		eLuaTest = 0,
		eTestClient = 1,
		eGameServer = 2,
	};
};

class CgeneserverDlg;
class GeneServerThread : public DAppThread
{
private:
	GeneServerThread();
	~GeneServerThread();

public:	
	static GeneServerThread* Instance();

	void SetType(BYTE byType){ m_byType = byType; };
	BYTE GetType(){ return m_byType;};
	string GetTypeName();

public:
	BOOL StartServer(CgeneserverDlg* pMainDlg);
	void StopServer();
	void ReloadLua(){ m_bReloadLua = TRUE; };

public:			//lua ��غ���
	void AddLog(const char* szLog);
	void SetCaption(const char* szCaption);
	int SendDBufToGameServer(DBuf* pSendDBuf);		//����������TestClientʱ��Ч
	int SendDBufToClient(DWORD dwLinkerId, DBuf* pSendDBuf);			// ��������ΪGameServerʱ��Ч
	
public:
	virtual void ProcessMsg(DBuf* pMsgBuf);	//������Ϣ������Ϣ���ͻ�����Ҫ�̳�
	virtual void ProcessLogic();			//�����߼���������Ҫ����

private:
	void GameServerReConn();					// ��Ϸ����������,����eTestClient�õ�
	
private:
	string GetConfigFileName(string& strConfig);
	void ChangeCurDir();

private:	
	CgeneserverDlg* m_pMainDlg;
	string m_strCommCaption;					// �����ļ��ж����ͨ��caption
	BOOL m_bLuaInit;							// �Ƿ��Ѿ�������lua
	BOOL m_bReloadLua;							// �Ƿ�����LUA�ű�
	string m_strLuaFile;						// �ű��ļ�·��
	
	BYTE m_byType;								//���͡���ӦGeneServerTypeö��
	USHORT m_ushServerPort;						//��Ϊ�������ļ����˿�
	DWORD m_dwGameServerConn;					//����Ϸ�������ӣ�����eTestClient�õ�	
	stAddrInfo m_addrGameServer;				//��Ϸ����ַ��Ϣ������eTestClient�õ�
	
	ULONGLONG m_ullGameServerConnTick;
};

#endif