#include "stdafx.h"
#include "DAppLog.h"
#include <io.h>
#include <Dbghelp.h>

DAppLog::DAppLog()
{
	m_strPrefix = "";
	m_strLogDir = "";
	m_strLogFileName = "";
	m_strLogContent = "";

	m_nCurLogFileID = 0;
	m_nCurLogCount = 0;
	m_nCurYear = 0;
	m_nCurMonth = 0;
	m_nCurDay = 0;

	m_bLogWrite = FALSE;
	m_hWndInfo = nullptr;

	ZeroMemory(m_szTempBuf,sizeof(m_szTempBuf));
}

DAppLog::~DAppLog()
{
}

DAppLog* DAppLog::Instance()
{
	static DAppLog s_inst;
	return &s_inst;
}

BOOL DAppLog::SetDirPrefix(const char* szDir,const char* szPrefix)
{
	m_strLogDir = szDir;
	m_strPrefix = szPrefix;
	if(m_strLogDir[m_strLogDir.size() - 1] != '\\')
		m_strLogDir += "\\";
	
	//得到当前路径名
	string strPath = "";
	if(m_strLogDir.find(":\\") != string::npos)				//绝对路径
		strPath = m_strLogDir;
	else													//相对路径
	{
		GetCurrExePath(strPath);
		strPath += m_strLogDir;
	}
	m_strLogDir = strPath;
	//获取当前日期最大LOG文件ID号
	m_nCurLogFileID = 0;
	char buf[512] = {0},bufDay[512] = {0};
	SYSTEMTIME sysTime;  //得到系统时间
	GetLocalTime(&sysTime);
	m_nCurYear = sysTime.wYear;
	m_nCurMonth = sysTime.wMonth;
	m_nCurDay = sysTime.wDay;
	_snprintf(buf,(sizeof(buf) - 1),"%d-%d-%d",m_nCurYear,m_nCurMonth,m_nCurDay);
	string strTemp = buf;
	strPath += strTemp;
	strPath += "\\";

	if(_access(strPath.c_str(),0) == -1)		//判断是否存在LOG目录
	{
		::MakeSureDirectoryPathExists(strPath.c_str());		
	}

	_snprintf(bufDay,(sizeof(bufDay) - 1),"%s_%04d%02d%02d",m_strPrefix.c_str(),m_nCurYear,m_nCurMonth,m_nCurDay);
	vector<string> vecFile;
	ListFileInPath(strPath.c_str(),vecFile,TRUE,FALSE);
	if(vecFile.size() == 0)
		m_nCurLogFileID = 1;
	else
	{
		int nPos = m_strPrefix.length() + 10; //"_20101209_"的长度为10
		memset(buf,0,sizeof(buf));
		int nTemp = 0;

		for(size_t i = 0; i < vecFile.size(); i++)
		{
			if(vecFile[i].length() < (nPos + 3))	//如果文件名长度小于规定的日期"20101125"之类的,这种情况会出现在"."".."目录文件中
				continue;

			if(strncmp(bufDay,vecFile[i].c_str(),(nPos - 1)) == 0)
			{
				strncpy(buf, (vecFile[i].c_str() + nPos), 3);
				nTemp = ::atoi(buf);
				if(nTemp > m_nCurLogFileID)
					m_nCurLogFileID = nTemp;
			}
		}

		++m_nCurLogFileID;
	}

	//计算出当前的日志文件
	memset(buf,0,sizeof(buf));
	_snprintf(buf,(sizeof(buf) - 1),"%s_%04d%02d%02d_%03d.log",m_strPrefix.c_str(),m_nCurYear,m_nCurMonth,m_nCurDay,m_nCurLogFileID);
	strTemp = buf;
	m_strLogFileName = strPath + strTemp;
	//创建当前日志文件
	HANDLE hFile = ::CreateFileA(m_strLogFileName.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,nullptr);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		StopLog();
		return FALSE;
	}
	::CloseHandle(hFile);
	hFile = nullptr;

	return TRUE;
}

void DAppLog::AddLog(BOOL bViewInWnd,const char* szContent)
{	
	DAutoLock autoLock(m_LogLock);
	//每条LOG前，会加上时间，后面会加上\r\n的换行符
	size_t nLen = strlen(szContent);
	if(nLen < 0)
		return;
	
	ZeroMemory(m_szTempBuf,sizeof(m_szTempBuf));
	SYSTEMTIME sysTime;  //得到系统时间
	GetLocalTime(&sysTime);
	_snprintf(m_szTempBuf,(sizeof(m_szTempBuf) - 1),"[%04d-%02d-%02d %02d:%02d:%02d] %s\r\n",sysTime.wYear,sysTime.wMonth,sysTime.wDay,
		sysTime.wHour,sysTime.wMinute,sysTime.wSecond,szContent);
	
	m_strLogContent = m_szTempBuf;
	RealWriteLog();

	m_szTempBuf[strlen(m_szTempBuf) - 2] = '\0';	//发给窗口显示时不发送"\r\n"	
	if(m_hWndInfo && bViewInWnd)
	{
		ULONG64 dwResult = 0;
		::SendMessageTimeoutA(m_hWndInfo,WM_APPLOGINFO,(WPARAM)m_szTempBuf,0, SMTO_ABORTIFHUNG,20,&dwResult);
	}
}

void DAppLog::Info(BOOL bViewInWnd,const char* szFormat,...)
{
	if(!m_bLogWrite)	//
		return;
	if(!szFormat)
		return;

	char buf[APPLOGMAXLEN] = {0};
	va_list argptr;
	int nCnt = 0;
	va_start(argptr,szFormat);
	nCnt = vsnprintf(buf,(sizeof(buf) - 1),szFormat,argptr);
	va_end(argptr);	

	AddLog(bViewInWnd,buf);
}

