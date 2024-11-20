#pragma once
#define MAX_ITEM_SECTION 16
#define MAX_ITEM_TYPE 512
#define MAX_ITEM (MAX_ITEM_SECTION*MAX_ITEM_TYPE)
#define GET_ITEM(x,y)					(((x)*MAX_ITEM_TYPE)+(y))

typedef struct
{
	int Index;
	std::string Name;
}Script_Item;

typedef std::map<int, Script_Item> type_map_item;


class CItemManager
{
public:
	CItemManager();
	virtual~CItemManager();
	void Load(const char* filename);
	const char* GetName(int index);


	static CItemManager* Instance();
private:
	type_map_item m_ItemInfo;
};

#define gItemManager			(CItemManager::Instance())
