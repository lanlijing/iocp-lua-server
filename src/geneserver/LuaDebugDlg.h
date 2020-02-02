#pragma once
#include "afxwin.h"


// LuaDebugDlg �Ի���

class LuaDebugDlg : public CDialogEx
{
	DECLARE_DYNAMIC(LuaDebugDlg)

public:
	LuaDebugDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~LuaDebugDlg();

// �Ի�������
	enum { IDD = IDD_DLGLUADEBUG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtexecute();
	afx_msg void OnBnClickedBtclose();

protected:
	CBrush m_brushDlgBg;			//����ɫ
	CBrush m_brushEditBg;			//EDIT�ؼ�����ɫ

public:
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	CEdit m_edtScript;
};
