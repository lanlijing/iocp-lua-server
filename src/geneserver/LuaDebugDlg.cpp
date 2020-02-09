// LuaDebugDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "geneserver.h"
#include "LuaDebugDlg.h"
#include "DNetMsgBase.h"
#include "DNetMsg.h"
#include "DBuf.h"
#include "GeneServerThread.h"
#include "afxdialogex.h"


// LuaDebugDlg �Ի���

IMPLEMENT_DYNAMIC(LuaDebugDlg, CDialogEx)

LuaDebugDlg::LuaDebugDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(LuaDebugDlg::IDD, pParent)
{

}

LuaDebugDlg::~LuaDebugDlg()
{
}

void LuaDebugDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDTLUAINPUT, m_edtScript);
}


BEGIN_MESSAGE_MAP(LuaDebugDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTEXECUTE, &LuaDebugDlg::OnBnClickedBtexecute)
	ON_BN_CLICKED(IDC_BTCLOSE, &LuaDebugDlg::OnBnClickedBtclose)
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE(IDC_EDTLUAINPUT, &LuaDebugDlg::OnEnChangeEdtluainput)
END_MESSAGE_MAP()


// LuaDebugDlg ��Ϣ�������


BOOL LuaDebugDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
//		case VK_RETURN://���λس�
//			return TRUE;
		case VK_ESCAPE://����Esc
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void LuaDebugDlg::OnBnClickedBtexecute()
{
	CString cstrText = _T("");
	m_edtScript.GetWindowTextW(cstrText);
	if (cstrText.GetLength() == 0)
		return;

	USES_CONVERSION;
	string strScript = W2A(cstrText);
	//
	DBuf* pBuf = DBuf::TakeNewDBuf();
	pBuf->m_dwBufType = DBufType::eAppDispatchMsg;
	pBuf->m_dwBufFrom = 0;
	pBuf->BeginNetDBuf(NET_MSG_TAG, 0, NetMsgID::SYSTEM_MSG_LUADEBUG);
	pBuf->WriteString(strScript);
	pBuf->EndNetDBuf();

	GeneServerThread::Instance()->PushMsgToList(pBuf, TRUE);
}


void LuaDebugDlg::OnBnClickedBtclose()
{
	OnOK();
}


BOOL LuaDebugDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	m_brushDlgBg.CreateSolidBrush(RGB(0,255,0));
	m_brushEditBg.CreateSolidBrush(RGB(196, 196, 196));

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}


HBRUSH LuaDebugDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_DLG)								//�Ի�����ɫ  
		hbr = (HBRUSH)m_brushDlgBg;										
	else if (nCtlColor == CTLCOLOR_EDIT)    //�ı��༭����ɫ  
	{
		pDC->SetTextColor(RGB(0, 0, 255));				//������ɫ
		pDC->SetBkMode(TRANSPARENT);					//͸��ģʽ
		hbr = (HBRUSH)m_brushEditBg;
	}

	return hbr;
}


void LuaDebugDlg::OnEnChangeEdtluainput()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	Invalidate(FALSE);
}
