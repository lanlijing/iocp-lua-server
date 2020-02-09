// LuaDebugDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "geneserver.h"
#include "LuaDebugDlg.h"
#include "DNetMsgBase.h"
#include "DNetMsg.h"
#include "DBuf.h"
#include "GeneServerThread.h"
#include "afxdialogex.h"


// LuaDebugDlg 对话框

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


// LuaDebugDlg 消息处理程序


BOOL LuaDebugDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
//		case VK_RETURN://屏蔽回车
//			return TRUE;
		case VK_ESCAPE://屏蔽Esc
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

	// TODO:  在此添加额外的初始化
	m_brushDlgBg.CreateSolidBrush(RGB(0,255,0));
	m_brushEditBg.CreateSolidBrush(RGB(196, 196, 196));

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}


HBRUSH LuaDebugDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_DLG)								//对话框颜色  
		hbr = (HBRUSH)m_brushDlgBg;										
	else if (nCtlColor == CTLCOLOR_EDIT)    //文本编辑框颜色  
	{
		pDC->SetTextColor(RGB(0, 0, 255));				//文字颜色
		pDC->SetBkMode(TRANSPARENT);					//透明模式
		hbr = (HBRUSH)m_brushEditBg;
	}

	return hbr;
}


void LuaDebugDlg::OnEnChangeEdtluainput()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	Invalidate(FALSE);
}
