#include "stdafx.h"
#include "GMHellas.h"
#include "UIManager.h"
#include "PortalMgr.h"
#include "MapManager.h"
#include "NewUISystem.h"
#include "CSItemOption.h"
#include "SocketSystem.h"
#include "SkillManager.h"
#include "GIPetManager.h"
#include "UIJewelHarmony.h"
#include "wsclientinline.h"
#include "PersonalShopTitleImp.h"
#include "NewUIItemTooltip.h"

int g_iChaosCastleZen[6] = { 25, 80, 150, 250, 400, 650 };
int g_iChaosCastleLevel[12][2] = {
						{ 15, 49 }, { 50, 119 }, { 120, 179 }, { 180, 239 }, { 240, 299 }, { 300, 999 },
						{ 15, 29 }, { 30,  99 }, { 100, 159 }, { 160, 219 }, { 220, 279 }, { 280, 999 }
};

#if MAIN_UPDATE == 303
CNewUIItemTooltip::CNewUIItemTooltip()
{
	m_ItemLevel = -1;

	this->OpenItemTooltip("Data\\Local\\itemtooltip.bmd");

	this->OpenItemTooltipText("Data\\Local\\ItemTooltipText.bmd");

	this->OpenItemLevelTooltip("Data\\Local\\ItemLevelTooltip.bmd");
}
#else
CNewUIItemTooltip::CNewUIItemTooltip()
{
	char filename[MAX_PATH];

	m_ItemLevel = -1;


	sprintf_s(filename, "Data\\Local\\%s\\itemtooltip.bmd", g_strSelectedML.c_str());

	this->OpenItemTooltip(filename);

	sprintf_s(filename, "Data\\Local\\%s\\ItemTooltipText.bmd", g_strSelectedML.c_str());

	this->OpenItemTooltipText(filename);

	sprintf_s(filename, "Data\\Local\\%s\\ItemLevelTooltip.bmd", g_strSelectedML.c_str());

	this->OpenItemLevelTooltip(filename);
}
#endif // MAIN_UPDATE == 303

CNewUIItemTooltip::~CNewUIItemTooltip()
{
	m_ItemTooltip.clear();
	m_ItemTooltipText.clear();
	m_ItemTooltipLevel.clear();
}

void CNewUIItemTooltip::OpenFileTooltip()
{
}

void CNewUIItemTooltip::OpenItemTooltip(const char* filename)
{
	int Size = MAX_ITEM;

	Size = PackFileDecrypt<_ITEM_TOOLTIP_DATA>(filename, m_ItemTooltip, MAX_ITEM, sizeof(_ITEM_TOOLTIP_DATA), 0xE2F1);

	if (Size != MAX_ITEM || Size == 0)
	{
		SendMessage(g_hWnd, WM_DESTROY, 0, 0);
	}
}

void CNewUIItemTooltip::OpenItemTooltipText(const char* filename)
{
	int Size = MAX_TEXT_TOOLTIP;

	Size = PackFileDecrypt<_ITEM_TOOLTIP_TEXT>(filename, m_ItemTooltipText, MAX_TEXT_TOOLTIP, sizeof(_ITEM_TOOLTIP_TEXT), 0xE2F1);

	if (Size == 0)
	{
		SendMessage(g_hWnd, WM_DESTROY, 0, 0);
	}
}

void CNewUIItemTooltip::OpenItemLevelTooltip(const char* filename)
{
	int Size = MAX_TEXT_TOOLTIP;

	Size = PackFileDecrypt<_ITEM_LEVEL_TOOTIP_DATA>(filename, m_ItemTooltipLevel, MAX_LEVEL_TOOLTIP, sizeof(_ITEM_LEVEL_TOOTIP_DATA), 0xE2F1);

	if (Size == 0)
	{
		SendMessage(g_hWnd, WM_DESTROY, 0, 0);
	}
}

