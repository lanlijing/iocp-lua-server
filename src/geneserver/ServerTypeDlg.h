#pragma once


// ServerTypeDlg �Ի���

class ServerTypeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ServerTypeDlg)

public:
	ServerTypeDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~ServerTypeDlg();

// �Ի�������
	enum { IDD = IDD_DLGTYPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	int m_nServerType;
};
