#pragma once
class CErrorReport
{
public:
	CErrorReport();
	virtual ~CErrorReport();

	int m_nFileCount;
	HANDLE lpszFile;
	void Create();
	void LogMessage(const char* lpszFormat, ...);
};

extern CErrorReport g_ConsoleDebug;

#define pLog		((&g_ConsoleDebug))