
// geneserver.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "geneserver.h"
#include "geneserverDlg.h"
#include "ServerTypeDlg.h"
#include "GeneServerThread.h"
#include "Exception.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CgeneserverApp

BEGIN_MESSAGE_MAP(CgeneserverApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CgeneserverApp ����

CgeneserverApp::CgeneserverApp()
{
	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO:  �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CgeneserverApp ����

CgeneserverApp theApp;


// CgeneserverApp ��ʼ��

BOOL CgeneserverApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()��  ���򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	SetThreadLocale(GetUserDefaultLCID());		//������Ӣ��ϵͳ�³�������,��Ϊֻ�ڱ��߳����ã����ж���߳�Ҫ����
	AfxEnableControlContainer();
	InitExceptionHandler();

	// ���� shell ���������Է��Ի������
	// �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
	CShellManager *pShellManager = new CShellManager;

	// ���Windows Native���Ӿ����������Ա��� MFC �ؼ�����������
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO:  Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	// SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	//����Ϊ��ȡ�����в���
	CString strCmdLine = m_lpCmdLine;
	if (strCmdLine.Find(_T(LUATESTNAME)) != -1)
		GeneServerThread::Instance()->SetType(GeneServerType::eLuaTest);
	else if (strCmdLine.Find(_T(TESTCLIENTNAME)) != -1)
		GeneServerThread::Instance()->SetType(GeneServerType::eTestClient);
	else if (strCmdLine.Find(_T(GAMESERVERNAME)) != -1)
		GeneServerThread::Instance()->SetType(GeneServerType::eGameServer);
	else
	{
		ServerTypeDlg typeDlg;
		if (typeDlg.DoModal() == IDOK)
		{
			GeneServerThread::Instance()->SetType(typeDlg.m_nServerType);
		}
		else
			return FALSE;
	}

	//
	CgeneserverDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO:  �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO:  �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "����: �Ի��򴴽�ʧ�ܣ�Ӧ�ó���������ֹ��\n");
		TRACE(traceAppMsg, 0, "����: ������ڶԻ�����ʹ�� MFC �ؼ������޷� #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS��\n");
	}

	// ɾ�����洴���� shell ��������
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}

