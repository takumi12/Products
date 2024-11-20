#pragma once
#define PACK_FILE_DECRYPT_H

#define MAX_LEVEL_TOOLTIP			128
#define MAX_TEXT_TOOLTIP			512
#define GET_ITEM(x, y)				((x*MAX_ITEM_INDEX) + y)

#pragma pack(push, 1)
typedef struct
{
	short Text;
	unsigned char Color;
	bool IsBold;
} _TEXT_INFO;
#pragma pack(pop)

typedef struct
{
	WORD Index;
	char Description[256];
	WORD iStatus;
}_ITEM_TOOLTIP_TEXT;

typedef struct
{
	WORD Type;
	WORD Index;
	char Name[64];
	unsigned char ColorName;
	bool IsBold;
	bool NoTaxGold;
	bool IsOptionSpecial;
	bool RenderLevel;
	short ItemLevel;
	_TEXT_INFO TextInfo[12];
}_ITEM_TOOLTIP_DATA;

typedef struct
{
/*+00*/ WORD Type;
/*+02*/ char Name[64];
/*+66*/ unsigned char ColorName;
/*+67*/ bool IsBold;
/*+68*/ bool IsOptionSpecial;
/*+69*/ bool RenderLevel;
/*+70*/ _TEXT_INFO TextInfo[8];
}_ITEM_LEVEL_TOOTIP_DATA;

class CNewUIItemTooltip
{
public:
	CNewUIItemTooltip();
	virtual ~CNewUIItemTooltip();

	void OpenFileTooltip();
	void OpenItemTooltip(const char* filename);
	void OpenItemTooltipText(const char* filename);
	void OpenItemLevelTooltip(const char* filename);


	void RenderItemTooltip(int sx, int sy, ITEM* pItem, bool Sell, int InvenType = 0, bool bItemTextListBoxUse = false);
	void SetTooltipValueText(char* Description, char* information, int NextLine, int* color, int Num, ITEM* pItem, Script_Item* pItemAtrr, int iItemLevel);
	void SetNeedStrDex(int* iDecNeedStrength, int* iDecNeedDex, int iItemLevel, ITEM* pItem);
	int GetSocketSeedID(__int16 Type, int iLevel);
	void ItemEquipClass(Script_Item* pItemAttr, int* NextLine, int* SkipNum);
	BOOL IsAbsoluteWeapon(__int16 Type);
	BOOL IsEnableOptionWing(__int16 Type);

	_ITEM_TOOLTIP_DATA* FindTooltip(int Type);
	_ITEM_TOOLTIP_TEXT* FindTooltipText(int Index);
	_ITEM_LEVEL_TOOTIP_DATA* FindLevelTooltip(int Level);
	bool InBounds(int index, int min, int max);
	int SplitText(char* text, int a2, int TextNum, int Color, int Bold, bool a6);

	void CreateItemName(ITEM_t* pItemMap);
private:
	std::vector<_ITEM_TOOLTIP_DATA>m_ItemTooltip;
	std::vector<_ITEM_TOOLTIP_TEXT>m_ItemTooltipText;
	std::vector<_ITEM_LEVEL_TOOTIP_DATA>m_ItemTooltipLevel;
	int m_ItemLevel;
};

extern std::string g_strSelectedML;
