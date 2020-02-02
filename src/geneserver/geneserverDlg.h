
// geneserverDlg.h : ͷ�ļ�
//

#pragma once

#include "DAppLog.h"
#include "LuaDebugDlg.h"
#include "afxwin.h"

// CgeneserverDlg �Ի���
class CgeneserverDlg : public CDialogEx
{
// ����
public:
	CgeneserverDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_GENESERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
	CBrush m_brushDlgBg;			//����ɫ
	CBrush m_brushEditBg;			//EDIT�ؼ�����ɫ	
	int	m_nMaxLogLen;				//EDIT�ؼ����LOG����
public:
	CButton m_chkShowLog;
	CEdit m_edtLog;
	afx_msg void OnBnClickedBtreloadlua();
};
