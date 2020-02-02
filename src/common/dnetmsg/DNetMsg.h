#ifndef _DNETMSG_H_
#define _DNETMSG_H_

#pragma once

#include "DNetMsgBase.h"

/*
������Ϣ����
Ҫ����������Ҫ����һ�£�
�����C++
�����LUA
�ͻ���LUA
*/
struct NetMsgID
{
	enum
	{
		//ϵͳ�ڲ���Ϣ
		SYSTEM_MSG_BEGIN = 1,
		SYSTEM_MSG_CLOSE = 2, //����ر�
		SYSTEM_MSG_LUADEBUG = 3, //lua����		
		SYSTEM_MSG_END = 99,

		//�����ǿͻ��˺ͷ���������
		CLIENT_SERVER_BEGIN = 1001,

		//��������ID���
		PROXY_CL_SETNETID = 1002, //���ط���ͻ�����������ID
		CL_PROXY_TIMER = 1003, //�ͻ��������ط���������
		CL_PROXY_SERVERTIME = 1004, // �ͻ�������������ʱ��
		PROXY_CL_SERVERTIME = 1005,	// �������ͻ�������ʱ��
		//��½���
		CL_SERVER_LOGIN = 1011, //�ͻ��������½
		SERVER_CL_LOGIN = 1012, //����˷��ص�½
		CL_SERVER_REGIST = 1013, //�ͻ�������ע��
		SERVER_CL_REGIST = 1014, //����˷���ע��
		CL_SERVER_REQBASEINFO = 1015,	// �ͻ���������һ�����Ϣ
		SERVER_CL_REQBASEINFO = 1016,	// ����˷�����һ�����Ϣ
		CL_SERVER_REQRANDOMNICKNAME = 1017,		// �ͻ��������������
		SERVER_CL_REQRANDOMNICKNAME = 1018,		//����˷����������
		//��½����

		// PVEս����Ϣ���
		CL_SERVER_REQFIGHT = 1101,			// �ͻ�������������ս��
		SERVER_CL_REQFIGHT = 1102,			// �������ͻ��˷���ս������
		CL_SERVER_FIGHTREADY = 1103,		// �ͻ��˸��߷����ս����ʼ
		SERVER_CL_FIGHTBEGIH = 1104,		// ����˸��߿ͻ���ս����ʼ
		CL_SERVER_SETFIGHTIMME = 1105,		// �ͻ��˸��߷�����Զ�ս��
		SERVER_CL_SETCANINSIZE = 1106,		// ����˸��߿ͻ��˵����ɽ��������С(0~5)
		SERVER_CL_PVEFIGHTEND = 1107,		// �������ͻ��˷���PVEս������
		CL_SERVER_REQCARDINFIGHT = 1108,	// �ͻ��������������һ�����Ʒ���ս��
		SERVER_CL_CARDINFIGHT = 1109,		// �������ͻ��˷��ؿ��Ʒ���ս������
		SERVER_CL_CARDOUTFIGHT = 1110,		// ����˸��߿ͻ���ĳ�����˳�ս��(����)
		SERVER_CL_NORMALATTACK = 1111,		// ����˸��߿ͻ���ĳ������ͨ����
		// PVEս����Ϣ����

		// ������Ϣ���
		CL_SERVER_WORLDCHAT = 1201,				// �ͻ���������������������
		SERVER_CL_WORLDCHAT = 1202,				// �������ͻ��˷�����������
		CL_SERVER_PRIVATECHAT = 1203,			// �ͻ�������������˽��
		SERVER_CL_PRIVATECHAT = 1204,			// �������ͻ��˷���˽��
		// ������Ϣ����

		CLIENT_SERVER_END = 9999,

		//�����Ƿ����֮�����Ϣ
		SERVER_SERVER_BEGIN = 10001,
		PROXY_GAMESVR_CONNECT = 10002,			// ���ط�������Ϸ��
		PROXY_DBSVR_CONNECT = 10003,			// ���ط��������ݿ��
		GAMESVR_DBSVR_CONNECT = 10004,				// ��Ϸ������DB��

		//�ͻ���������
		CLIENT_ONOFF_BEGIN = 10100,
		PROXY_GAMESVR_CLIENTOFF = 10101,		// ���ط�������Ϸ��ĳ�ͻ�������
		GAMESVR_DBSVR_CLIENTOFF = 10102,		// ��Ϸ������DB��ĳ�������
		GAMESVR_PROXY_CLOSECLIENT = 10103,		// ��Ϸ�������ط��ر�ĳ���ͻ���
		CLIENT_ONOFF_END = 10199,
		// �ͻ��������߽���

		SERVER_SERVER_END = 19999,
	};
};


#endif