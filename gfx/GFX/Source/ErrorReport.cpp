#include "StdAfx.h"
#include "ErrorReport.h"

CErrorReport g_ConsoleDebug;

CErrorReport::CErrorReport()
{
#if CONSOLE != 0
	Create();
#endif
}

CErrorReport::~CErrorReport()
{
}

void CErrorReport::Create()
{
#if CONSOLE != 0
	char lpszFileName[128] = "gfx.log";
	// Cambia OPEN_ALWAYS a CREATE_ALWAYS
	this->lpszFile = CreateFile(lpszFileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (this->lpszFile == INVALID_HANDLE_VALUE)
	{
		return;
	}
#endif
	// No necesitas SetFilePointer para CREATE_ALWAYS, ya que siempre crea un archivo nuevo.
}

void CErrorReport::LogMessage(const char* lpszFormat, ...)
{
#if CONSOLE != 0
	char lpszBufferTEmp[1024] = { 0, };
	va_list va;
	va_start(va, lpszFormat);
	vsprintf_s(lpszBufferTEmp, lpszFormat, va);
	va_end(va);

	strcat_s(lpszBufferTEmp, "\r\n");

	DWORD OutSize;
	WriteFile(this->lpszFile, lpszBufferTEmp, strlen(lpszBufferTEmp), &OutSize, 0);
#endif
}