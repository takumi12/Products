#pragma once

#define MAX_COLUMNS 100

class CQueryManager
{
public:
	CQueryManager();
	virtual ~CQueryManager();
	bool Connect(const char* odbc, const char* user, const char* pass);
	void Disconnect();
	void Diagnostic(char* query);
	bool ExecQuery(const char* query, ...);
	void Close();
	SQLRETURN Fetch();
	int GetResult(int index);
	int FindIndex(char* ColName);
	int FindIndex(const char* ColName);
	int GetAsInteger(char* ColName);
	int GetAsInteger(const char* ColName);
	float GetAsFloat(char* ColName);
	float GetAsFloat(const char* ColName);
	__int64 GetAsInteger64(char* ColName);
	__int64 GetAsInteger64(const char* ColName);
	void GetAsString(char* ColName, char* OutBuffer, int OutBufferSize);
	void GetAsString(const char* ColName, char* OutBuffer, int OutBufferSize);
	void GetAsBinary(char* ColName, BYTE* OutBuffer, int OutBufferSize);
	void GetAsBinary(const char* ColName, BYTE* OutBuffer, int OutBufferSize);


	void BindParameterAsString(int ParamNumber, void* InBuffer, int ColumnSize);
	void BindParameterAsBinary(int ParamNumber, void* InBuffer, int ColumnSize);
	void ConvertStringToBinary(char* InBuff, int InSize, BYTE* OutBuff, int OutSize);
	void ConvertBinaryToString(BYTE* InBuff, int InSize, char* OutBuff, int OutSize);
private:
	SQLHANDLE m_SQLEnvironment;
	SQLHANDLE m_SQLConnection;
	SQLHANDLE m_STMT;
	char m_odbc[32];
	char m_user[32];
	char m_pass[32];
	SQLINTEGER m_RowCount;
	SQLSMALLINT m_ColCount;
	SQLCHAR m_SQLColName[MAX_COLUMNS][30];
	char m_SQLData[MAX_COLUMNS][8192];
	SQLINTEGER m_SQLDataLen[MAX_COLUMNS];
	SQLINTEGER m_SQLBindValue[MAX_COLUMNS];
	//std::vector<std::string> m_SQLData;
};

extern CQueryManager gQueryManager;