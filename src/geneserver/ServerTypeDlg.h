#pragma once


// ServerTypeDlg 对话框

class ServerTypeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ServerTypeDlg)

public:
	ServerTypeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~ServerTypeDlg();

// 对话框数据
	enum { IDD = IDD_DLGTYPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	int m_nServerType;
};
