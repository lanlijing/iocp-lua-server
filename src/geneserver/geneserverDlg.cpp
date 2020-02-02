
// geneserverDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "geneserver.h"
#include "DNetMsgBase.h"
#include "DNetMsg.h"
#include "DBuf.h"
#include "GeneServerThread.h"
#include "geneserverDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CgeneserverDlg �Ի���



CgeneserverDlg::CgeneserverDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CgeneserverDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pLuaDebugDlg = nullptr;
	m_nMaxLogLen = 30000;
}

void CgeneserverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHKSHOWLOG, m_chkShowLog);
	DDX_Control(pDX, IDC_EDIT1, m_edtLog);
}

BEGIN_MESSAGE_MAP(CgeneserverDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTLUADEBUG, &CgeneserverDlg::OnBnClickedBtluadebug)
	ON_WM_SHOWWINDOW()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTCLEARLOG, &CgeneserverDlg::OnBnClickedBtclearlog)
	ON_MESSAGE(WM_APPLOGINFO, &CgeneserverDlg::OnApplogInfo)
	ON_BN_CLICKED(IDC_BTRELOADLUA, &CgeneserverDlg::OnBnClickedBtreloadlua)
END_MESSAGE_MAP()


// CgeneserverDlg ��Ϣ�������

BOOL CgeneserverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	m_brushDlgBg.CreateSolidBrush(RGB(0,127,255));
	m_brushEditBg.CreateSolidBrush(RGB(196,196,196));

	m_pLuaDebugDlg = new LuaDebugDlg();
	m_pLuaDebugDlg->Create(IDD_DLGLUADEBUG,this);

	m_chkShowLog.SetCheck(1);

	if (GeneServerThread::Instance()->StartServer(this) == FALSE)
	{
		OnCancel();
		return FALSE;
	}
		
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CgeneserverDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		if (nID == SC_CLOSE)
		{
			if (MessageBoxA(GetSafeHwnd(), "���Ƿ�ȷ�Ϲرմ˳���", "�ر�", MB_OKCANCEL) == IDCANCEL)
				return;
			
			DBuf* pBuf = DBuf::TakeNewDBuf();
			pBuf->m_dwBufType = DBufType::eAppDispatchMsg;
			pBuf->m_dwBufFrom = 0;
			pBuf->BeginNetDBuf(NET_MSG_TAG, 0, NetMsgID::SYSTEM_MSG_CLOSE);
			pBuf->EndNetDBuf();

			GeneServerThread::Instance()->PushMsgToList(pBuf, TRUE);
		}
		else
			CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CgeneserverDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CgeneserverDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CgeneserverDlg::AddLog(const char* szLog)
{
	if (m_chkShowLog.GetCheck() == 0)
		return;

	int nCurLogLen = m_edtLog.GetWindowTextLengthW();
	if (nCurLogLen >= m_nMaxLogLen)
	{
		m_edtLog.SetWindowTextW(_T(""));
		nCurLogLen = 0;
	}

	USES_CONVERSION;
	CString strTemp = A2W(szLog);
	strTemp = strTemp + _T("\r\n");

	m_edtLog.SetSel(nCurLogLen, nCurLogLen);
	m_edtLog.ReplaceSel(strTemp);
}

BOOL CgeneserverDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN://���λس�
			return TRUE;
		case VK_ESCAPE://����Esc
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


BOOL CgeneserverDlg::DestroyWindow()
{
	GeneServerThread::Instance()->StopServer();
	if (m_pLuaDebugDlg)
	{
		m_pLuaDebugDlg->DestroyWindow();
		delete m_pLuaDebugDlg;
		m_pLuaDebugDlg = nullptr;
	}

	return CDialogEx::DestroyWindow();
}


void CgeneserverDlg::OnBnClickedBtluadebug()
{
	m_pLuaDebugDlg->ShowWindow(SW_SHOW);
}

void CgeneserverDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	CRect rcMain(0, 0, 0, 0);
	CRect rcSet(0, 0, 0, 0);
	GetWindowRect(&rcMain);
	m_pLuaDebugDlg->GetWindowRect(&rcSet);
	m_pLuaDebugDlg->MoveWindow(rcMain.left + 50, rcMain.top + 50, rcSet.Width(), rcSet.Height(), FALSE);
}


HBRUSH CgeneserverDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_DLG)								//�Ի�����ɫ  
		hbr = (HBRUSH)m_brushDlgBg;										
	else if ((nCtlColor == CTLCOLOR_STATIC) && (pWnd->GetDlgCtrlID() == IDC_CHKSHOWLOG))
	{
		pDC->SetBkMode(TRANSPARENT);					//͸��ģʽ
		hbr = (HBRUSH)m_brushDlgBg;
	}
	else if (nCtlColor == CTLCOLOR_EDIT)    //�ı��༭����ɫ  
	{
		pDC->SetTextColor(RGB(0, 0, 255));				//������ɫ
		pDC->SetBkMode(TRANSPARENT);					//͸��ģʽ
		hbr = (HBRUSH)m_brushEditBg;
	}

	return hbr;
}

void CgeneserverDlg::OnBnClickedBtclearlog()
{
	m_edtLog.SetWindowTextW(_T(""));
}

afx_msg LRESULT CgeneserverDlg::OnApplogInfo(WPARAM wParam, LPARAM lParam)
{
	char* szLog = (char*)wParam;
	AddLog(szLog);

	return 0;
}

void CgeneserverDlg::OnBnClickedBtreloadlua()
{
	GeneServerThread::Instance()->ReloadLua();
}
