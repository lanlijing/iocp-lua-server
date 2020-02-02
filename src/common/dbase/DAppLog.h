/***********************************************************************************
name:DAppLog
comment:
1��һ����־��
2�������̰߳�ȫ
3����û�в�ȡ�ȰѲ�����־�������ڴ��еĻ��ƣ�����Ϊ����������쳣�������������ڴ��е���־���ᶪʧ
***********************************************************************************/
#ifndef _DAPPLOG_H_
#define _DAPPLOG_H_

#pragma once

#include "DLockerBuff.h"

#define WM_APPLOGINFO WM_USER + 11233
#define APPLOGMAXLEN 4096

class DAppLog
{
private:
	DAppLog();
	~DAppLog();

public:	
	static DAppLog* Instance();
	BOOL SetDirPrefix(const char* szDir,const char* szPrefix);		//������־�ķ���Ŀ¼
	void Info(BOOL bViewInWnd,const char* szFormat,...);
	void AddAssertLog(const char* szFile,int nLine,const char* szFunc,const char* szContent);	//��ӡ��ĳ������������˴�
#define AssertLog(N) AddAssertLog(__FILE__,__LINE__,__FUNCTION__,N)
	void Init(const char* szDir,const char* szPrefix,HWND hWndInfo = nullptr);
	void SetLogWrite(BOOL bWrite){m_bLogWrite = bWrite;};	//����������;���������Ƿ�дLOG�Ŀ���
	void StopLog();
	void RealWriteLog();					//����д����־����

	static BOOL ListFileInPath(const char* cpDir,vector<string>& vecFile,BOOL bShortName,BOOL bChildDir);
	static void GetCurrExePath(string& strPath);

private:
	void AddLog(BOOL bViewInWnd,const char* szContent);	//������м���Log����

public:
	enum {eMaxLogRecordInFile = 200000,};		//��һLOG�ļ�������20����LOG��¼����(ֻ�Ǵ��³���������һ�����ͽ������ļ�)
	
private:	
	string m_strPrefix;					//��־�ļ�����ǰ׺
	string m_strLogDir;					//Log��־�ķ���Ŀ¼��
	string m_strLogFileName;			//Log��־���ļ�����ȫ���磺E:\\Log\\20101210\\DBServer_20101210_001.log
	string m_strLogContent;				//��ǰ��־����
	
	int m_nCurLogFileID;				//��ǰ��Log�ļ����к�
	int m_nCurLogCount;					//��ǰ��LOG�ļ���¼����
	int m_nCurYear;						//��ǰ��
	int m_nCurMonth;					//��ǰ��
	int m_nCurDay;						//��ǰ��

	BOOL m_bLogWrite;				//�����Ƿ�дLOG

	char m_szTempBuf[APPLOGMAXLEN];

	HWND m_hWndInfo;
	DLockerBuff	m_LogLock;	
};

#endif