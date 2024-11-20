#include "StdAfx.h"
#include "CEICommandContainer.h"

CEICommandAsValue::CEICommandAsValue()
{
	memset(_value, 0, sizeof(_value));
	_Type = VT_Undefined;
}

CEICommandAsValue::~CEICommandAsValue()
{
}

CEICommandAsValue::CEICommandAsValue(char* data)
{
	_Type = VT_String;
	strncpy(_value, data, sizeof(_value) - 1); // Almacenar la cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(const char* data)
{
	_Type = VT_String;
	strncpy(_value, data, sizeof(_value) - 1); // Almacenar la cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(bool data)
{
	_Type = VT_Boolean;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(short data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(int data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(float data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(double data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(BYTE data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(WORD data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(DWORD data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

CEICommandAsValue::CEICommandAsValue(unsigned __int64 data)
{
	_Type = VT_Number;
	_snprintf(_value, sizeof(_value), "%f", (double)data); // Convertir número a cadena
	_value[sizeof(_value) - 1] = '\0';
}

int CEICommandAsValue::GetType() const
{
	return _Type;
}

bool CEICommandAsValue::to_boolean() const
{
	return (bool)std::stoi(_value);
}

double CEICommandAsValue::to_Number() const
{
	return std::stod(_value);
}

const char* CEICommandAsValue::to_char() const
{
	return _value;
}