void DAppLog::AddAssertLog(const char* szFile,int nLine,const char* szFunc,const char* szContent)
{
	if(!m_bLogWrite)	
		return;

	int nLen = strlen(szFile) + strlen(szFunc) + strlen(szContent);
	nLen += 16;

	char* pBuf = new char[nLen];		//在本函数结束时DELETE
	_snprintf(pBuf,nLen - 1,"ASSERT:%s %d %s %s",szFile,nLine,szFunc,szContent);
	AddLog(FALSE,pBuf);

	delete[] pBuf;		
}

void DAppLog::Init(const char* szDir,const char* szPrefix,HWND hWndInfo)
{
	m_bLogWrite = TRUE;
	SetDirPrefix(szDir,szPrefix);
	m_hWndInfo = hWndInfo;
}

void DAppLog::StopLog()
{
	m_bLogWrite = FALSE;
}

void DAppLog::RealWriteLog()
{	
	//LOG内容写入文件
	FILE* fl = fopen(m_strLogFileName.c_str(),"a+");
	if(!fl)
	{
		StopLog();
		return;
	}
	fwrite(m_strLogContent.c_str(),1,m_strLogContent.length(),fl);
	m_nCurLogCount++;	
	fclose(fl);	

	//如果日期发生了变化或者单个LOG文件超过最大日志数量，要建立新文件
	//E:\\server\\Log\\20101210\\DBServer20101210_001.log
	int nYear = 0,nMonth = 0,nDay = 0;
	SYSTEMTIME sysTime;  //得到系统时间
	GetLocalTime(&sysTime);
	nYear = sysTime.wYear;
	nMonth = sysTime.wMonth;
	nDay = sysTime.wDay;
	BOOL bNeedNewFile = FALSE;
	if((m_nCurYear != nYear) || (m_nCurMonth != nMonth) || (m_nCurDay != nDay))
	{
		m_nCurYear = nYear;
		m_nCurMonth = nMonth;
		m_nCurDay = nDay;		
		m_nCurLogFileID = 1;
		m_nCurLogCount = 0;

		bNeedNewFile = TRUE;
	}
	else if(m_nCurLogCount > eMaxLogRecordInFile)
	{
		++m_nCurLogFileID;
		m_nCurLogCount = 0;

		bNeedNewFile = TRUE;
	}
	if(bNeedNewFile)
	{
		string strPath = "",strTemp = "";
		strPath = m_strLogDir;		
		char buf[512] = {0};		
		_snprintf(buf,sizeof(buf) - 1,"%d-%d-%d",m_nCurYear,m_nCurMonth,m_nCurDay);
		strTemp = buf;
		strPath += strTemp;
		strPath += "\\";
		if(_access(strPath.c_str(),0) == -1)		//判断是否存在LOG目录
		{
			::MakeSureDirectoryPathExists(strPath.c_str());
		}
		memset(buf,0,sizeof(buf));
		_snprintf(buf,sizeof(buf),"%s_%04d%02d%02d_%03d.log",m_strPrefix.c_str(),m_nCurYear,m_nCurMonth,m_nCurDay,m_nCurLogFileID);
		strTemp = buf;
		m_strLogFileName = strPath + strTemp;
		//创建当前日志文件
		HANDLE hFile = ::CreateFileA(m_strLogFileName.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,nullptr);
		if (hFile == INVALID_HANDLE_VALUE) 
		{
			StopLog();
			return;
		}
		::CloseHandle(hFile);
		hFile = nullptr;
	}
}

BOOL DAppLog::ListFileInPath(const char* cpDir,vector<string>& vecFile,BOOL bShortName,BOOL bChildDir)
{
	if((strlen(cpDir) > 510) || (strlen(cpDir) == 0))	//传进来的路径长度不能大于510，因为函数体内拷贝的szDir临时变量只分配了512字节，可能还要加一个'\'字符
		return FALSE;									//路径长度小于1则明显不对

	char szDir[512] = {0};
	strncpy(szDir,cpDir,sizeof(szDir) - 1);
	int nDirLen = (int)strlen(szDir);
	if(szDir[nDirLen - 1] != '\\')	//如果传进来的路径不以'\'结尾，加一个'\'字符
	{
		szDir[nDirLen] = '\\';
		szDir[nDirLen + 1] = '\0';
	}

	string strFind,strFile,strTemp = "*.*";
	strFind = szDir; strFind += strTemp;

	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	if((hFind = ::FindFirstFileA(strFind.c_str(),&FindFileData)) == INVALID_HANDLE_VALUE)
		return FALSE;
	while(TRUE)
	{
		if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && bChildDir)
		{
			if(FindFileData.cFileName[0] != '.')
			{
				strTemp = FindFileData.cFileName;
				strFile = szDir;
				strFile += strTemp;
				if(!ListFileInPath(strFile.c_str(),vecFile,bShortName,bChildDir))
					return FALSE;
			}
		}
		else
		{
			if(bShortName)
				strFile = FindFileData.cFileName;
			else
			{
				strFile = szDir; 
				strTemp = FindFileData.cFileName;
				strFile += strTemp;
			}

			vecFile.push_back(strFile);
		}
		if(!::FindNextFileA(hFind,&FindFileData))
			break;
	}

	return TRUE;
}

void DAppLog::GetCurrExePath(string& strPath)
{
	char buf[512] = {0};
	::GetModuleFileNameA(nullptr,buf,(sizeof(buf) - 1));

	size_t nLen = strlen(buf);
	nLen -= 2;		//最后一位是\0
	while(true)
	{
		if(buf[nLen] == '\\')
			break;
		--nLen;
	}
	buf[nLen + 1] = '\0';	//只保留路径，可执行文件名不要

	strPath = buf;
}
