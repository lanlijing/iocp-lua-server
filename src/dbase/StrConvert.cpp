#include "stdafx.h"
#include "DMemoryPool.h"
#include "StrConvert.h"

string StrConvert::a2u(string strIn)
{
	string strRet = "";

	const char* str = strIn.c_str();
	int utfLen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wchar_t* pUtfStr = (wchar_t*)(DMemoryPool::Instance()->GetMemory((utfLen + 1) * sizeof(wchar_t)));		//从内存池中取内存(内存池中已经初始化为0)
	MultiByteToWideChar(CP_ACP, 0, str, -1, pUtfStr, utfLen);
	int nTargetLen = WideCharToMultiByte(CP_UTF8, 0, pUtfStr, -1, NULL, 0, NULL, NULL);
	char* pretBuf = DMemoryPool::Instance()->GetMemory(nTargetLen + 1);								//从内存池中取内存(内存池中已经初始化为0)
	WideCharToMultiByte(CP_UTF8, 0, pUtfStr, -1, pretBuf, nTargetLen, NULL, NULL);
	strRet = pretBuf;

	DMemoryPool::Instance()->BackMemory((char**)&pUtfStr);							//内存池回收内存
	DMemoryPool::Instance()->BackMemory(&pretBuf);

	return strRet;
}

string StrConvert::u2a(string strIn)
{
	string strRet = "";

	const char* str = strIn.c_str();
	int utf8Len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	wchar_t* pUtf8Str = (wchar_t*)(DMemoryPool::Instance()->GetMemory((utf8Len + 1) * sizeof(wchar_t)));	//从内存池中取内存(内存池中已经初始化为0)
	MultiByteToWideChar(CP_UTF8, 0, str, -1, pUtf8Str, utf8Len);
	int nAnsiLen = WideCharToMultiByte(CP_ACP, 0, pUtf8Str, -1, NULL, 0, NULL, NULL);
	char* ansiBuf = DMemoryPool::Instance()->GetMemory(nAnsiLen + 1);						//从内存池中取内存(内存池中已经初始化为0)
	WideCharToMultiByte(CP_ACP, 0, pUtf8Str, -1, ansiBuf, nAnsiLen, NULL, NULL);
	strRet = ansiBuf;

	DMemoryPool::Instance()->BackMemory(&ansiBuf);							//内存池回收内存
	DMemoryPool::Instance()->BackMemory((char**)&pUtf8Str);							//内存池回收内存

	return strRet;
}