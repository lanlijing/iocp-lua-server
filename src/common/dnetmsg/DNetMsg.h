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
		CL_GS_BEGIN = 1001,
		CL_GS_END = 9999,
	};
};


#endif