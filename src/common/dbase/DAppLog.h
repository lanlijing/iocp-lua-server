/***********************************************************************************
name:DAppLog
comment:
1、一个日志类
2、具有线程安全
3、并没有采取先把部分日志缓存在内存中的机制，是因为如果服务器异常当掉，缓存在内存中的日志将会丢失
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
	BOOL SetDirPrefix(const char* szDir,const char* szPrefix);		//设置日志的分类目录
	void Info(BOOL bViewInWnd,const char* szFormat,...);
	void AddAssertLog(const char* szFile,int nLine,const char* szFunc,const char* szContent);	//打印出某个函数那里出了错
#define AssertLog(N) AddAssertLog(__FILE__,__LINE__,__FUNCTION__,N)
	void Init(const char* szDir,const char* szPrefix,HWND hWndInfo = nullptr);
	void SetLogWrite(BOOL bWrite){m_bLogWrite = bWrite;};	//程序运行中途可以设置是否写LOG的开关
	void StopLog();
	void RealWriteLog();					//真正写入日志内容

	static BOOL ListFileInPath(const char* cpDir,vector<string>& vecFile,BOOL bShortName,BOOL bChildDir);
	static void GetCurrExePath(string& strPath);

private:
	void AddLog(BOOL bViewInWnd,const char* szContent);	//向队列中加入Log内容

public:
	enum {eMaxLogRecordInFile = 200000,};		//单一LOG文件控制在20万条LOG记录左右(只是大致超过，不是一超过就建立新文件)
	
private:	
	string m_strPrefix;					//日志文件名的前缀
	string m_strLogDir;					//Log日志的分类目录名
	string m_strLogFileName;			//Log日志的文件名。全长如：E:\\Log\\20101210\\DBServer_20101210_001.log
	string m_strLogContent;				//当前日志内容
	
	int m_nCurLogFileID;				//当前的Log文件序列号
	int m_nCurLogCount;					//当前的LOG文件记录计数
	int m_nCurYear;						//当前年
	int m_nCurMonth;					//当前月
	int m_nCurDay;						//当前日

	BOOL m_bLogWrite;				//设置是否写LOG

	char m_szTempBuf[APPLOGMAXLEN];

	HWND m_hWndInfo;
	DLockerBuff	m_LogLock;	
};

#endif