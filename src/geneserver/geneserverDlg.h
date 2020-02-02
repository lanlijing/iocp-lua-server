
// geneserverDlg.h : 头文件
//

#pragma once

#include "DAppLog.h"
#include "LuaDebugDlg.h"
#include "afxwin.h"

// CgeneserverDlg 对话框
class CgeneserverDlg : public CDialogEx
{
// 构造
public:
	CgeneserverDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_GENESERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	void AddLog(const char* szLog);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedBtluadebug();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedBtclearlog();
	afx_msg LRESULT OnApplogInfo(WPARAM wParam, LPARAM lParam);

protected:
	LuaDebugDlg* m_pLuaDebugDlg;
	CBrush m_brushDlgBg;			//背景色
	CBrush m_brushEditBg;			//EDIT控件背景色	
	int	m_nMaxLogLen;				//EDIT控件最大LOG字数
public:
	CButton m_chkShowLog;
	CEdit m_edtLog;
	afx_msg void OnBnClickedBtreloadlua();
};
