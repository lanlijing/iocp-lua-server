
// geneserver.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CgeneserverApp: 
// �йش����ʵ�֣������ geneserver.cpp
//

class CgeneserverApp : public CWinApp
{
public:
	CgeneserverApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CgeneserverApp theApp;