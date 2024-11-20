#include "stdafx.h"
#include "SDK\Include\GFx\GFxPlayer.h"
#include "CGFxBase.h"

CEICommandAsValue::CEICommandAsValue()
{
	_Number = 0.0;
	_Boolean = false;
	valueType = VT_Boolean;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(bool data) : valueType(VT_Boolean), _Boolean(data)
{
	_Number = 0.0;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(float data) : valueType(VT_Number), _Number(data)
{
	_Boolean = false;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(double data) : valueType(VT_Number), _Number(data)
{
	_Boolean = false;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(int data) : valueType(VT_Number)
{
	_Number = (double)(data);
	_Boolean = false;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(__int64 data) : valueType(VT_Number)
{
	_Number = (double)(data);
	_Boolean = false;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(unsigned __int64 data) : valueType(VT_Number)
{
	_Number = (double)(data);
	_Boolean = false;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(DWORD data) : valueType(VT_Number)
{
	_Number = (double)(data);
	_Boolean = false;
	memset(_String, 0, sizeof(_String));
}

CEICommandAsValue::CEICommandAsValue(char* data) : valueType(VT_String)
{
	_Number = 0.0;
	_Boolean = false;
	strncpy(_String, data, sizeof(_String) - 1);
	_String[sizeof(_String) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(const char* data) : valueType(VT_String)
{
	_Number = 0.0;
	_Boolean = false;
	strncpy(_String, data, sizeof(_String) - 1);
	_String[sizeof(_String) - 1] = '\0';
}

int CEICommandAsValue::GetValueType()
{
	return valueType;
}

bool CEICommandAsValue::GetBooleano()
{
	return _Boolean;
}

double CEICommandAsValue::GetNumber()
{
	return _Number;
}

const char* CEICommandAsValue::GetString()
{
	return _String;
}
