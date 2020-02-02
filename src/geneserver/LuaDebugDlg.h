#pragma once
#include "afxwin.h"


// LuaDebugDlg 对话框

class LuaDebugDlg : public CDialogEx
{
	DECLARE_DYNAMIC(LuaDebugDlg)

public:
	LuaDebugDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~LuaDebugDlg();

// 对话框数据
	enum { IDD = IDD_DLGLUADEBUG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtexecute();
	afx_msg void OnBnClickedBtclose();

protected:
	CBrush m_brushDlgBg;			//背景色
	CBrush m_brushEditBg;			//EDIT控件背景色

public:
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	CEdit m_edtScript;
};
