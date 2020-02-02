// ServerTypeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "geneserver.h"
#include "ServerTypeDlg.h"
#include "afxdialogex.h"


// ServerTypeDlg 对话框

IMPLEMENT_DYNAMIC(ServerTypeDlg, CDialogEx)

ServerTypeDlg::ServerTypeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(ServerTypeDlg::IDD, pParent)
	, m_nServerType(0)
{

}

ServerTypeDlg::~ServerTypeDlg()
{
}

void ServerTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_CMBTYPE, m_nServerType);
}

