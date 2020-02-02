#ifndef _DNETWORKBASE_H_
#define _DNETWORKBASE_H_

#pragma once

#include <winsock2.h>

#define NETIOBUFLEN				4096					//ÿ����ʵ���ͺͽ��յ�����IO����󳤶ȡ�����ǳ�����������Ϣ��Ҫ�ֶ�η���
#define MAXSENDWSABUF			32						//WSASENDʱÿ����෢�͵Ļ���������
#define CLIENTCONNSTARTID		200000					//�ͻ������ӵ���ʼID(���õñȽϴ��Ǳ�֤������˿��غ�)
#define SERVERLINKERSTARTID		100000					//�����LINKER�ĳ�ʼID(���õñȽϴ��Ǳ�֤������˿��غ�)
#define LINKERDESTROYWAITTIME	15000					//linker��������״̬���ٹ��೤ʱ����������
#define THREADWAITOBJECTTIME	2500				//�˳�ʱ�����߳̾��WaitForSingleObject��ʱ��

struct LinkerCloseReason					//�ͻ������ӵĹر�����
{
	enum
	{
		eCloseFromApp,						//Ӧ�ò�ر�
		eCloseFromClient,					//�ͻ��˹ر�
		eCloseFromProgramError,				//�������ر�
		eCloseFromClientError,				//�ͻ��˷Ƿ���Ϣ
	};
};

struct stAddrInfo
{
	string	m_strAddr;					//Զ�˵�ַ������������
	string 	m_strIP;					//��ΪSERVER�˵�linker�����ǿͻ��˵ĵ�ַ����Ϊ�ͻ������ӣ����Ƿ���˵�ַ(������IP�����������Ҫת��IP)
	int		m_nFarPort;					//��Ϊ�ͻ��ˣ��������ӵ�Զ�̷������Ķ˿ڡ���Ϊ�������ϵͳ���ɵĶ˿�

	stAddrInfo()
	{
		Init();
	}
	void Init()
	{
		m_strAddr.clear();
		m_strIP.clear();
		m_nFarPort = 0;
	}
	stAddrInfo(const stAddrInfo& src)
	{
		m_strAddr = src.m_strAddr;
		m_strIP = src.m_strIP;
		m_nFarPort = src.m_nFarPort;
	}
	~stAddrInfo()
	{
	}

	BOOL IsActive()		//�Ƿ��Ѿ���Ч
	{
		return (m_strIP.length() > 7);	//IP��ַ��С����Ϊ7
	}

	stAddrInfo& operator=(const stAddrInfo& src)
	{
		m_strAddr = src.m_strAddr;
		m_strIP = src.m_strIP;
		m_nFarPort = src.m_nFarPort;

		return *this;
	}
};

class DBuf;
class DAppThread;
class DIOCPServer;
class DClientConn;
class DNetWorkBase
{
private:
	typedef map<USHORT,	DIOCPServer*> IOCPServerMap;
	typedef map<USHORT, DIOCPServer*>::iterator IOCPServerMapIter;
	typedef map<DWORD,	DClientConn*> ClientConnMap;
	typedef map<DWORD,	DClientConn*>::iterator ClientConnMapIter;

private:
	DNetWorkBase();
	~DNetWorkBase();

public:	
	static DNetWorkBase* Instance();
	void CleanAllServerClient();

	BOOL SocketStart();
	BOOL SocketCleanup();

	/*
	LinkerSend  һ����ServerLinker��100��
	����Ǵ��ͻ���,ServerLinker����Ϊ��С��������16
	����Ǵ������ ClientConn����Ϊ��С��������16��
	*/
	void InitNetObjectPool(int nClientConnIncSize, int nServerLinkerIncSize, int nLinkerSendIncSize);
	void CleanUpNetObjectPool();

	USHORT CreateIOCPServer(USHORT ushPort, DAppThread* pAppThread);						//����紴��һ�������������Ľӿڣ�����Ϊ��Ҫ�����Ķ˿ڡ�����ɹ������ض˿ں���Ϊ��ʶ��ʧ�ܣ��򷵻�0
	void CloseIOCPServer(USHORT ushPort);							//�����Ǵ���ʱ���صĶ˿ں�
	BOOL GetALinkerAddr(USHORT ushPort, DWORD dwLinkerID, stAddrInfo& addrInfo);				// �õ�һ�����ӵ�IP
	void CloseALinker(USHORT ushPort, DWORD dwID, BYTE byReason, BOOL bTellApp = TRUE);		//�ر�ĳ����������ĳ�����Կͻ��˵�����
	void SendNetMsg(USHORT ushPort, DWORD dwID, const char* szMsg, int nMsgLen);				//��ĳ����������ĳ�����Կͻ��˵����ӷ�����Ϣ

	DWORD CreateClientConn(const char* szFarAddr, USHORT ushPort, DAppThread* pAppThread);		//�����һ����������Զ�̷������Ľӿڣ��ɹ����ظ����ӵ��ڲ�ID��Ϊ��ʶ��ʧ���򷵻�0
	void CloseClientConn(DWORD dwID);
	int SendConnMsg(DWORD dwID,DBuf* pDBuf);							// ����0Ϊ�ɹ�							
	int SendConnMsg(DWORD dwID,const char* pSendMsg, int nMsgLen);		// ����0Ϊ�ɹ�
	
private:	
	IOCPServerMap m_mapIOCPServer;
	ClientConnMap m_mapClientConnMap;
};

#endif