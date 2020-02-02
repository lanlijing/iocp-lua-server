#ifndef _DNETWORKBASE_H_
#define _DNETWORKBASE_H_

#pragma once

#include <winsock2.h>

#define NETIOBUFLEN				4096					//每次真实发送和接收的网络IO的最大长度。如果是超长的网络消息，要分多次发送
#define MAXSENDWSABUF			32						//WSASEND时每次最多发送的缓冲区数量
#define CLIENTCONNSTARTID		200000					//客户端连接的起始ID(设置得比较大是保证不会与端口重合)
#define SERVERLINKERSTARTID		100000					//服务端LINKER的超始ID(设置得比较大是保证不会与端口重合)
#define LINKERDESTROYWAITTIME	15000					//linker设置销毁状态后再过多长时间真正销毁
#define THREADWAITOBJECTTIME	2500				//退出时，对线程句柄WaitForSingleObject的时间

struct LinkerCloseReason					//客户端连接的关闭类型
{
	enum
	{
		eCloseFromApp,						//应用层关闭
		eCloseFromClient,					//客户端关闭
		eCloseFromProgramError,				//程序错误关闭
		eCloseFromClientError,				//客户端非法消息
	};
};

struct stAddrInfo
{
	string	m_strAddr;					//远端地址，可能是域名
	string 	m_strIP;					//作为SERVER端的linker，这是客户端的地址。作为客户端连接，这是服务端地址(这里是IP，如果是域名要转成IP)
	int		m_nFarPort;					//作为客户端，是所连接的远程服务器的端口。作为服务端是系统生成的端口

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

	BOOL IsActive()		//是否已经有效
	{
		return (m_strIP.length() > 7);	//IP地址最小长度为7
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
	LinkerSend  一般是ServerLinker的100倍
	如果是纯客户端,ServerLinker可设为极小的数。如16
	如果是纯服务端 ClientConn可设为极小的数，如16。
	*/
	void InitNetObjectPool(int nClientConnIncSize, int nServerLinkerIncSize, int nLinkerSendIncSize);
	void CleanUpNetObjectPool();

	USHORT CreateIOCPServer(USHORT ushPort, DAppThread* pAppThread);						//给外界创建一个监听服务器的接口，参数为需要创建的端口。如果成功，返回端口号作为标识，失败，则返回0
	void CloseIOCPServer(USHORT ushPort);							//参数是创建时返回的端口号
	BOOL GetALinkerAddr(USHORT ushPort, DWORD dwLinkerID, stAddrInfo& addrInfo);				// 得到一个连接的IP
	void CloseALinker(USHORT ushPort, DWORD dwID, BYTE byReason, BOOL bTellApp = TRUE);		//关闭某个服务器上某个来自客户端的链接
	void SendNetMsg(USHORT ushPort, DWORD dwID, const char* szMsg, int nMsgLen);				//向某个服务器上某个来自客户端的链接发送消息

	DWORD CreateClientConn(const char* szFarAddr, USHORT ushPort, DAppThread* pAppThread);		//给外界一个调用连接远程服务器的接口，成功返回该连接的内部ID作为标识，失败则返回0
	void CloseClientConn(DWORD dwID);
	int SendConnMsg(DWORD dwID,DBuf* pDBuf);							// 返回0为成功							
	int SendConnMsg(DWORD dwID,const char* pSendMsg, int nMsgLen);		// 返回0为成功
	
private:	
	IOCPServerMap m_mapIOCPServer;
	ClientConnMap m_mapClientConnMap;
};

#endif