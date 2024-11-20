#pragma once

extern void BuxConvert(BYTE* pbyBuffer, DWORD Size);
extern DWORD MapFileEncrypt(BYTE* pbyDst, BYTE* pbySrc, int iSize);
extern DWORD MapFileDecrypt(BYTE* pbyDst, BYTE* pbySrc, int iSize);
extern DWORD GenerateCheckSum2(BYTE* pbyBuffer, int dwSize, WORD Key);
extern void CreateMessageBox(UINT uType, const char* title, const char* message, ...); // OK
