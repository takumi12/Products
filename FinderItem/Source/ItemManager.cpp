#include "framework.h"
#include "Util.h"
#include "pugixml.hpp"
#include "ItemManager.h"


CItemManager::CItemManager()
{
}

CItemManager::~CItemManager()
{
}

void CItemManager::Load(const char* filename)
{
	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(filename);

	this->m_ItemInfo.clear();

	if (res.status != pugi::status_ok)
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, "Error", "Error load fail: %s", filename);
		return;
	}

	pugi::xml_node root = file.child("ItemList");

	for (pugi::xml_node child = root.child("Section"); child; child = child.next_sibling())
	{
		int Section = child.attribute("Index").as_int();

		for (pugi::xml_node Item = child.child("Item"); Item; Item = Item.next_sibling())
		{
			Script_Item item_info;

			item_info.Index = (GET_ITEM(Section, Item.attribute("Index").as_int()));

			item_info.Name = Item.attribute("Name").as_string();

			this->m_ItemInfo.insert(type_map_item::value_type(item_info.Index, item_info));

		}
	}
}

const char* CItemManager::GetName(int index)
{
	std::map<int, Script_Item>::iterator it = this->m_ItemInfo.find(index);

	if (it == this->m_ItemInfo.end())
	{
		return "None";
	}
	else
	{
		return it->second.Name.c_str();
	}
}

CItemManager* CItemManager::Instance()
{
	static CItemManager s_Instance;
	return &s_Instance;
}