void CNewUIItemTooltip::RenderItemTooltip(int sx, int sy, ITEM* pItem, bool Sell, int InvenType, bool bItemTextListBoxUse)
{
	if (!pItem || pItem->Type == -1)
		return;

	tm* ExpireTime;

	int ItemType = pItem->Type;

	if (pItem->bPeriodItem == true && pItem->bExpiredPeriod == false)
	{
		_tzset();
		if (pItem->lExpireTime == 0)
			return;

		ExpireTime = localtime((time_t*)&(pItem->lExpireTime));
	}

	m_ItemLevel = -1;
	int ItemLevel = (pItem->Level >> 3) & 0xF;

	Script_Item* pItemAttr = GMItemMng->find(ItemType);

	if (!pItemAttr)
		return;

	int LineNum = 0;
	int SkipLine = 0;

	_ITEM_TOOLTIP_DATA* mcTooltip = FindTooltip(ItemType);

	if (mcTooltip)
	{
		_ITEM_LEVEL_TOOTIP_DATA* mcTooltipLevel = NULL;

		if (mcTooltip->ItemLevel >= 0)
		{
			int FindLevel;

			if (ItemType == ITEM_HELPER + 37)
			{
				switch (pItem->Option1)
				{
				case 1:
					m_ItemLevel = 1;
					break;
				case 2:
					m_ItemLevel = 2;
					break;
				case 4:
					m_ItemLevel = 3;
					break;
				case 8:
					m_ItemLevel = 4;
					break;
				case 16:
					m_ItemLevel = 5;
					break;
				default:
					m_ItemLevel = 0;
					break;
				}

				FindLevel = mcTooltip->ItemLevel + m_ItemLevel;
			}
			else
			{
				FindLevel = mcTooltip->ItemLevel + ItemLevel;
			}
			mcTooltipLevel = FindLevelTooltip(FindLevel);

			if (!mcTooltipLevel)
			{
				return;
			}
		}

		memset(TextList, 0, sizeof(TextList));
		memset(TextBold, 0, sizeof(TextBold));
		memset(TextListColor, 0, sizeof(TextListColor));


		int color = TEXT_COLOR_GREEN_BLUE;

		if (IsAbsoluteWeapon(ItemType))
		{
			if (mcTooltip->ItemLevel == -1)
				color = mcTooltip->ColorName;
			else
				color = mcTooltipLevel->ColorName;
		}
		else if (IsEnableOptionWing(ItemType) == 1)
		{
			if (ItemLevel < 7)
				color = mcTooltip->ColorName;
			else
				color = TEXT_COLOR_YELLOW;
		}
		else if ((pItem->ExtOption % 0x04) != EXT_A_SET_OPTION && (pItem->ExtOption % 0x04) != EXT_B_SET_OPTION)
		{
			if (g_SocketItemMgr.IsSocketItem(pItem) == 1)
			{
				color = TEXT_COLOR_VIOLET;
			}
			else if (pItem->SpecialNum <= 0 || (pItem->Option1 & 63) <= 0)
			{
				if (ItemLevel < 7)
				{
					if (mcTooltip->ItemLevel == -1)
					{
						if (pItem->SpecialNum <= 0)
							color = mcTooltip->ColorName;
						else
							color = TEXT_COLOR_BLUE;
					}
					else
					{
						color = mcTooltipLevel->ColorName;
					}
				}
				else
				{
					color = TEXT_COLOR_YELLOW;
				}
			}
			else
			{
				color = TEXT_COLOR_GREEN;
			}
		}
		else
		{
			color = TEXT_COLOR_GREEN_BLUE;
		}

		if (ItemType != ITEM_HELPER + 4 && ItemType != ITEM_HELPER + 5)
		{
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP) && !mcTooltip->NoTaxGold)
			{
				if (
#ifdef LJH_ADD_SYSTEM_OF_EQUIPPING_ITEM_FROM_INVENTORY
					!g_pMyInventory->IsInvenItem(ItemType) ||
#endif
#ifdef KJH_FIX_SELL_LUCKYITEM
					IsLuckySetItem(ItemType) &&
#endif
					!pItem->Durability
					)
				{
					char value[100] = { 0 };

					if (Sell)
					{
						unicode::t_char val64[100];

						ConvertGold(ItemValue(pItem, 0), value, 0);
						ConvertGold64(ItemValue(pItem, 0), val64);
						sprintf(TextList[LineNum], GlobalText[1620], val64, value);
					}
					else
					{
						ConvertGold(ItemValue(pItem, 1), value, 0);
						sprintf(TextList[LineNum], GlobalText[63], value);
					}

					TextListColor[LineNum] = color;
					TextBold[LineNum++] = mcTooltip->IsBold;

					sprintf(TextList[LineNum++], "\n");
					SkipLine++;
				}
			}

			if ((InvenType == SEASON3B::TOOLTIP_TYPE_MY_SHOP || InvenType == SEASON3B::TOOLTIP_TYPE_PURCHASE_SHOP)
				&& !IsPersonalShopBan(pItem))
			{
				int price;
				char Text[100];

				int indexInv = pItem->x + (COL_PERSONALSHOP_INVEN * pItem->y) + MAX_MY_INVENTORY_INDEX;

				if (GetPersonalItemPrice(indexInv, price, g_IsPurchaseShop))
				{
					ConvertGold(price, Text);
					sprintf(TextList[LineNum], GlobalText[63], Text);

					if (price >= 10000000)
						TextListColor[LineNum] = TEXT_COLOR_RED;
					else if (price >= 1000000)
						TextListColor[LineNum] = TEXT_COLOR_YELLOW;
					else if (price >= 100000)
						TextListColor[LineNum] = TEXT_COLOR_GREEN;
					else
						TextListColor[LineNum] = TEXT_COLOR_WHITE;
					TextBold[LineNum++] = true;
					sprintf(TextList[LineNum++], "\n");
					SkipLine++;

					DWORD gold = CharacterMachine->Gold;

					if ((int)gold < price && g_IsPurchaseShop == PSHOPWNDTYPE_PURCHASE)
					{
						TextListColor[LineNum] = TEXT_COLOR_RED;
						TextBold[LineNum] = true;
						sprintf(TextList[LineNum++], GlobalText[423]);
						sprintf(TextList[LineNum++], "\n");
						SkipLine++;
					}
				}
				else if (g_IsPurchaseShop == PSHOPWNDTYPE_SALE)
				{
					TextListColor[LineNum] = TEXT_COLOR_RED;
					TextBold[LineNum] = true;
					sprintf(TextList[LineNum++], GlobalText[1101]);
					sprintf(TextList[LineNum++], "\n");
					SkipLine++;
				}
			}

			char TextName[100];

			memset(TextName, 0, sizeof(TextName));

			if (!g_csItemOption.GetSetItemName(TextName, ItemType, pItem->ExtOption) || IsAbsoluteWeapon(ItemType))
			{
				strcat(TextName, mcTooltip->Name);
			}
			else
			{
				strcat(TextName, mcTooltip->Name);
			}

			if ((pItem->Option1 & 0x3F) <= 0 || IsAbsoluteWeapon(ItemType) || IsEnableOptionWing(ItemType))
			{
				if (mcTooltip->ItemLevel != -1 && mcTooltipLevel)
				{
					sprintf(TextList[LineNum], mcTooltipLevel->Name);
				}
				else
				{
					if (ItemLevel && mcTooltip->RenderLevel)
						sprintf(TextList[LineNum], "%s +%d", TextName, ItemLevel);
					else
						sprintf(TextList[LineNum], TextName);
				}
			}
			else if (m_ItemLevel == -1)
			{
				if (ItemLevel)
					sprintf(TextList[LineNum], "%s %s +%d", GlobalText[620], TextName, ItemLevel);
				else
					sprintf(TextList[LineNum], "%s %s", GlobalText[620], TextName);
			}
			else
			{
				sprintf(TextList[LineNum], mcTooltipLevel->Name);
				color = TEXT_COLOR_BLUE;
			}
			TextListColor[LineNum] = color;

			if (mcTooltip->ItemLevel != -1 && mcTooltipLevel)
				TextBold[LineNum++] = mcTooltipLevel->IsBold;
			else
				TextBold[LineNum++] = mcTooltip->IsBold;

			sprintf(TextList[LineNum++], "\n");
			SkipLine++;

			if (ItemType == ITEM_HELPER + 29)
			{
				static  const int iChaosCastleLevel[12][2] = {
							{ 15, 49 }, { 50, 119 }, { 120, 179 }, { 180, 239 }, { 240, 299 }, { 300, 999 },
							{ 15, 29 }, { 30,  99 }, { 100, 159 }, { 160, 219 }, { 220, 279 }, { 280, 999 } };

				int startIndex = 0;
				if (Hero->GetBaseClass() == Magic_Gladiator
					|| Hero->GetBaseClass() == Dark_Lord
					|| Hero->GetBaseClass() == Rage_Fighter
					)
				{
					startIndex = 6;
				}

				int HeroLevel = CharacterAttribute->Level;

				TextListColor[LineNum] = TEXT_COLOR_WHITE;
				sprintf(TextList[LineNum], "%s %s    %s      %s    ", GlobalText[1147], GlobalText[368], GlobalText[935], GlobalText[936]); TextListColor[LineNum] = TEXT_COLOR_WHITE; TextBold[LineNum] = false; LineNum++;
				TextBold[LineNum++] = false;

				for (int i = 0; i < 6; i++)
				{
					int Zen = g_iChaosCastleZen[i];

					sprintf(TextList[LineNum], "        %d             %3d~%3d     %3d,000", i + 1, iChaosCastleLevel[startIndex + i][0], min(400, iChaosCastleLevel[startIndex + i][1]), Zen);

					if ((HeroLevel >= iChaosCastleLevel[startIndex + i][0] && HeroLevel <= iChaosCastleLevel[startIndex + i][1]) && Hero->IsMasterLevel() == false)
					{
						TextListColor[LineNum] = TEXT_COLOR_DARKYELLOW;
					}
					else
					{
						TextListColor[LineNum] = TEXT_COLOR_WHITE;
					}
					TextBold[LineNum++] = false;
				}
				sprintf(TextList[LineNum], "         %d          %s   %3d,000", 7, GlobalText[737], 1000);

				if (Hero->IsMasterLevel() == true)
					TextListColor[LineNum] = TEXT_COLOR_DARKYELLOW;
				else
					TextListColor[LineNum] = TEXT_COLOR_WHITE;

				TextBold[LineNum++] = false;


				sprintf(TextList[LineNum++], "\n");
				SkipLine++;

				sprintf(TextList[LineNum], GlobalText[1157]);
				TextListColor[LineNum] = TEXT_COLOR_DARKBLUE;
				TextBold[LineNum++] = false;
			}
			else if (ItemType == ITEM_POTION + 28)
			{
				LineNum = RenderHellasItemInfo(pItem, LineNum);
			}
			else if (ItemType == ITEM_HELPER + 70)
			{
				if (pItem->Durability == 2)
				{
					sprintf(TextList[LineNum], GlobalText[2605]);
					TextListColor[LineNum] = TEXT_COLOR_BLUE;
					TextBold[LineNum++] = false;
				}
				else if (pItem->Durability == 1)
				{
					sprintf(TextList[LineNum], GlobalText[2606]);
					TextListColor[LineNum] = TEXT_COLOR_BLUE;
					TextBold[LineNum++] = false;

					g_PortalMgr.GetPortalPositionText(TextList[LineNum]);
					TextListColor[LineNum] = TEXT_COLOR_BLUE;
					TextBold[LineNum++] = false;
				}
			}
			else
			{
				_ITEM_TOOLTIP_TEXT* mcTooltipText;

				if (mcTooltip->ItemLevel != -1 && mcTooltipLevel)
				{
					for (int i = 0; i < 8; ++i)
					{
						int iText = mcTooltipLevel->TextInfo[i].Text;
						int iColor = mcTooltipLevel->TextInfo[i].Color;
						int iBool = mcTooltipLevel->TextInfo[i].IsBold;

						mcTooltipText = FindTooltipText(iText);

						if (!mcTooltipText) continue;

						char Information[256];

						int Num = mcTooltipText->iStatus;
						memset(Information, 0, sizeof(Information));

						int ac = 0;
						if (Num != -1)
						{
							SetTooltipValueText(mcTooltipText->Description, Information, LineNum, &iColor, Num, pItem, pItemAttr, ItemLevel);
							ac = 1;
						}
						else
						{
							memcpy(Information, mcTooltipText->Description, sizeof(Information));
						}

						bool isPercent = (ac != 0);
						LineNum = this->SplitText(Information, 1, LineNum, iColor, iBool, isPercent);
					}
				}
				else
				{
					for (int i = 0; i < 12; ++i)
					{
						int iText = mcTooltip->TextInfo[i].Text;
						int iColor = mcTooltip->TextInfo[i].Color;
						int iBool = mcTooltip->TextInfo[i].IsBold;

						mcTooltipText = FindTooltipText(iText);

						if (!mcTooltipText)
						{
							continue;
						}
						char Information[256];
						int Num = mcTooltipText->iStatus;
						memset(Information, 0, sizeof(Information));

						bool isPercent = false;
						if (Num != -1)
						{
							SetTooltipValueText(mcTooltipText->Description, Information, LineNum, &iColor, Num, pItem, pItemAttr, ItemLevel);
							isPercent = true;
						}
						else
						{
							memcpy(Information, mcTooltipText->Description, sizeof(Information));
						}

						LineNum = this->SplitText(Information, 1, LineNum, iColor, iBool, isPercent);
					}
				}
			}

			if (IsRequireClassRenderItem(ItemType) == 1)
				ItemEquipClass(pItemAttr, &LineNum, &SkipLine);

			if (pItem->option_380 && !IsEnableOptionWing(ItemType))
			{
				if (g_pItemAddOptioninfo)
				{
					std::vector<std::string> base_aux;
					g_pItemAddOptioninfo->GetItemAddOtioninfoText(base_aux, ItemType);

					sprintf(TextList[LineNum++], "\n"); SkipLine++;

					for (int i = 0; i < (int)base_aux.size(); ++i)
					{
						strncpy(TextList[LineNum], base_aux[i].c_str(), 100);
						TextListColor[LineNum] = TEXT_COLOR_REDPURPLE;
						TextBold[LineNum++] = true;
					}
					sprintf(TextList[LineNum++], "\n");
					SkipLine++;
					base_aux.shrink_to_fit();
					base_aux.clear();
				}
			}

			if (!g_SocketItemMgr.IsSocketItem(pItem))
			{
				if (pItem->Jewel_Of_Harmony_Option > 0)
				{
					StrengthenItem type = g_pUIJewelHarmonyinfo->GetItemType(ItemType);

					if (type < SI_None)
					{
						if (g_pUIJewelHarmonyinfo->IsHarmonyJewelOption(type, pItem->Jewel_Of_Harmony_Option))
						{
							sprintf(TextList[LineNum++], "\n");
							SkipLine++;

							HARMONYJEWELOPTION harmonyjewel = g_pUIJewelHarmonyinfo->GetHarmonyJewelOptionInfo(type, pItem->Jewel_Of_Harmony_Option);

							if (type == SI_Defense && pItem->Jewel_Of_Harmony_Option == 7)
							{
								sprintf(TextList[LineNum], "%s +%d%%", harmonyjewel.Name, harmonyjewel.HarmonyJewelLevel[pItem->Jewel_Of_Harmony_OptionLevel]);
							}
							else
							{
								sprintf(TextList[LineNum], "%s +%d", harmonyjewel.Name, harmonyjewel.HarmonyJewelLevel[pItem->Jewel_Of_Harmony_OptionLevel]);
							}

							if (ItemLevel < pItem->Jewel_Of_Harmony_OptionLevel)
								TextListColor[LineNum] = TEXT_COLOR_GRAY;
							else
								TextListColor[LineNum] = TEXT_COLOR_YELLOW;

							TextBold[LineNum++] = true;
							sprintf(TextList[LineNum++], "\n");
							SkipLine++;
						}
						else
						{
							sprintf(TextList[LineNum++], "\n");
							SkipLine++;

							sprintf(TextList[LineNum], "%s : %d %d %d"
								, GlobalText[2204]
								, (int)type
								, (int)pItem->Jewel_Of_Harmony_Option
								, (int)pItem->Jewel_Of_Harmony_OptionLevel
							);

							TextListColor[LineNum] = TEXT_COLOR_DARKRED;
							TextBold[LineNum++] = true;

							sprintf(TextList[LineNum], GlobalText[2205]);

							TextListColor[LineNum] = TEXT_COLOR_DARKRED;
							TextBold[LineNum++] = true;
							sprintf(TextList[LineNum++], "\n"); SkipLine++;
						}
					}
				}
			}

			LineNum = g_csItemOption.RenderDefaultOptionText(pItem, LineNum);

			if (pItem->SpecialNum > 0 && (ItemType < ITEM_HELPER + 109 || ItemType > ITEM_HELPER + 115))
			{
				sprintf(TextList[LineNum++], "\n"); SkipLine++;
			}

			for (int i = 0; i < pItem->SpecialNum && (pItem->Type < ITEM_HELPER + 109 || pItem->Type > ITEM_HELPER + 115 || IsEnableOptionWing(pItem->Type)); i++)
			{
				int iMana;
				gSkillManager.GetSkillInformation(pItem->Special[i], 1, NULL, &iMana, NULL);
				GetSpecialOptionText(pItem->Type, TextList[LineNum], pItem->Special[i], pItem->SpecialValue[i], iMana);

				TextListColor[LineNum] = TEXT_COLOR_BLUE;
				TextBold[LineNum++] = false;


				if (pItem->Special[i] == AT_LUCK)
				{
					sprintf(TextList[LineNum], GlobalText[94], pItem->SpecialValue[i]);
					TextListColor[LineNum] = TEXT_COLOR_BLUE;
					TextBold[LineNum++] = false;
				}
				else if (pItem->Special[i] == AT_SKILL_RIDER)
				{
					sprintf(TextList[LineNum], GlobalText[179]);
					TextListColor[LineNum] = TEXT_COLOR_DARKRED;
					TextBold[LineNum++] = false;
				}
				else if (pItem->Special[i] != 62
					&& pItem->Special[i] != 510
					&& pItem->Special[i] != 516
					&& pItem->Special[i] != 512)
				{
					if (pItem->Special[i] == AT_IMPROVE_DAMAGE && (pItem->Type == ITEM_SWORD + 31
						|| pItem->Type == ITEM_SWORD + 21
						|| pItem->Type == ITEM_SWORD + 23
						|| pItem->Type == ITEM_SWORD + 25
						|| pItem->Type == ITEM_SWORD + 28))
					{
						sprintf(TextList[LineNum], GlobalText[89], pItem->SpecialValue[i]);
						TextListColor[LineNum] = TEXT_COLOR_BLUE;
						TextBold[LineNum++] = false;
					}
				}
				else
				{
					sprintf(TextList[LineNum], GlobalText[1201]);
					TextListColor[LineNum] = TEXT_COLOR_DARKRED;
					TextBold[LineNum++] = false;
				}
			}

			SkipLine++;

			if (pItem->bPeriodItem == true)
			{
				if (pItem->bExpiredPeriod == true)
				{
					sprintf(TextList[LineNum], GlobalText[3266]);
					TextListColor[LineNum++] = TEXT_COLOR_RED;
				}
				else
				{
					sprintf(TextList[LineNum], GlobalText[3265]);
					TextListColor[LineNum++] = TEXT_COLOR_ORANGE;

					sprintf(TextList[LineNum], "%d-%02d-%02d  %02d:%02d", ExpireTime->tm_year + 1900, ExpireTime->tm_mon + 1, ExpireTime->tm_mday, ExpireTime->tm_hour, ExpireTime->tm_min);
					TextListColor[LineNum++] = TEXT_COLOR_BLUE;
				}
			}

			if (!bItemTextListBoxUse)
			{
				SIZE TextSize;

				int PointedIndex = g_pMyInventory->GetPointedItemIndex();
				bool bThisisEquippedItem = false;

				if (PointedIndex == -1 || PointedIndex < EQUIPMENT_WEAPON_RIGHT || PointedIndex >= MAX_EQUIPMENT_INDEX)
					bThisisEquippedItem = true;

				LineNum = g_csItemOption.RenderSetOptionListInItem(pItem, LineNum, bThisisEquippedItem);
				int LineBack = g_csItemOption.m_ByLineEnd - g_csItemOption.m_ByLineIni;

				LineNum = g_SocketItemMgr.AttachToolTipForSocketItem(pItem, LineNum);

				TextSize.cx = 0;
				TextSize.cy = 0;

				float fRateY = g_fScreenRate_y / 1.3;

				g_pRenderText->SetFont(g_hFont);
				g_pMultiLanguage->_GetTextExtentPoint32(g_pRenderText->GetFontDC(), TextList[0], 1, &TextSize);

				float Height = (int)(((double)(TextSize.cy * ((LineNum - LineBack) - SkipLine)) + (double)(TextSize.cy * SkipLine) / 2.0) / fRateY);

				int iScreenHeight = (WinFrameY - 60);

				sy += INVENTORY_SCALE;
				if (sy + Height > iScreenHeight)
				{
					sy += iScreenHeight - (sy + Height);
				}
				RenderTipTextList(sx, sy, LineNum, 0, RT3_SORT_CENTER, STRP_NONE, TRUE);
			}
			else
			{
				RenderTipTextList(sx, sy, LineNum, 0, RT3_SORT_CENTER, STRP_BOTTOMCENTER);
			}
		}
		else
		{
			if (ItemType == ITEM_HELPER + 4)
			{
				if ((g_pMyInventory->GetPointedItemIndex()) == EQUIPMENT_HELPER)
				{
					SendRequestPetInfo(PET_TYPE_DARK_HORSE, InvenType, EQUIPMENT_HELPER);
				}
			}
			else
			{
				if ((g_pMyInventory->GetPointedItemIndex()) == EQUIPMENT_WEAPON_LEFT)
				{
					SendRequestPetInfo(PET_TYPE_DARK_SPIRIT, InvenType, EQUIPMENT_WEAPON_LEFT);
				}
			}
			giPetManager::RenderPetItemInfo(sx, sy, pItem, InvenType);
		}
	}
}

