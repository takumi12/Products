#pragma once
#define INVENTORY_WEAR_SIZE					12
#define INVENTORY_MAIN_SIZE					76
#define INVENTORY_EXT1_SIZE					108
#define INVENTORY_EXT2_SIZE					140
#define INVENTORY_EXT3_SIZE					172
#define INVENTORY_EXT4_SIZE					204
#define INVENTORY_FULL_SIZE					236
#define WAREHOUSE_SIZE						240

#define SET_NUMBERHB(x) ((BYTE)((DWORD)(x)>>(DWORD)8))
#define SET_NUMBERLB(x) ((BYTE)((DWORD)(x)&0xFF))
#define SET_NUMBERHW(x) ((WORD)((DWORD)(x)>>(DWORD)16))
#define SET_NUMBERLW(x) ((WORD)((DWORD)(x)&0xFFFF))
#define SET_NUMBERHDW(x) ((DWORD)((QWORD)(x)>>(QWORD)32))
#define SET_NUMBERLDW(x) ((DWORD)((QWORD)(x)&0xFFFFFFFF))

#define MAKE_NUMBERW(x,y) ((WORD)(((BYTE)((y)&0xFF))|((BYTE)((x)&0xFF)<<8)))
#define MAKE_NUMBERDW(x,y) ((DWORD)(((WORD)((y)&0xFFFF))|((WORD)((x)&0xFFFF)<<16)))
#define MAKE_NUMBERQW(x,y) ((QWORD)(((DWORD)((y)&0xFFFFFFFF))|((DWORD)((x)&0xFFFFFFFF)<<32)))




extern void BuxConvert(BYTE* pbyBuffer, DWORD Size);
extern DWORD MapFileEncrypt(BYTE* pbyDst, BYTE* pbySrc, int iSize);
extern DWORD MapFileDecrypt(BYTE* pbyDst, BYTE* pbySrc, int iSize);
extern DWORD GenerateCheckSum2(BYTE* pbyBuffer, int dwSize, WORD Key);
extern void CreateMessageBox(UINT uType, const char* title, const char* message, ...); // OK
extern std::string WStringToUTF8(const std::wstring& wstr);

extern DWORD GetIndexItem(BYTE* lpMsg);
extern DWORD GetSerialItem(BYTE* lpMsg);
