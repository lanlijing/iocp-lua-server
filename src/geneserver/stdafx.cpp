
// stdafx.cpp : 只包括标准包含文件的源文件
// geneserver.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"

//把要包含的LIB文件放在此
#ifdef _DEBUG

#pragma comment (lib, "dbased.lib")
#pragma comment (lib, "diocpd.lib")
#pragma comment (lib, "lua51d.lib")

#else

#pragma comment (lib, "dbase.lib")
#pragma comment (lib, "diocp.lib")
#pragma comment (lib, "lua51.lib")

#endif