void CNewUIItemTooltip::SetTooltipValueText(char* Description, char* information, int NextLine, int* color, int NumText, ITEM* pItem, Script_Item* pItemAtrr, int iItemLevel)
{
	switch (NumText)
	{
	case 0:
	{
		int minindex = 0;
		int maxindex = 0;
		int magicalindex = 0;

		if (iItemLevel >= pItem->Jewel_Of_Harmony_OptionLevel)
		{
			StrengthenCapability SC;
			g_pUIJewelHarmonyinfo->GetStrengthenCapability(&SC, pItem, 1);

			if (SC.SI_isSP)
			{
				minindex = SC.SI_SP.SI_minattackpower;
				maxindex = SC.SI_SP.SI_maxattackpower;
				magicalindex = SC.SI_SP.SI_magicalpower;
			}
		}
		int DamageMin = pItem->DamageMin;
		int DamageMax = pItem->DamageMax;

		if (minindex + DamageMin < maxindex + DamageMax)
			sprintf(information, Description, minindex + DamageMin, maxindex + DamageMax);
		else
			sprintf(information, Description, maxindex + DamageMax, maxindex + DamageMax);

		if (DamageMin > 0)
		{
			if (minindex || maxindex)
			{
				*color = TEXT_COLOR_YELLOW;
			}
			else if ((pItem->Option1 & 63) > 0)
			{
				*color = TEXT_COLOR_BLUE;
			}
		}
	}
	break;
	case 1:
		sprintf(information, Description, pItem->Durability);
		break;
	case 2:
	{
		int maxdefense = 0;
		if (iItemLevel >= pItem->Jewel_Of_Harmony_OptionLevel)
		{
			StrengthenCapability SC;
			g_pUIJewelHarmonyinfo->GetStrengthenCapability(&SC, pItem, 2);

			if (SC.SI_isSD)
			{
				maxdefense = SC.SI_SD.SI_defense;
			}
		}
		sprintf(information, Description, maxdefense + pItem->Defense);
		if (maxdefense)
		{
			*color = TEXT_COLOR_YELLOW;
		}
		else if (pItem->Type >= 3584 && pItem->Type < 6144 && (pItem->Option1 & 63) > 0)
		{
			*color = TEXT_COLOR_BLUE;
		}
	}
	break;
	case 3:
		sprintf(information, Description, pItem->MagicDefense);
		break;
	case 4:
	{
		int maxDurability = calcMaxDurability(pItem, pItemAtrr, iItemLevel);
		sprintf(information, Description, pItem->Durability, maxDurability);
	}
	break;
	case 5:
	{
		WORD Level = 0;
		if (Hero->IsMasterLevel() == true)
		{
			Level = CharacterAttribute->Level + Master_Level_Data.nMLevel;
		}
		else
		{
			Level = CharacterAttribute->Level;
		}
		sprintf(information, Description, pItem->RequireLevel);
		if (Level < pItem->RequireLevel)
		{
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
		break;
	}
	case 6:
	{
		int iDecNeedStrength = 0;
		int iDecNeedDex = 0;
		CNewUIItemTooltip::SetNeedStrDex(&iDecNeedStrength, &iDecNeedDex, iItemLevel, pItem);
		sprintf(information, Description, pItem->RequireStrength - iDecNeedStrength);
	}
	break;
	case 7:
	{
		int iDecNeedStrength = 0;
		int iDecNeedDex = 0;
		CNewUIItemTooltip::SetNeedStrDex(&iDecNeedStrength, &iDecNeedDex, iItemLevel, pItem);
		sprintf(information, Description, pItem->RequireDexterity - iDecNeedDex);
	}
	break;
	case 8:
		sprintf(information, Description, pItem->RequireVitality);
		break;
	case 9:
		sprintf(information, Description, pItem->RequireEnergy);
		break;
	case 10:
		sprintf(information, Description, pItem->RequireCharisma);
		break;
	case 11:
		sprintf(information, Description, pItem->MagicPower);
		break;
	case 12:
	{
		int DamageMin = pItem->DamageMin;
		int DamageMax = pItem->DamageMax;

		if (pItem->Type >> 4 == 15)
		{
			sprintf(information, Description, DamageMin, DamageMin);
		}
		else
		{
			int SkillIndex = getSkillIndexByBook(pItem->Type);

			SKILL_ATTRIBUTE* skillAtt = &SkillAttribute[SkillIndex];

			DamageMin = skillAtt->Damage;
			DamageMax = skillAtt->Damage + (skillAtt->Damage * 0.5f);
			sprintf(information, Description, DamageMin, DamageMin);
		}

		if (DamageMin > 0 && (pItem->Option1 & 63) > 0)
			*color = TEXT_COLOR_BLUE;
	}
	break;
	case 13:
		if (m_ItemLevel == -1)
		{
			if (iItemLevel >= 5)
				sprintf(information, Description);
			else
				sprintf(information, "");
		}
		else
		{
			sprintf(information, Description);
		}
		break;
	case 14:
		sprintf(information, Description, iItemLevel + 1);
		break;
	case 15:
		sprintf(information, Description, pItem->SuccessfulBlocking);
		if ((pItem->Option1 & 63) > 0)
			*color = 1;
		break;
	case 16:
		sprintf(information, Description, 10 * pItem->Durability);
		break;
	case 101:
		sprintf(information, Description, pItemAtrr->WeaponSpeed);
		break;
	case 102:
		sprintf(information, Description, pItemAtrr->WalkSpeed);
		break;
	case 201:
	{
		WORD Level = 0;
		if (Hero->IsMasterLevel() == true)
		{
			Level = CharacterAttribute->Level + Master_Level_Data.nMLevel;
		}
		else
		{
			Level = CharacterAttribute->Level;
		}
		if (Level < pItem->RequireLevel)
		{
			sprintf(information, Description, pItem->RequireLevel - Level);
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
		else
		{
			sprintf(information, "");
		}
	}
	break;
	case 202:
	{
		int iDecNeedStrength = 0;
		int iDecNeedDex = 0;
		SetNeedStrDex(&iDecNeedStrength, &iDecNeedDex, iItemLevel, pItem);

		int iDecStrength = CharacterAttribute->AddStrength + CharacterAttribute->Strength;

		if (iDecStrength >= (pItem->RequireStrength - iDecNeedStrength))
		{
			sprintf(information, "");
			if (iDecNeedStrength) *color = TEXT_COLOR_YELLOW;
		}
		else
		{
			sprintf(information, Description, (pItem->RequireStrength - iDecStrength - iDecNeedStrength));
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
	}
	break;
	case 203:
	{
		int iDecNeedStrength = 0;
		int iDecNeedDex = 0;
		SetNeedStrDex(&iDecNeedStrength, &iDecNeedDex, iItemLevel, pItem);

		int iDecDex = CharacterAttribute->AddDexterity + CharacterAttribute->Dexterity;

		if (iDecDex >= (pItem->RequireDexterity - iDecNeedDex))
		{
			sprintf(information, "");
			if (iDecNeedDex) *color = TEXT_COLOR_YELLOW;
		}
		else
		{
			sprintf(information, Description, (pItem->RequireDexterity - iDecDex - iDecNeedDex));
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
	}
	break;
	case 204:
	{
		int bRequireStat = CharacterAttribute->AddVitality + CharacterAttribute->Vitality;

		if (bRequireStat >= pItem->RequireVitality)
		{
			sprintf(information, "");
		}
		else
		{
			sprintf(information, Description, (pItem->RequireVitality - bRequireStat));
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
	}
	break;
	case 205:
	{
		int bRequireStat = CharacterAttribute->AddEnergy + CharacterAttribute->Energy;

		if (bRequireStat >= pItem->RequireEnergy)
		{
			sprintf(information, "");
		}
		else
		{
			sprintf(information, Description, (pItem->RequireEnergy - bRequireStat));
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
	}
	break;
	case 206:
	{
		int bRequireStat = CharacterAttribute->AddCharisma + CharacterAttribute->Charisma;

		if (bRequireStat >= pItem->RequireCharisma)
		{
			sprintf(information, "");
		}
		else
		{
			sprintf(information, Description, (pItem->RequireCharisma - bRequireStat));
			*color = TEXT_COLOR_RED;
			TextListColor[NextLine - 1] = *color;
		}
	}
	break;
	case 207:
		sprintf(information, Description, iItemLevel + 32);
		break;
	case 208:
		sprintf(information, Description, 2 * iItemLevel + 39);
		break;
	case 209:
		sprintf(information, Description, 2 * iItemLevel + 20);
		break;
	case 210:
		sprintf(information, Description, 2 * iItemLevel + 12);
		break;
	case 211:
		sprintf(information, Description, 2 * iItemLevel + 25);
		break;
	case 212:
		sprintf(information, Description, 2 * iItemLevel + 24);
		break;
	case 213:
		sprintf(information, Description, 2 * iItemLevel + 10);
		break;
	case 214:
		sprintf(information, Description, iItemLevel + 10);
		break;
	case 215:
	{
#ifdef LEM_ADD_SEASON5_PART5_MINIUPDATE_JEWELMIX
		char tCount = COMGEM::CalcCompiledCount(pItem);
		if (tCount > 0)
		{
			int	nJewelIndex = COMGEM::Check_Jewel_Com(pItem->Type);
			if (nJewelIndex != COMGEM::NOGEM)
			{
				sprintf(information, Description, tCount, COMGEM::GetComGemTypeName(pItem->Type));
			}
		}
#endif
	}
	break;
	case 216:
	{
		int iSocketSeedID = GetSocketSeedID(pItem->Type, iItemLevel);

		char szOptionName[64] = { '\0', };

		g_SocketItemMgr.GetSocketOptionNameText(szOptionName, 0, iSocketSeedID);
		sprintf(information, Description, szOptionName);
	}
	break;
	case 217:
	{
		int iSocketSeedID = GetSocketSeedID(pItem->Type, iItemLevel);
		char szOptionValueText[16] = { '\0', };

		g_SocketItemMgr.GetSocketOptionValueText(szOptionValueText, 0, iSocketSeedID, pItem->Type);
		sprintf(information, Description, szOptionValueText);
	}
	break;
	case 218:
		sprintf(information, Description, CharacterAttribute->Level);
		break;
	case 219:
	{
		const ITEM_ADD_OPTION& Item_data = g_pItemAddOptioninfo->GetItemAddOtioninfo(pItem->Type);
		sprintf(information, Description, Item_data.m_byValue1);
	}
	break;
	case 220:
	{
		const ITEM_ADD_OPTION& Item_data = g_pItemAddOptioninfo->GetItemAddOtioninfo(pItem->Type);
		sprintf(information, Description, Item_data.m_byValue2);
	}
	break;
	case 221:
	{
		std::string timetext;
		const ITEM_ADD_OPTION& Item_data = g_pItemAddOptioninfo->GetItemAddOtioninfo(pItem->Type);
		g_StringTime(Item_data.m_Time, timetext, true);
		sprintf(information, Description, timetext.c_str());
	}
	break;
	case 222:
		if (pItem->Durability == 254)
			sprintf(information, Description);
		else
			sprintf(information, "");
		break;
	case 223:
		sprintf(information, Description, 5 - pItem->Durability);
		break;
	case 224:
		if (g_PortalMgr.IsRevivePositionSaved() == 1)
		{
			int iMap = g_PortalMgr.GetReviveWorld();
			int x = g_PortalMgr.GetRevivePosX();
			int y = g_PortalMgr.GetRevivePosY();
			sprintf(information, Description, gMapManager.GetMapName(iMap), x, y);
		}
		else
		{
			sprintf(information, "");
		}
		break;
	case 225:
		sprintf(information, Description, CharacterAttribute->Level >> 1, CharacterAttribute->Level >> 1);
		break;
	case 226:
		sprintf(information, Description, CharacterAttribute->Level / 12, CharacterAttribute->Level / 25);
		break;
	case 103:
		{
			BYTE RankedMuun = pItem->SocketSeedSetOption;
			if(pItemAtrr->IsKind1(ItemKind1::MUUN_INVENTORY) && RankedMuun != 0xFF)
			{
				sprintf(information, Description, "O");
				for (int i = 0; i < RankedMuun; i++)
				{
					strcat(information, "O");
				}
			}
		}
		break;
	case 104:
		{
			BYTE RankedMuun = pItem->SocketSeedSetOption;
			if(pItemAtrr->IsKind1(ItemKind1::MUUN_INVENTORY) && RankedMuun != 0xFF)
			{
				sprintf(information, Description, (pItem->Level >> 3) & 0xF, RankedMuun + 1);
			}
		}
		break;
	case 105:
		//--muun option condition
		break;
	case 106:
		//--muun option condition date
		break;
	case 107:
		//--muun option value time date
		break;
	case 227:
		sprintf(information, Description, pItem->SocketCount);
		break;
	case 228:
		sprintf(information, Description, iItemLevel + 21);
		break;
	case 229:
		sprintf(information, Description, iItemLevel + 33);
		break;
	case 230:
		sprintf(information, Description, iItemLevel + 35);
		break;
	case 231:
		sprintf(information, Description, 2 * iItemLevel + 13);
		break;
	case 232:
		sprintf(information, Description, 2 * iItemLevel + 30);
		break;
	case 233:
		sprintf(information, Description, 2 * iItemLevel + 29);
		break;
	case 234:
		sprintf(information, Description, 71);
		break;
	case 235:
		sprintf(information, Description, iItemLevel + 60);
		break;
	case 236:
		sprintf(information, Description, iItemLevel + 80);
		break;
	case 237:
		sprintf(information, Description, iItemLevel + 75);
		break;
	case 238:
		sprintf(information, Description, iItemLevel + 100);
		break;
	case 239:
		sprintf(information, Description, iItemLevel + 55);
		break;
	case 240:
		sprintf(information, Description, 2 * iItemLevel + 43);
		break;
	case 241:
		sprintf(information, Description, 2 * iItemLevel + 37);
		break;
	case 242:
	case 243:
	case 244:
	case 245:
	{
		int value = 100 + pow(iItemLevel, 1.2) * 11.8;

		sprintf(information, Description, value);
	}
	break;
	case 246:
	{
		int value = 140 + pow(iItemLevel, 1.5) * 15;

		if (iItemLevel > 9)
		{
			value += pow(pow((iItemLevel - 10), 1.3) * 5, 1.6);
		}
		sprintf(information, Description, value);
	}
	break;
	case 247:
		sprintf(information, Description, pItem->DamageMin, pItem->DamageMax);
		break;
	case 249:
		sprintf(information, Description, pItem->DamageMin, pItem->DamageMax);
		break;
	case 251:
	{
		switch (pItem->SocketSeedSetOption)
		{
		case 1:
			(*color) = 14;
			break;
		case 2:
			(*color) = 2;
			break;
		case 3:
			(*color) = 1;
			break;
		case 4:
			(*color) = 3;
			break;
		case 5:
			(*color) = 9;
			break;
		}
	}
	break;
	case 252:
	{
		switch (pItem->SocketSeedSetOption)
		{
		case 1:
			(*color) = 14;
			break;
		case 2:
			(*color) = 2;
			break;
		case 3:
			(*color) = 1;
			break;
		case 4:
			(*color) = 3;
			break;
		case 5:
			(*color) = 9;
			break;
		}
		//sprintf(information, Description, iItemLevel + 75);
	}
	break;
	default:
		sprintf(information, Description);
		break;
	}
}

void CNewUIItemTooltip::SetNeedStrDex(int* iDecNeedStrength, int* iDecNeedDex, int iItemLevel, ITEM* pItem)
{
	if (iItemLevel >= pItem->Jewel_Of_Harmony_OptionLevel)
	{
		StrengthenCapability SC;
		g_pUIJewelHarmonyinfo->GetStrengthenCapability(&SC, pItem, 0);

		if (SC.SI_isNB)
		{
			*iDecNeedStrength = SC.SI_NB.SI_force;
			*iDecNeedDex = SC.SI_NB.SI_activity;
		}
	}
	if (pItem->SocketCount > 0)
	{
		for (int i = 0; i < pItem->SocketCount; ++i)
		{
			if (pItem->SocketSeedID[i] == 38)
			{
				int iReqStrengthDown = g_SocketItemMgr.GetSocketOptionValue(pItem, i);
				*iDecNeedStrength += iReqStrengthDown;
			}
			else if (pItem->SocketSeedID[i] == 39)
			{
				int iReqDexterityDown = g_SocketItemMgr.GetSocketOptionValue(pItem, i);
				*iDecNeedDex += iReqDexterityDown;
			}
		}
	}
}

int CNewUIItemTooltip::GetSocketSeedID(__int16 Type, int iLevel)
{
	int iCategoryIndex;
	int bySocketSeedID = 0;

	iCategoryIndex = (Type - (ITEM_WING + 59));

	if (!InBounds(iCategoryIndex, 1, 6))
		iCategoryIndex = (Type - (ITEM_WING + 100)) % 6 + 1;

	switch (iCategoryIndex)
	{
	case 1:
		bySocketSeedID = iLevel;
		break;
	case 2:
		bySocketSeedID = iLevel + 10;
		break;
	case 3:
		bySocketSeedID = iLevel + 16;
		break;
	case 4:
		bySocketSeedID = iLevel + 21;
		break;
	case 5:
		bySocketSeedID = iLevel + 29;
		break;
	case 6:
		bySocketSeedID = iLevel + 34;
		break;
	}
	return bySocketSeedID;
}

void CNewUIItemTooltip::ItemEquipClass(Script_Item* pItemAttr, int* NextLine, int* SkipNum)
{
	int iTextColor;

	if (pItemAttr)
	{
		BYTE byFirstClass = Hero->GetBaseClass();
		BYTE byStepClass = Hero->GetStepClass();

		TextListColor[*NextLine + 2] = TEXT_COLOR_WHITE;
		TextListColor[*NextLine + 3] = TEXT_COLOR_WHITE;

		sprintf_s(TextList[*NextLine], "\n");
		++(*NextLine);
		++(*SkipNum);

		int iCount = 0;

		for (int i = 0; i < MAX_CLASS; ++i)
		{
			if (pItemAttr->RequireClass[i] == 1)
				++iCount;
		}

		if (iCount == MAX_CLASS)
			return;

		for (int i = 0; i < MAX_CLASS; ++i)
		{
			BYTE byRequireClass = pItemAttr->RequireClass[i];

			if (byRequireClass == 0)
				continue;

			if (i == byFirstClass && byRequireClass <= byStepClass)
			{
				iTextColor = TEXT_COLOR_WHITE;
			}
			else
			{
				iTextColor = TEXT_COLOR_DARKRED;
			}

			int NameClass = 2305;

			switch (i)
			{
			case Dark_Wizard:
				if (byRequireClass == 1)
				{
					NameClass = 20;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 25;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 1669;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3186;
				}
				break;
			case Dark_Knight:
				if (byRequireClass == 1)
				{
					NameClass = 21;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 26;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 1668;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3187;
				}
				break;
			case Fairy_Elf:
				if (byRequireClass == 1)
				{
					NameClass = 22;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 27;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 1670;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3188;
				}
				break;
			case Magic_Gladiator:
				if (byRequireClass == 1)
				{
					NameClass = 23;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 2305;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 1671;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3189;
				}
				break;
			case Dark_Lord:
				if (byRequireClass == 1)
				{
					NameClass = 24;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 2305;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 1672;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3190;
				}
				break;
			case Summoner:
				if (byRequireClass == 1)
				{
					NameClass = 1687;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 1688;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 1689;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3191;
				}
				break;
			case Rage_Fighter:
				if (byRequireClass == 1)
				{
					NameClass = 3150;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 2305;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3151;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3192;
				}
				break;
			case Grow_Lancer:
				if (byRequireClass == 1)
				{
					NameClass = 3175;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 2305;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3176;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3177;
				}
				break;
			case Runer_Wizzard:
				if (byRequireClass == 1)
				{
					NameClass = 3179;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 3181;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3182;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3183;
				}
				break;
			case Slayer:
				if (byRequireClass == 1)
				{
					NameClass = 3193;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 3194;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3195;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3196;
				}
				break;
			case Gun_Crusher:
				if (byRequireClass == 1)
				{
					NameClass = 3200;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 3201;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3202;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3203;
				}
				break;
			case Mage_Kundun:
				if (byRequireClass == 1)
				{
					NameClass = 3218;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 3219;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3220;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3221;
				}
				break;
			case Mage_Lemuria:
				if (byRequireClass == 1)
				{
					NameClass = 3213;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 3214;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3215;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3216;
				}
				break;
			case Illusion_Knight:
				if (byRequireClass == 1)
				{
					NameClass = 3209;
				}
				else if (byRequireClass == 2)
				{
					NameClass = 3210;
				}
				else if (byRequireClass == 3)
				{
					NameClass = 3211;
				}
				else if (byRequireClass == 4)
				{
					NameClass = 3212;
				}
				break;
			}

			sprintf_s(TextList[(*NextLine)], GlobalText[61], GlobalText[NameClass]);
			TextListColor[(*NextLine)] = iTextColor;
			TextBold[(*NextLine)++] = 0;
		}
	}
}

BOOL CNewUIItemTooltip::IsAbsoluteWeapon(__int16 Type)
{
	int WeaponKind3 = GMItemMng->GetKind3(Type);
	
	if (WeaponKind3 == ItemKind3::ARCHANGEL_WEAPON || WeaponKind3 == ItemKind3::BLESSED_ARCHANGEL_WEAPON)
		return true;
	
	return false;
}

BOOL CNewUIItemTooltip::IsEnableOptionWing(__int16 Type)
{
	int WeaponKind2 = GMItemMng->GetKind2(Type);

	if (WeaponKind2 == ItemKind2::WINGS_LVL_2
		|| WeaponKind2 == ItemKind2::WINGS_LVL_3
		|| WeaponKind2 == ItemKind2::WINGS_LVL_4
		)
	{
		return true;
	}
	return false;
}

_ITEM_TOOLTIP_DATA* CNewUIItemTooltip::FindTooltip(int Type)
{
	if (Type >= 0 && Type < MAX_ITEM)
		return &m_ItemTooltip[Type];
	else
		return NULL;
}

_ITEM_TOOLTIP_TEXT* CNewUIItemTooltip::FindTooltipText(int Index)
{
	if (Index >= 0 && Index < MAX_TEXT_TOOLTIP)
		return &m_ItemTooltipText[Index];
	else
		return NULL;
}

_ITEM_LEVEL_TOOTIP_DATA* CNewUIItemTooltip::FindLevelTooltip(int Level)
{
	if (Level >= 0 && Level < MAX_LEVEL_TOOLTIP)
		return &m_ItemTooltipLevel[Level];
	else
		return NULL;
}

bool CNewUIItemTooltip::InBounds(int index, int min, int max)
{
	bool success;

	if (index >= min && index <= max)
		success = true;
	else
		success = false;

	return success;
}

int CNewUIItemTooltip::SplitText(char* text, int a2, int TextNum, int Color, int Bold, bool a6)
{
	if (NULL == text)
	{
		return TextNum;
	}
	char buf[0xA00] = { 0 };
	int nLine = 0;
	if (a2)
	{
		if (1 == a2)
		{
			nLine = ::DivideStringByPixel(buf, 10, 256, text, 200, true, '#');
		}
	}
	else
	{
		nLine = ::DivideStringByPixel(buf, 10, 256, text, 150, true, '#');
	}
	for (int i = 0; i < nLine; i++)
	{
		TextListColor[TextNum] = Color;
		TextBold[TextNum] = Bold;
		std::string str(&buf[256 * i]);
		if (true == a6)
		{
			for (int j = str.find("%", 0); j != -1; j = str.find("%", j + 2))
			{
				str.insert(j, "%");
			}
		}
		unicode::_sprintf(TextList[TextNum++], str.data());
	}

	return TextNum;
}

void CNewUIItemTooltip::CreateItemName(ITEM_t* pItemMap)
{
	ITEM* pItem = &pItemMap->Item;

	if (!pItem || pItem->Type == -1)
		return;

	int iLevelTooltip = -1;
	int ItemLevel = (pItem->Level >> 3) & 0xF;
	int ItemOption = (pItem->Option1);
	int ItemExtOption = (pItem->ExtOption);

	Script_Item* pItemAttr = GMItemMng->find(pItem->Type);

	if (!pItemAttr)
		return;

	if (pItem->Type == ITEM_POTION + 15)
	{
		sprintf(pItemMap->Name, "%s %d", pItemAttr->Name, pItem->Level);
		pItemMap->Color = TEXT_COLOR_YELLOW;
		pItemMap->Bold = TRUE;
	}
	else
	{
		_ITEM_TOOLTIP_DATA* mcTooltip = FindTooltip(pItem->Type);

		if (mcTooltip)
		{
			_ITEM_LEVEL_TOOTIP_DATA* mcTooltipLevel = NULL;

			if (mcTooltip->ItemLevel >= 0)
			{
				int FindLevel;

				if (pItem->Type == ITEM_HELPER + 37)
				{
					switch (ItemOption)
					{
					case 1:
						iLevelTooltip = 1;
						break;
					case 2:
						iLevelTooltip = 2;
						break;
					case 4:
						iLevelTooltip = 3;
						break;
					case 8:
						iLevelTooltip = 4;
						break;
					case 16:
						iLevelTooltip = 5;
						break;
					default:
						iLevelTooltip = 0;
						break;
					}

					FindLevel = mcTooltip->ItemLevel + iLevelTooltip;
				}
				else
				{
					FindLevel = mcTooltip->ItemLevel + ItemLevel;
				}
				mcTooltipLevel = FindLevelTooltip(FindLevel);

				if (!mcTooltipLevel)
				{
					return;
				}
			}

			int color = TEXT_COLOR_GREEN_BLUE;

			if (IsAbsoluteWeapon(pItem->Type))
			{
				if (mcTooltip->ItemLevel == -1)
				{
					color = mcTooltip->ColorName;

				}
				else if (!mcTooltipLevel)
				{
					color = mcTooltipLevel->ColorName;
				}
			}
			else if (IsEnableOptionWing(pItem->Type) == 1)
			{
				if (ItemLevel < 7)
					color = mcTooltip->ColorName;
				else
					color = TEXT_COLOR_YELLOW;
			}
			else if ((pItem->ExtOption % 0x04) != EXT_A_SET_OPTION && (pItem->ExtOption % 0x04) != EXT_B_SET_OPTION)
			{
				if (g_SocketItemMgr.IsSocketItem(pItem) == 1)
				{
					color = TEXT_COLOR_VIOLET;
				}
				else if (pItem->SpecialNum <= 0 || (pItem->Option1 & 63) <= 0)
				{
					if (ItemLevel < 7)
					{
						if (mcTooltip->ItemLevel == -1)
						{
							if (pItem->SpecialNum <= 0)
								color = mcTooltip->ColorName;
							else
								color = TEXT_COLOR_BLUE;
						}
						else if (!mcTooltipLevel)
						{
							color = mcTooltipLevel->ColorName;
						}
					}
					else
					{
						color = TEXT_COLOR_YELLOW;
					}
				}
				else
				{
					color = TEXT_COLOR_GREEN;
				}
			}
			else
			{
				color = TEXT_COLOR_GREEN_BLUE;
			}

			pItemMap->Color = color;

			if (pItem->Type != ITEM_HELPER + 4 && pItem->Type != ITEM_HELPER + 5)
			{
				char TextName[100];

				memset(TextName, 0, sizeof(TextName));

				if (!g_csItemOption.GetSetItemName(TextName, pItem->Type, pItem->ExtOption) || IsAbsoluteWeapon(pItem->Type))
				{
					strcat(TextName, mcTooltip->Name);
				}
				else
				{
					strcat(TextName, mcTooltip->Name);
				}

				if ((pItem->Option1 & 0x3F) <= 0 || IsAbsoluteWeapon(pItem->Type) || IsEnableOptionWing(pItem->Type))
				{
					if (mcTooltip->ItemLevel != -1 && mcTooltipLevel && mcTooltipLevel->Name[0] != '\0')
					{
						sprintf(pItemMap->Name, mcTooltipLevel->Name);
					}
					else
					{
						if (ItemLevel && mcTooltip->RenderLevel)
							sprintf(pItemMap->Name, "%s +%d", TextName, ItemLevel);
						else
							sprintf(pItemMap->Name, TextName);
					}
				}
				else if (iLevelTooltip == -1)
				{
					if (ItemLevel)
						sprintf(pItemMap->Name, "%s %s +%d", GlobalText[620], TextName, ItemLevel);
					else
						sprintf(pItemMap->Name, "%s %s", GlobalText[620], TextName);
					pItemMap->Color = TEXT_COLOR_GREEN;
				}
				else
				{
					if (mcTooltipLevel && mcTooltipLevel->Name[0] != '\0')
					{
						sprintf(pItemMap->Name, mcTooltipLevel->Name);
					}
					color = TEXT_COLOR_BLUE;
				}

				if ((pItem->Level >> 7) & 1)
				{
					if (pItem->Type != ITEM_HELPER + 3)
					{
						strcat(pItemMap->Name, GlobalText[176]);
					}
					else
					{
						strcat(pItemMap->Name, " +");
						strcat(pItemMap->Name, GlobalText[179]);
					}
				}

				if ((pItem->Level & 3) || ((pItem->Option1 >> 6) & 1))
					strcat(pItemMap->Name, GlobalText[177]);

				if ((pItem->Level >> 2) & 1)
					strcat(pItemMap->Name, GlobalText[178]);


				if ((pItem->ExtOption % 0x04) != EXT_A_SET_OPTION && (pItem->ExtOption % 0x04) != EXT_B_SET_OPTION && g_SocketItemMgr.IsSocketItem(pItem) == false)
				{
					if (!IsAbsoluteWeapon(pItem->Type) && !IsEnableOptionWing(pItem->Type))
					{
						if ((ItemOption & 63) > 0)
						{
							color = TEXT_COLOR_GREEN;
						}
						else if(ItemLevel < 7)
						{
							if ((pItem->Level & 3) || ((ItemOption >> 6) & 1))
								color = TEXT_COLOR_BLUE;

							if ((pItem->Level >> 2) & 1)
								color = TEXT_COLOR_BLUE;
						}
					}
				}

				pItemMap->Color = color;

				if (mcTooltip->ItemLevel != -1 && mcTooltipLevel)
					pItemMap->Bold = mcTooltipLevel->IsBold;
				else
					pItemMap->Bold = mcTooltip->IsBold;
			}
		}
		else
		{
			pItemMap->Color = TEXT_COLOR_WHITE;
			sprintf(pItemMap->Name, "%s +%d", pItemAttr->Name, pItem->Level);
		}
	}
}
