#include "framework.h"
#include "Util.h"

static BYTE bBuxCode[]={ 0xFC , 0xCF, 0xAB };

void BuxConvert(BYTE* pbyBuffer, DWORD Size)
{
	for (int i = 0; i < Size; i++)
	{
		pbyBuffer[i] ^= bBuxCode[i % 3];
	}
}

DWORD MapFileEncrypt(BYTE* pbyDst, BYTE* pbySrc, int iSize)
{
	if (!pbyDst)
	{
		return (iSize);
	}
	BYTE byMapXorKey[16] = { 0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
							0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2 };

	WORD wMapKey = 0x5E;
	for (int i = 0; i < iSize; ++i)
	{
		pbyDst[i] = (pbySrc[i] + (BYTE)wMapKey) ^ byMapXorKey[i % 16];
		wMapKey = pbyDst[i] + 0x3D;
		wMapKey = wMapKey & 0xFF;
	}
	return (iSize);
}

DWORD MapFileDecrypt(BYTE* pbyDst, BYTE* pbySrc, int iSize)
{
	if (!pbyDst)
	{
		return (iSize);
	}
	BYTE byMapXorKey[16] = { 0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
							0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2 };

	WORD wMapKey = 0x5E;
	for (int i = 0; i < iSize; ++i)
	{
		pbyDst[i] = (pbySrc[i] ^ byMapXorKey[i % 16]) - (BYTE)wMapKey;
		wMapKey = pbySrc[i] + 0x3D;
		wMapKey = wMapKey & 0xFF;
	}
	return (iSize);
}

DWORD GenerateCheckSum2(BYTE* pbyBuffer, int dwSize, WORD Key)
{
	int dwKey = Key;
	int dwResult = Key << 9;

	for (int dwChecked = 0; dwChecked <= dwSize - 4; dwChecked += 4)
	{
		DWORD dwTemp;

		memcpy(&dwTemp, pbyBuffer + dwChecked, 4);

		DWORD v4 = (Key + (dwChecked >> 2)) % 2;
		switch (v4)
		{
		case 0:
			dwResult ^= dwTemp;
			break;
		case 1:
			dwResult += dwTemp;
			break;
		}

		if (!(dwChecked % 0x10))
		{
			dwResult ^= (unsigned int)(dwResult + dwKey) >> ((dwChecked >> 2) % 8 + 1);
		}
	}
	return dwResult;
}

void CreateMessageBox(UINT uType, const char* title, const char* message, ...) // OK
{
	char buff[256];

	memset(buff, 0, sizeof(buff));

	va_list arg;
	va_start(arg, message);
	vsprintf_s(buff, message, arg);
	va_end(arg);

	MessageBox(NULL, buff, title, uType);
}

std::string WStringToUTF8(const std::wstring& wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(wstr);
}

DWORD GetIndexItem(BYTE* lpMsg)
{
	return (lpMsg[0] | ((lpMsg[9] & 0xF0) * 32) | ((lpMsg[7] & 0x80) * 2));
}

DWORD GetSerialItem(BYTE* lpMsg)
{
	if (lpMsg[0] == 0xFF && (lpMsg[7] & 0x80) == 0x80 && (lpMsg[9] & 0xF0) == 0xF0)
	{
		return 0xFFFFFFFF;
	}

	return (MAKE_NUMBERDW(MAKE_NUMBERW(lpMsg[3], lpMsg[4]), MAKE_NUMBERW(lpMsg[5], lpMsg[6])));
}
