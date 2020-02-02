#include "stdafx.h"
#include "Exception.h"
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

#pragma comment(lib,"dbghelp.lib")

LONG WINAPI ExceptionCallBack(_EXCEPTION_POINTERS* pExcp)
{	
	char szPath[512] = {0};
	DWORD dwPathLen = ::GetModuleFileNameA(nullptr,szPath,(512 - 1));
	SYSTEMTIME mySystemTime;
	::GetLocalTime(&mySystemTime);

	_snprintf(szPath + dwPathLen,(sizeof(szPath) - dwPathLen),"_%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d.dmp", mySystemTime.wYear, mySystemTime.wMonth, mySystemTime.wDay, mySystemTime.wHour, mySystemTime.wMinute, mySystemTime.wSecond);
	HANDLE hFile = ::CreateFileA(szPath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{	
		_MINIDUMP_EXCEPTION_INFORMATION exinfo;
		exinfo.ThreadId = ::GetCurrentThreadId();
		exinfo.ExceptionPointers = pExcp;
		exinfo.ClientPointers = FALSE;
		BOOL bOK = MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal, &exinfo, nullptr, nullptr);
		if(!bOK)
		{
			char szMsg[256] = {0};
			_snprintf(szMsg,sizeof(szMsg),"MiniDumpWriteDump Fail.Error Code:%d",GetLastError());
			DWORD dwWrite = 0;
			WriteFile(hFile,szMsg,strlen(szMsg),&dwWrite,nullptr);
			FlushFileBuffers(hFile);
		}
		::CloseHandle(hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void DisableSetUnhandledExceptionFilter()
{
	void *pProcAddress = (void*)GetProcAddress(LoadLibraryA("kernel32.dll"),	"SetUnhandledExceptionFilter");

	if (pProcAddress != nullptr)
	{
		unsigned char szCode[16];
		INT nSize = 0;
		szCode[nSize++] = 0x33;
		szCode[nSize++] = 0xC0;
		szCode[nSize++] = 0xC2;
		szCode[nSize++] = 0x04;
		szCode[nSize++] = 0x00;

		DWORD dwOldFlag = 0;
		DWORD dwTempFlag = 0;
		VirtualProtect(pProcAddress, nSize, PAGE_READWRITE, &dwOldFlag);
		WriteProcessMemory(GetCurrentProcess(), pProcAddress, szCode, nSize, nullptr);
		VirtualProtect(pProcAddress, nSize, dwOldFlag, &dwTempFlag);
	}
}

void InitExceptionHandler()
{
	::SetUnhandledExceptionFilter(ExceptionCallBack);
#ifndef _DEBUG
	DisableSetUnhandledExceptionFilter();			//release模式下，必须使用API HOOK拦截系统的异常处理
#endif
}

