
// stdafx.cpp : ֻ������׼�����ļ���Դ�ļ�
// geneserver.pch ����ΪԤ����ͷ
// stdafx.obj ������Ԥ����������Ϣ

#include "stdafx.h"

//��Ҫ������LIB�ļ����ڴ�
#ifdef _DEBUG

#pragma comment (lib, "dbased.lib")
#pragma comment (lib, "diocpd.lib")
#pragma comment (lib, "lua51d.lib")

#else

#pragma comment (lib, "dbase.lib")
#pragma comment (lib, "diocp.lib")
#pragma comment (lib, "lua51.lib")

#endif


