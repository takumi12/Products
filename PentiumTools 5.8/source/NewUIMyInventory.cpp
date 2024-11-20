// NewUIMyInventory.cpp: implementation of the CNewUIMyInventory class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUIMyInventory.h"
#include "NewUISystem.h"
#include "NewUICustomMessageBox.h"
#include "CSChaosCastle.h"
#include "GOBoid.h"
#include "ZzzEffect.h"
#include "GIPetManager.h"
#include "CSParts.h"
#include "UIJewelHarmony.h"
#include "CDirection.h"
#include "GMCryWolf1st.h"
#include "GMCryingWolf2nd.h"
#include "ZzzInventory.h"
#include "wsclientinline.h"
#include "MixMgr.h"
#include "ZzzLodTerrain.h"
#include "CSQuest.h"
#include "UIGuildInfo.h"
#include "UIManager.h"
#include "CSItemOption.h"
#include "MapManager.h"
#include "w_PetProcess.h"
#include "SocketSystem.h"
#include "w_CursedTemple.h"
#include "PortalMgr.h"
#ifdef CSK_FIX_BLUELUCKYBAG_MOVECOMMAND
#include "Event.h"
#endif // CSK_FIX_BLUELUCKYBAG_MOVECOMMAND
#include "GMUnitedMarketPlace.h"
#include "ChangeRingManager.h"
#ifdef PBG_ADD_NEWCHAR_MONK
#include "MonkSystem.h"
#endif //PBG_ADD_NEWCHAR_MONK
#include "CharacterManager.h"

using namespace SEASON3B;

SEASON3B::CNewUIMyInventory::CNewUIMyInventory()
{
	m_pNewUIMng = NULL;
	m_pNewUI3DRenderMng = NULL;

	for (int i = 0; i < MAX_INVENT_EXPANSION + 1; i++)
		m_pNewInventoryCtrl[i] = NULL;

	m_Pos.x = m_Pos.y = 0;

	memset(&m_EquipmentSlots, 0, sizeof(EQUIPMENT_ITEM) * MAX_EQUIPMENT_INDEX);
	m_iPointedSlot = -1;

	m_MyShopMode = MYSHOP_MODE_OPEN;
	m_RepairMode = REPAIR_MODE_OFF;
	m_dwStandbyItemKey = 0;

	m_bRepairEnableLevel = false;
	m_bMyShopOpen = false;

	m_iBackPointed = m_iPointedSlot;
	this->ExpansionInit();

	m_nBackupMouseX = 0;
	m_nBackupMouseY = 0;
	m_bItemAutoMove = false;
	m_bInventoryBase = NULL;
}

SEASON3B::CNewUIMyInventory::~CNewUIMyInventory()
{
	Release();
}

bool SEASON3B::CNewUIMyInventory::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y)
{
	if (NULL == pNewUIMng || NULL == pNewUI3DRenderMng || NULL == g_pNewItemMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_INVENTORY, this);

	m_pNewUI3DRenderMng = pNewUI3DRenderMng;
	m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);
	m_pNewUI3DRenderMng->Add3DRenderObj(this, TOOLTIP_CAMERA_Z_ORDER);

	for (int i = 0; i < MAX_INVENT_EXPANSION + 1; i++)
	{
		m_pNewInventoryCtrl[i] = new CNewUIInventoryCtrl;

		if (i)
		{
			if (false == m_pNewInventoryCtrl[i]->Create(m_pNewUI3DRenderMng, g_pNewItemMng, this, x + 15, y + 200, 8, 4))
			{
				SAFE_DELETE(m_pNewInventoryCtrl[i]);
				return false;
			}
		}
		else
		{
			if (false == m_pNewInventoryCtrl[i]->Create(m_pNewUI3DRenderMng, g_pNewItemMng, this, x + 15, y + 200, 8, 8))
			{
				SAFE_DELETE(m_pNewInventoryCtrl[i]);
				return false;
			}
		}
	}

	SetPos(x, y);
	LoadImages();
	SetEquipmentSlotInfo();
	SetButtonInfo();
	Show(false);
	return true;
}

void SEASON3B::CNewUIMyInventory::Release()
{
	if (m_pNewUI3DRenderMng)
		m_pNewUI3DRenderMng->DeleteUI2DEffectObject(UI2DEffectCallback);

	UnequipAllItems();
	DeleteAllItems();

	UnloadImages();

	for (int i = 0; i < MAX_INVENT_EXPANSION + 1; i++)
	{
		SAFE_DELETE(m_pNewInventoryCtrl[i]);
	}

	if (m_pNewUI3DRenderMng)
	{
		m_pNewUI3DRenderMng->Remove3DRenderObj(this);
		m_pNewUI3DRenderMng = NULL;
	}
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}

	m_ExpansionBack.clear();
}

bool SEASON3B::CNewUIMyInventory::EquipItem(int iIndex, BYTE* pbyItemPacket)
{
	if (iIndex >= 0 && iIndex < MAX_EQUIPMENT_INDEX && g_pNewItemMng && CharacterMachine)
	{
		ITEM* pTargetItemSlot = &CharacterMachine->Equipment[iIndex];
		if (pTargetItemSlot->Type > 0)
		{
			UnequipItem(iIndex);
		}

		ITEM* pTempItem = g_pNewItemMng->CreateItem(pbyItemPacket);

		if (NULL == pTempItem)
		{
			return false;
		}

		if (pTempItem->Type == ITEM_HELPER + 4)
			SendRequestPetInfo(PET_TYPE_DARK_HORSE, 0, iIndex);

		if (pTempItem->Type == ITEM_HELPER + 5)
		{
			giPetManager::CreatePetDarkSpirit(Hero);
			SendRequestPetInfo(PET_TYPE_DARK_SPIRIT, 0, iIndex);
		}

		pTempItem->lineal_pos = iIndex;
		pTempItem->ex_src_type = ITEM_EX_SRC_EQUIPMENT;
		memcpy(pTargetItemSlot, pTempItem, sizeof(ITEM));
		g_pNewItemMng->DeleteItem(pTempItem);

		CreateEquippingEffect(pTargetItemSlot);
	}
	return false;
}

void SEASON3B::CNewUIMyInventory::UnequipItem(int iIndex)
{
	if (iIndex >= 0 && iIndex < MAX_EQUIPMENT_INDEX && g_pNewItemMng && CharacterMachine)
	{
		ITEM* pEquippedItem = &CharacterMachine->Equipment[iIndex];

		if (pEquippedItem && pEquippedItem->Type != -1)
		{
			if (pEquippedItem->Type == ITEM_HELPER + 4)
			{
				Hero->InitPetInfo(PET_TYPE_DARK_HORSE);
			}
			else if (pEquippedItem->Type == ITEM_HELPER + 5)
			{
				giPetManager::DeletePet(Hero);
				Hero->InitPetInfo(PET_TYPE_DARK_SPIRIT);
			}

			if (pEquippedItem->Type != ITEM_HELPER + 5)
				DeleteEquippingEffectBug(pEquippedItem);

			pEquippedItem->Type = -1;
			pEquippedItem->Level = 0;
			pEquippedItem->Number = -1;
			pEquippedItem->Option1 = 0;
			pEquippedItem->ExtOption = 0;
			pEquippedItem->SocketCount = 0;
			for (int i = 0; i < MAX_SOCKETS; ++i)
			{
				pEquippedItem->SocketSeedID[i] = SOCKET_EMPTY;
				pEquippedItem->SocketSphereLv[i] = 0;
			}
			pEquippedItem->SocketSeedSetOption = 0;
			DeleteEquippingEffect();
		}
	}
}
void SEASON3B::CNewUIMyInventory::UnequipAllItems()
{
	if (CharacterMachine)
	{
		for (int i = 0; i < MAX_EQUIPMENT_INDEX; i++)
		{
			UnequipItem(i);
		}
	}
}

bool SEASON3B::CNewUIMyInventory::IsEquipable(int iIndex, ITEM* pItem)
{
	if (pItem == NULL)
		return false;

	bool bEquipable = false;
	Script_Item* pItemAttr = GMItemMng->find(pItem->Type);

	if (pItemAttr->RequireClass[Hero->GetBaseClass()])
	{
		bEquipable = true;
	}
	else if (Hero->GetBaseClass() == CLASS_DARK && pItemAttr->RequireClass[CLASS_WIZARD] && pItemAttr->RequireClass[CLASS_KNIGHT])
	{
		bEquipable = true;
	}

	BYTE byFirstClass = Hero->GetBaseClass();
	BYTE byStepClass = Hero->GetStepClass();

	if (pItemAttr->RequireClass[byFirstClass] > byStepClass)
		return false;

	if (bEquipable == false)
		return false;

	bEquipable = false;
	if (pItemAttr->m_byItemSlot == iIndex)
	{
		bEquipable = true;
	}
	else if (pItemAttr->m_byItemSlot || iIndex != EQUIPMENT_WEAPON_LEFT)
	{
		if(pItemAttr->m_byItemSlot == EQUIPMENT_RING_RIGHT && iIndex == EQUIPMENT_RING_LEFT)
			bEquipable = true;
	}
	else if ( Hero->GetBaseClass() != CLASS_KNIGHT
		&& Hero->GetBaseClass() != CLASS_DARK
		&& Hero->GetBaseClass() != CLASS_RAGEFIGHTER )
	{
		if (Hero->GetBaseClass() == CLASS_SUMMONER && (pItem->Type < ITEM_STAFF || pItem->Type > ITEM_STAFF + MAX_ITEM_INDEX))
		{
			bEquipable = true;
		}
	}
	else
	{
		if (pItemAttr->TwoHand == true)
			return false;

		if (iIndex == EQUIPMENT_WEAPON_LEFT)
		{
			ITEM* l = &CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT];
			if (l->Type != -1 && pItemAttr->TwoHand == true)
			{
				return false;
			}
		}

		bEquipable = true;
	}

	if (Hero->GetBaseClass() == CLASS_ELF)
	{
		int WeaponKind2 = GMItemMng->GetKind2(pItem->Type);

		if (iIndex == EQUIPMENT_WEAPON_RIGHT)
		{
			ITEM* left = &CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT];

			int Weapon2Kind2 = GMItemMng->GetKind2(left->Type);

			if (left->Type != -1)
			{
				if (WeaponKind2 == ItemKind2::BOW && (Weapon2Kind2 != ItemKind2::ARROW && Weapon2Kind2 != ItemKind2::QUIVER))
				{
					bEquipable = false;
				}
				else if (WeaponKind2 == ItemKind2::CROSSBOW && (Weapon2Kind2 != ItemKind2::BOLT && Weapon2Kind2 != ItemKind2::QUIVER))
				{
					bEquipable = false;
				}
			}
		}
		else if (iIndex == EQUIPMENT_WEAPON_LEFT)
		{
			ITEM* right = &CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT];

			int Weapon2Kind2 = GMItemMng->GetKind2(right->Type);

			if (right->Type != -1)
			{
				if (WeaponKind2 == ItemKind2::ARROW && Weapon2Kind2 != ItemKind2::BOW)
				{
					bEquipable = false;
				}
				else if (WeaponKind2 == ItemKind2::BOLT && Weapon2Kind2 != ItemKind2::CROSSBOW)
				{
					bEquipable = false;
				}
				else if (WeaponKind2 == ItemKind2::QUIVER && (Weapon2Kind2 != ItemKind2::CROSSBOW && Weapon2Kind2 != ItemKind2::BOW))
				{
					bEquipable = false;
				}
			}
		}
	}

	if (Hero->GetBaseClass() == CLASS_RAGEFIGHTER)
	{
		if (iIndex == EQUIPMENT_GLOVES)
		{
			bEquipable = false;
		}
		else if (!pItemAttr->m_byItemSlot)
		{
			bEquipable = g_CMonkSystem.RageEquipmentWeapon(iIndex, pItem->Type);
		}
	}

	if (bEquipable == false)
		return false;

	DWORD wStrength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
	DWORD wDexterity = CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity;
	DWORD wEnergy = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;
	DWORD wVitality = CharacterAttribute->Vitality + CharacterAttribute->AddVitality;
	DWORD wCharisma = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;
	DWORD wLevel = CharacterAttribute->Level + Master_Level_Data.nMLevel;

	int iItemLevel = (pItem->Level >> 3) & 15;

	int iDecNeedStrength = 0, iDecNeedDex = 0;

	extern JewelHarmonyInfo* g_pUIJewelHarmonyinfo;
	if (iItemLevel >= pItem->Jewel_Of_Harmony_OptionLevel)
	{
		StrengthenCapability SC;
		g_pUIJewelHarmonyinfo->GetStrengthenCapability(&SC, pItem, 0);

		if (SC.SI_isNB)
		{
			iDecNeedStrength = SC.SI_NB.SI_force;
			iDecNeedDex = SC.SI_NB.SI_activity;
		}
	}
	if (pItem->SocketCount > 0)
	{
		for (int i = 0; i < pItem->SocketCount; ++i)
		{
			if (pItem->SocketSeedID[i] == 38)
			{
				int iReqStrengthDown = g_SocketItemMgr.GetSocketOptionValue(pItem, i);
				iDecNeedStrength += iReqStrengthDown;
			}
			else if (pItem->SocketSeedID[i] == 39)
			{
				int iReqDexterityDown = g_SocketItemMgr.GetSocketOptionValue(pItem, i);
				iDecNeedDex += iReqDexterityDown;
			}
		}
	}

	if ((pItem->RequireStrength - (unsigned int)iDecNeedStrength) > wStrength)
		return false;
	if ((pItem->RequireDexterity - (unsigned int)iDecNeedDex) > wDexterity)
		return false;
	if (pItem->RequireEnergy > wEnergy)
		return false;
	if (pItem->RequireVitality > wVitality)
		return false;
	if (pItem->RequireCharisma > wCharisma)
		return false;
	if (pItem->RequireLevel > wLevel)
		return false;

	if (pItem->Type == ITEM_HELPER + 5)
	{
		PET_INFO* pPetInfo = giPetManager::GetPetInfo(pItem);
		WORD wRequireCharisma = (185 + (pPetInfo->m_wLevel * 15));
		if (wRequireCharisma > wCharisma)
			return false;
	}

	if (gMapManager.WorldActive == WD_7ATLANSE && (pItem->Type >= ITEM_HELPER + 2 && pItem->Type <= ITEM_HELPER + 3))
	{
		return false;
	}
	else if (pItem->Type == ITEM_HELPER + 2 && gMapManager.WorldActive == WD_10HEAVEN)
	{
		return false;
	}
	else if (pItem->Type == ITEM_HELPER + 2 && g_Direction.m_CKanturu.IsMayaScene())
	{
		return false;
	}
	else if (gMapManager.InChaosCastle() || (M34CryWolf1st::Get_State_Only_Elf()
		&& g_isCharacterBuff((&Hero->Object), eBuff_CrywolfHeroContracted)))
	{
		if ((pItem->Type >= ITEM_HELPER + 2 && pItem->Type <= ITEM_HELPER + 5) || pItem->Type == ITEM_HELPER + 37)
			return false;
	}
	else if ((pItem->Type >= ITEM_HELPER + 2 && pItem->Type <= ITEM_HELPER + 4 || pItem->Type == ITEM_HELPER + 37)
		&& Hero->Object.CurrentAction >= PLAYER_SIT1 && Hero->Object.CurrentAction <= PLAYER_SIT_FEMALE2)
	{
		return false;
	}

	return bEquipable;
}

bool SEASON3B::CNewUIMyInventory::InsertItem(int iIndex, BYTE* pbyItemPacket)
{
	int InvenIndex = FindInvenIndex(iIndex);
	int LinealPos = SlotCntReceiveInvenIndex(iIndex, 64);

	if (IsCheckInvenIndex(InvenIndex))
	{
		return m_pNewInventoryCtrl[InvenIndex]->AddItem(LinealPos, pbyItemPacket);
	}

	return false;
}

void SEASON3B::CNewUIMyInventory::DeleteItem(int iIndex)
{
	int InvenIndex = FindInvenIndex(iIndex);
	int LinealPos = SlotCntReceiveInvenIndex(iIndex, 64);

	if (m_pNewInventoryCtrl[InvenIndex])
	{
		ITEM* pItem = m_pNewInventoryCtrl[InvenIndex]->FindItem(LinealPos);

		if (pItem != NULL)
		{
			m_pNewInventoryCtrl[InvenIndex]->RemoveItem(pItem);
		}
		else
		{
			CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
			if (pPickedItem
				&& pPickedItem->GetOwnerInventory() == m_pNewInventoryCtrl[InvenIndex]
				&& pPickedItem->GetSourceLinealPos() == LinealPos)
			{
				CNewUIInventoryCtrl::DeletePickedItem();
			}
		}
	}
}

void SEASON3B::CNewUIMyInventory::DeleteAllItems()
{
	for (int i = 0; i < MAX_INVENT_EXPANSION + 1; i++)
	{
		if (m_pNewInventoryCtrl[i])
			m_pNewInventoryCtrl[i]->RemoveAllItems();
	}
}

void SEASON3B::CNewUIMyInventory::SetPos(int x, int y)
{
	int X = x + 7;

	m_Pos.x = x;
	m_Pos.y = y;

	SetEquipmentSlotInfo();

	m_pNewInventoryCtrl[0]->SetPos(x + 15, y + 200);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonPosition(&m_BtnExit, m_Pos.x, m_Pos.y, 1);
		SetButtonPosition(&m_BtnRepair, m_Pos.x, m_Pos.y, 3);
		SetButtonPosition(&m_BtnMyShop, m_Pos.x, m_Pos.y, 2);
		SetButtonPosition(&m_BtnJewel, m_Pos.x, m_Pos.y, 4);
		if (GMProtect->IsInvExt())
		{
			m_BtnExpansion.SetPos(m_Pos.x + 132.0, m_Pos.y + 395.0);
		}
	}
	else
	{
		if (GMProtect->IsInvExt() == false)
		{
			X = m_Pos.x + 13;
		}

		m_BtnExit.SetPos(X, m_Pos.y + 391); X += 36;
		m_BtnRepair.SetPos(X, m_Pos.y + 391); X += 36;
		m_BtnMyShop.SetPos(X, m_Pos.y + 391); X += 36;
		m_BtnJewel.SetPos(X, m_Pos.y + 391); X += 36;

		if (GMProtect->IsInvExt())
		{
			m_BtnExpansion.SetPos(X, m_Pos.y + 391); X += 36;
		}
	}

	if (g_pInvenExpansion)
	{
		g_pInvenExpansion->SetPos(m_Pos.x - 190, m_Pos.y);
	}
}

const POINT& SEASON3B::CNewUIMyInventory::GetPos() const
{
	return m_Pos;
}

CNewUIMyInventory::REPAIR_MODE SEASON3B::CNewUIMyInventory::GetRepairMode() const
{
	return m_RepairMode;
}

void SEASON3B::CNewUIMyInventory::SetRepairMode(bool bRepair)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			if (bRepair == true)
			{
				m_RepairMode = REPAIR_MODE_ON;
				m_pNewInventoryCtrl[i]->SetRepairMode(true);
			}
			else
			{
				m_RepairMode = REPAIR_MODE_OFF;
				m_pNewInventoryCtrl[i]->SetRepairMode(false);
			}
		}
	}
}

bool SEASON3B::CNewUIMyInventory::UpdateMouseEvent()
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i] && !m_pNewInventoryCtrl[i]->UpdateMouseEvent())
			return false;
	}

	if (EquipmentWindowProcess() == true)
		return false;
	if (InventoryProcess() == true)
		return false;
	if (BtnProcess() == true)
		return false;

	CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();

	if (!pPickedItem)
		goto LABEL_NEXT;

	if (!SEASON3B::IsPress(VK_LBUTTON))
		goto LABEL_NEXT;

	int width = GetScreenWidth();

	if (!CheckMouseIn(0, 0, width, 429))
		goto LABEL_NEXT;

	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_TRADE) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_DEVILSQUARE) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_BLOODCASTLE) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MIXINVENTORY) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MYSHOP_INVENTORY) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_LUCKYITEMWND) == true
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_PURCHASESHOP_INVENTORY) == true)
	{
		ResetMouseLButton();
		return false;
	}

	ITEM* pItemObj = pPickedItem->GetItem();

	if (pItemObj && pItemObj->Jewel_Of_Harmony_Option != 0)
	{
		g_pChatListBox->AddText("", GlobalText[2217], SEASON3B::TYPE_ERROR_MESSAGE);
		ResetMouseLButton();
		return false;
	}
	else if (pItemObj && IsHighValueItem(pItemObj) == true)
	{
		g_pChatListBox->AddText("", GlobalText[269], SEASON3B::TYPE_ERROR_MESSAGE);
		CNewUIInventoryCtrl::BackupPickedItem();
		ResetMouseLButton();
		return false;
	}
	else if (pItemObj && IsDropBan(pItemObj))
	{
		g_pChatListBox->AddText("", GlobalText[1915], SEASON3B::TYPE_ERROR_MESSAGE);
		CNewUIInventoryCtrl::BackupPickedItem();
		ResetMouseLButton();
		return false;
	}

	if (pItemObj)
	{
		if (pItemObj->Type == ITEM_POTION + 28)
		{
			if (gMapManager.IsCursedTemple() == true)
			{
				ResetMouseLButton();
				return false;
			}
		}
	}

	RenderTerrain(true);
	if (RenderTerrainTile(SelectXF, SelectYF, (int)SelectXF, (int)SelectYF, 1.f, 1, true))
	{
		int iSourceIndex = pPickedItem->GetSourceLinealPos();
		int tx = (int)(CollisionPosition[0] / TERRAIN_SCALE);
		int ty = (int)(CollisionPosition[1] / TERRAIN_SCALE);
		int InvenIndex = this->IsInvenctrlSame(pPickedItem->GetOwnerInventory());
		iSourceIndex = this->SlotCntSendSrcInvenIndex(iSourceIndex, InvenIndex);

		if (InvenIndex == -1)
		{
			if (pItemObj && pItemObj->ex_src_type == ITEM_EX_SRC_EQUIPMENT)
			{
				SendRequestDropItem(iSourceIndex, tx, ty);
			}
		}
		else if (Hero->Dead == 0)
		{
			SendRequestDropItem(MAX_EQUIPMENT_INDEX + iSourceIndex, tx, ty);
		}

		MouseUpdateTime = 0;
		MouseUpdateTimeMax = 6;
		ResetMouseLButton();
		return false;
	}
	else
	{
	LABEL_NEXT:
		g_csItemOption.SetViewOptionList(false);
		if (CheckMouseIn(m_Pos.x, m_Pos.y + 20, 95, 15) == true)
		{
			g_csItemOption.SetViewOptionList(true);
		}

		if (CheckMouseIn(m_Pos.x, m_Pos.y, INVENTORY_WIDTH, INVENTORY_HEIGHT))
		{
			if (SEASON3B::IsPress(VK_RBUTTON))
			{
				ResetMouseRButton();
				return false;
			}

			if (SEASON3B::IsNone(VK_LBUTTON) == false)
				return false;
		}
		else
		{
			if (!SEASON3B::IsNone(VK_LBUTTON) || !SEASON3B::IsNone(VK_RBUTTON))
			{
				if (g_pInvenExpansion->CheckExpansionInventory())
				{
					if (SEASON3B::IsPress(VK_RBUTTON))
					{
						ResetMouseRButton();
					}
					return false;
				}

				if (g_pInventoryJewel->CheckExpansionInventory())
				{
					if (SEASON3B::IsPress(VK_RBUTTON))
					{
						ResetMouseRButton();
					}
					return false;
				}
			}
		}
	}
	return true;
}

bool SEASON3B::CNewUIMyInventory::UpdateKeyEvent()
{
	if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
		return true;

	if (SEASON3B::IsPress(VK_ESCAPE) == true)
	{
		if (!g_pNPCShop->IsSellingItem())
		{
			g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY);
			PlayBuffer(SOUND_CLICK01);
		}
		return false;
	}
	else if (SEASON3B::IsPress('L') == true && m_bRepairEnableLevel == true)
	{
		if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP))
		{
			if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MIXINVENTORY))
			{
				if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_LUCKYITEMWND))
				{
					ToggleRepairMode();
					return false;
				}
			}
		}
	}

	if (CanOpenMyShopInterface() != true || !SEASON3B::IsPress('S'))
	{
		if (!CheckMouseIn(m_Pos.x, m_Pos.y, INVENTORY_WIDTH, INVENTORY_HEIGHT))
		{
			if (!g_pInvenExpansion->CheckExpansionInventory())
				return true;
		}

		if (SEASON3B::IsRepeat(VK_CONTROL))
		{
			int iHotKey = -1;
			if (SEASON3B::IsPress('Q'))
			{
				iHotKey = HOTKEY_Q;
			}
			else if (SEASON3B::IsPress('W'))
			{
				iHotKey = HOTKEY_W;
			}
			else if (SEASON3B::IsPress('E'))
			{
				iHotKey = HOTKEY_E;
			}
			else if (SEASON3B::IsPress('R') && (GMProtect->LookAndFeelType() != 1 && GMProtect->LookAndFeelType() != 2))
			{
				iHotKey = HOTKEY_R;
			}

			if (iHotKey != -1)
			{
				int InvenIndex = -1;
				ITEM* pItem = FindItemAtPt(MouseX, MouseY, &InvenIndex);
				if (pItem == NULL)
					return false;

				if (CanRegisterItemHotKey(pItem->Type) == true)
				{
					int iItemLevel = (pItem->Level >> 3) & 15;
					g_pMainFrame->SetItemHotKey(iHotKey, pItem->Type, iItemLevel);
					return false;
				}
			}
		}
		return true;
	}
	if (m_bMyShopOpen)
	{
		if (m_MyShopMode == MYSHOP_MODE_OPEN)
		{
			ChangeMyShopButtonStateClose();
		}
		else if (m_MyShopMode == MYSHOP_MODE_CLOSE)
		{
			ChangeMyShopButtonStateOpen();
		}
		g_pNewUISystem->Toggle(SEASON3B::INTERFACE_MYSHOP_INVENTORY);
		PlayBuffer(SOUND_CLICK01);
	}
	return false;
}

bool SEASON3B::CNewUIMyInventory::Update()
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i] && false == m_pNewInventoryCtrl[i]->Update())
		{
			return false;
		}
	}

	if (IsVisible())
	{
		m_iPointedSlot = -1;
		for (int i = 0; i < MAX_EQUIPMENT_INDEX; i++)
		{
			if (CheckMouseIn(m_EquipmentSlots[i].x + 1, m_EquipmentSlots[i].y, m_EquipmentSlots[i].width - 4, m_EquipmentSlots[i].height - 4))
			{
				m_iPointedSlot = i;
				break;
			}
		}
	}
	return true;
}

bool SEASON3B::CNewUIMyInventory::Render()
{
	EnableAlphaTest();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	RenderFrame();
	RenderInventoryDetails();
	RenderSetOption();
	RenderSocketOption();
	RenderButtons();

	if (m_pNewInventoryCtrl[0])
		m_pNewInventoryCtrl[0]->Render();

	RenderEquippedItem();
	DisableAlphaBlend();
	return true;
}

void SEASON3B::CNewUIMyInventory::RenderSetOption()
{
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	if (g_csItemOption.GetSetOptionANum() > 0 || g_csItemOption.GetSetOptionBNum() > 0)
	{
		g_pRenderText->SetTextColor(255, 204, 25, 255);
	}
	else
	{
		g_pRenderText->SetTextColor(100, 100, 100, 255);
	}

	unicode::t_char strText[128];
	unicode::_sprintf(strText, "[%s]", GlobalText[989]);
	g_pRenderText->RenderText(m_Pos.x + INVENTORY_WIDTH * 0.2f, m_Pos.y + 25, strText, INVENTORY_WIDTH * 0.3f, 0, RT3_SORT_CENTER);

	if (g_csItemOption.IsViewOptionList() == true)
	{
		m_pNewUI3DRenderMng->RenderUI2DEffect(TOOLTIP_CAMERA_Z_ORDER, UI2DEffectCallback, this, -1, ITEM_SET_OPTION);
	}

}

void SEASON3B::CNewUIMyInventory::RenderSocketOption()
{
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0, 0, 0, 0);

	if (g_SocketItemMgr.IsSocketSetOptionEnabled())
	{
		g_pRenderText->SetTextColor(255, 204, 25, 255);
	}
	else
	{
		g_pRenderText->SetTextColor(100, 100, 100, 255);
	}

	unicode::t_char strText[128];
	unicode::_sprintf(strText, "[%s]", GlobalText[2651]);

	g_pRenderText->RenderText(m_Pos.x + INVENTORY_WIDTH * 0.5f, m_Pos.y + 25, strText, INVENTORY_WIDTH * 0.3f, 0, RT3_SORT_CENTER);

	if (CheckMouseIn(m_Pos.x + INVENTORY_WIDTH * 0.5f, m_Pos.y + 20, INVENTORY_WIDTH * 0.5f, 15) == true)
	{
		m_pNewUI3DRenderMng->RenderUI2DEffect(TOOLTIP_CAMERA_Z_ORDER, UI2DEffectCallback, this, -1, ITEM_SOCKET_SET_OPTION);
	}
}

void SEASON3B::CNewUIMyInventory::Render3D()
{
	for (int i = 0; i < MAX_EQUIPMENT_INDEX; i++)
	{
		ITEM* pEquippedItem = &CharacterMachine->Equipment[i];

		if (pEquippedItem->Type >= 0)
		{
			float posy = 0.0;
			float posx = m_EquipmentSlots[i].x + 1;
			float wmodel = (m_EquipmentSlots[i].width - 4);
			float hmodel = (m_EquipmentSlots[i].height - 4);

			if (i == EQUIPMENT_ARMOR)
			{
				posy = m_EquipmentSlots[i].y - 10.f;
			}
			else
			{
				posy = m_EquipmentSlots[i].y;
			}

			glColor4f(1.f, 1.f, 1.f, 1.f);

			Script_Item* item_info = GMItemMng->find(pEquippedItem->Type);

			if (item_info->Type != -1)
			{
				int itemwidth = (item_info->Width * 20);
				int itemhight = (item_info->Height *20);

				posx += (wmodel - itemwidth) / 2;
				posy += (hmodel - itemhight) / 2;

				wmodel = itemwidth;
				hmodel = itemhight;
			}

			RenderItem3D(posx, posy, wmodel, hmodel, pEquippedItem->Type, pEquippedItem->Level, pEquippedItem->Option1, pEquippedItem->ExtOption, false);
		}
	}
}

bool SEASON3B::CNewUIMyInventory::IsVisible() const
{
	return CNewUIObj::IsVisible();
}

void SEASON3B::CNewUIMyInventory::OpenningProcess()
{
	SetRepairMode(false);

	m_MyShopMode = MYSHOP_MODE_OPEN;
	ChangeMyShopButtonStateOpen();

	WORD wLevel = CharacterAttribute->Level;
	m_bRepairEnableLevel = (wLevel >= 50);
	m_bMyShopOpen = (wLevel >= 6);

	if (g_QuestMng.IsIndexInCurQuestIndexList(0x1000F))
	{
		if (g_QuestMng.IsEPRequestRewardState(0x1000F))
		{
			SendSatisfyQuestRequestFromClient(0x1000F);
			g_QuestMng.SetEPRequestRewardState(0x1000F, false);
		}
	}
	this->SetInvenPos();
}

void SEASON3B::CNewUIMyInventory::ClosingProcess()
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		m_pNewInventoryCtrl[i]->BackupPickedItem();
	}
	RepairEnable = 0;
	SetRepairMode(false);

	if (g_pInvenExpansion)
	{
		g_pInvenExpansion->SetPos(m_Pos.x - 190, m_Pos.y);
	}
}

float SEASON3B::CNewUIMyInventory::GetLayerDepth()
{
	return 4.2f;
}

CNewUIInventoryCtrl* SEASON3B::CNewUIMyInventory::GetInventoryCtrl(int index) const
{
	if (index >= 0 && index <= MAX_INVENT_EXPANSION + 1)
		return m_pNewInventoryCtrl[index];
	else
		return NULL;
}

ITEM* SEASON3B::CNewUIMyInventory::FindItem(int iLinealPos, int InvenIndex)
{
	int invenIndex;

	if (InvenIndex)
		invenIndex = InvenIndex;
	else
		invenIndex = FindInvenIndex(iLinealPos);

	int LinealPos = SlotCntReceiveInvenIndex(iLinealPos, 64);

	if (IsCheckInvenIndex(invenIndex))
		return m_pNewInventoryCtrl[invenIndex]->FindItem(LinealPos);

	return NULL;
}

ITEM* SEASON3B::CNewUIMyInventory::FindItemByKey(DWORD dwKey)
{
	ITEM* pItem = NULL;

	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			pItem = m_pNewInventoryCtrl[i]->FindItemByKey(dwKey);

			if (pItem != NULL)
			{
				break;
			}
		}
	}
	return pItem;
}

int SEASON3B::CNewUIMyInventory::FindItemIndex(short int siType, int iLevel)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			int Index = m_pNewInventoryCtrl[i]->FindItemIndex(siType, iLevel);
			if (Index != -1)
				return this->SlotCntSendSrcInvenIndex(Index, i);
		}
	}
	return -1;
}

int SEASON3B::CNewUIMyInventory::FindItemReverseIndex(short sType, int iLevel)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			int Index = m_pNewInventoryCtrl[i]->FindItemReverseIndex(sType, iLevel);
			if (Index != -1)
				return this->SlotCntSendSrcInvenIndex(Index, i);
		}
	}
	return -1;
}

int SEASON3B::CNewUIMyInventory::FindItemReverseKind2(short sType, int iLevel)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			int Index = m_pNewInventoryCtrl[i]->FindItemReverseKind2(sType, iLevel);
			if (Index != -1)
				return this->SlotCntSendSrcInvenIndex(Index, i);
		}
	}
	return -1;
}

int SEASON3B::CNewUIMyInventory::FindEmptySlot(IN int cx, IN int cy)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			int Index = m_pNewInventoryCtrl[i]->FindEmptySlot(cx, cy);
			if (Index != -1)
				return this->SlotCntSendSrcInvenIndex(Index, i);
		}
	}
	return -1;
}

int SEASON3B::CNewUIMyInventory::FindEmptySlot(ITEM* pItem, bool bcheckPick)
{
	Script_Item* pItemAttr = GMItemMng->find(pItem->Type);

	if (this->GetInvenEnableCnt() == 1 && bcheckPick)
	{
		g_pChatListBox->AddText("", GlobalText[3343], TYPE_SYSTEM_MESSAGE);
	}
	else
	{
		for (int i = 0; i < this->GetInvenEnableCnt(); i++)
		{
			if ((!bcheckPick || i) && m_pNewInventoryCtrl[i])
			{
				int Index = m_pNewInventoryCtrl[i]->FindEmptySlot(pItemAttr->Width, pItemAttr->Height);

				if (Index != -1)
					return this->SlotCntSendSrcInvenIndex(Index, i);
			}
		}
		if (this->GetInvenEnableCnt() == MAX_EQUIPMENT_INDEX + 1)
		{
			g_pChatListBox->AddText("", GlobalText[3341], TYPE_SYSTEM_MESSAGE);
		}
		else
		{
			g_pChatListBox->AddText("", GlobalText[3343], TYPE_SYSTEM_MESSAGE);
		}
	}
	return -1;
}

void SEASON3B::CNewUIMyInventory::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
	if (pClass)
	{
		CNewUIMyInventory* pMyInventory = (CNewUIMyInventory*)(pClass);

		if (dwParamB == ITEM_SET_OPTION)
		{
			g_csItemOption.RenderSetOptionList(pMyInventory->GetPos().x, pMyInventory->GetPos().y);
		}
		else if (dwParamB == ITEM_SOCKET_SET_OPTION)
		{
			g_SocketItemMgr.RenderToolTipForSocketSetOption(pMyInventory->GetPos().x, pMyInventory->GetPos().y);
		}
		else
		{
			pMyInventory->RenderItemToolTip(dwParamA);
		}
	}
}

void SEASON3B::CNewUIMyInventory::SetStandbyItemKey(DWORD dwItemKey)
{
	m_dwStandbyItemKey = dwItemKey;
}

DWORD SEASON3B::CNewUIMyInventory::GetStandbyItemKey() const
{
	return m_dwStandbyItemKey;
}

int SEASON3B::CNewUIMyInventory::GetStandbyItemIndex()
{
	ITEM* pItem = NULL;
	int InvenIndex = 0;

	for (int i = this->GetInvenEnableCnt(); InvenIndex < i; i = this->GetInvenEnableCnt())
	{
		if (m_pNewInventoryCtrl[InvenIndex])
		{
			pItem = m_pNewInventoryCtrl[InvenIndex]->FindItemByKey(m_dwStandbyItemKey);
			if (pItem)
				break;
		}
		++InvenIndex;
	}

	if (pItem)
	{
		return this->GetInvenSlotIndex(pItem->x, pItem->y, InvenIndex);
	}

	return -1;
}

ITEM* SEASON3B::CNewUIMyInventory::GetStandbyItem()
{
	ITEM* pItem = NULL;
	int InvenIndex = 0;

	for (int i = this->GetInvenEnableCnt(); InvenIndex < i; i = this->GetInvenEnableCnt())
	{
		if (m_pNewInventoryCtrl[InvenIndex])
		{
			pItem = m_pNewInventoryCtrl[InvenIndex]->FindItemByKey(m_dwStandbyItemKey);
			if (pItem)
				break;
		}
		++InvenIndex;
	}
	return pItem;
}

extern void PlayerStopAnimationSetting(CHARACTER* c, OBJECT* o);

void SEASON3B::CNewUIMyInventory::CreateEquippingEffect(ITEM* pItem)
{
	SetCharacterClass(Hero);
	OBJECT* pHeroObject = &Hero->Object;
	if (false == gMapManager.InChaosCastle())
	{
		switch (pItem->Type)
		{
		case ITEM_HELPER:
			CreateBug(MODEL_HELPER, pHeroObject->Position, pHeroObject);
			break;
		case ITEM_HELPER + 2:
			CreateBug(MODEL_UNICON, pHeroObject->Position, pHeroObject);
			if (!Hero->SafeZone)
				CreateEffect(BITMAP_MAGIC + 1, pHeroObject->Position, pHeroObject->Angle, pHeroObject->Light, 1, pHeroObject);
			break;
		case ITEM_HELPER + 3:
			CreateBug(MODEL_PEGASUS, pHeroObject->Position, pHeroObject);
			if (!Hero->SafeZone)
				CreateEffect(BITMAP_MAGIC + 1, pHeroObject->Position, pHeroObject->Angle, pHeroObject->Light, 1, pHeroObject);
			break;
		case ITEM_HELPER + 4:
			CreateBug(MODEL_DARK_HORSE, pHeroObject->Position, pHeroObject);
			if (!Hero->SafeZone)
				CreateEffect(BITMAP_MAGIC + 1, pHeroObject->Position, pHeroObject->Angle, pHeroObject->Light, 1, pHeroObject);
			break;
		case ITEM_HELPER + 37:
			Hero->Helper.Option1 = pItem->Option1;
			if (pItem->Option1 == 0x01)
			{
				CreateBug(MODEL_FENRIR_BLACK, pHeroObject->Position, pHeroObject);
			}
			else if (pItem->Option1 == 0x02)
			{
				CreateBug(MODEL_FENRIR_BLUE, pHeroObject->Position, pHeroObject);
			}
			else if (pItem->Option1 == 0x04)
			{
				CreateBug(MODEL_FENRIR_GOLD, pHeroObject->Position, pHeroObject);
			}
			else
			{
				CreateBug(MODEL_FENRIR_RED, pHeroObject->Position, pHeroObject);
			}

			if (!Hero->SafeZone)
			{
				CreateEffect(BITMAP_MAGIC + 1, pHeroObject->Position, pHeroObject->Angle, pHeroObject->Light, 1, pHeroObject);
			}
			break;
		case ITEM_HELPER + 64:
			ThePetProcess().CreatePet(pItem->Type, MODEL_HELPER + 64, pHeroObject->Position, Hero);
			break;
		case ITEM_HELPER + 65:
			ThePetProcess().CreatePet(pItem->Type, MODEL_HELPER + 65, pHeroObject->Position, Hero);
			break;
		case ITEM_HELPER + 67:
			ThePetProcess().CreatePet(pItem->Type, MODEL_HELPER + 67, pHeroObject->Position, Hero);
			break;
		case ITEM_HELPER + 80:
			ThePetProcess().CreatePet(pItem->Type, MODEL_HELPER + 80, pHeroObject->Position, Hero);
			break;
		case ITEM_HELPER + 106:
			ThePetProcess().CreatePet(pItem->Type, MODEL_HELPER + 106, pHeroObject->Position, Hero);
			break;
		case ITEM_HELPER + 123:
			ThePetProcess().CreatePet(pItem->Type, MODEL_HELPER + 123, pHeroObject->Position, Hero);
			break;
		default:
			{
				if (ThePetProcess().IsPet(pItem->Type))
				{
					ThePetProcess().CreatePet(pItem->Type, (pItem->Type + MODEL_ITEM), pHeroObject->Position, Hero);
				}
				else
				{
					CreateGoboid(pItem->Type, pHeroObject->Position, pHeroObject);
					PlayerStopAnimationSetting(Hero, pHeroObject);
				}
			}
			break;
		}
	}
	if (Hero->EtcPart <= 0 || Hero->EtcPart > 3)
	{
		if (pItem->Type == ITEM_HELPER + 20 && (pItem->Level >> 3) == 3)
		{
			DeleteParts(Hero);
			Hero->EtcPart = PARTS_LION;
		}
	}
	if (pItem->Type == ITEM_WING + 39 || pItem->Type == ITEM_HELPER + 30 ||
		pItem->Type == ITEM_WING + 130 ||
#ifdef PBG_ADD_NEWCHAR_MONK_ITEM
		(pItem->Type >= ITEM_WING + 49 && pItem->Type <= ITEM_WING + 50) ||
		(pItem->Type == ITEM_WING + 135) ||
#endif //PBG_ADD_NEWCHAR_MONK_ITEM
		pItem->Type == ITEM_WING + 40)
	{
		DeleteCloth(Hero, &Hero->Object);
	}
}

void SEASON3B::CNewUIMyInventory::DeleteEquippingEffectBug(ITEM* pItem)
{
	if (ThePetProcess().IsPet(pItem->Type) == true)
	{
		ThePetProcess().DeletePet(Hero, pItem->Type);
	}

	switch (pItem->Type)
	{
	case ITEM_HELPER + 30:
	case ITEM_WING + 39:
	case ITEM_WING + 40:
	case ITEM_WING + 130:
#ifdef PBG_ADD_NEWCHAR_MONK_ITEM
	case ITEM_WING + 49:
	case ITEM_WING + 50:
	case ITEM_WING + 135:
#endif //PBG_ADD_NEWCHAR_MONK_ITEM
		DeleteCloth(Hero, &Hero->Object);
		return;
	}

	if (IsBug(pItem) == true)
	{
		DeleteBug(&Hero->Object);
		PlayerStopAnimationSetting(Hero, &Hero->Object);
	}
}

void SEASON3B::CNewUIMyInventory::DeleteEquippingEffect()
{
	if (Hero->EtcPart < PARTS_ATTACK_TEAM_MARK)
	{
		DeleteParts(Hero);
		if (Hero->EtcPart > 3)
		{
			Hero->EtcPart = 0;
		}
	}

	SetCharacterClass(Hero);
}

void SEASON3B::CNewUIMyInventory::SetEquipmentSlotInfo()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		m_EquipmentSlots[EQUIPMENT_HELPER].x = m_Pos.x + 15;
		m_EquipmentSlots[EQUIPMENT_HELPER].y = m_Pos.y + 46;
		m_EquipmentSlots[EQUIPMENT_HELPER].width = 40;
		m_EquipmentSlots[EQUIPMENT_HELPER].height = 40;
		m_EquipmentSlots[EQUIPMENT_HELPER].dwBgImage = BITMAP_INVENTORY + 15;

		m_EquipmentSlots[EQUIPMENT_HELM].x = m_Pos.x + 75;
		m_EquipmentSlots[EQUIPMENT_HELM].y = m_Pos.y + 46;
		m_EquipmentSlots[EQUIPMENT_HELM].width = 40;
		m_EquipmentSlots[EQUIPMENT_HELM].height = 40;
		m_EquipmentSlots[EQUIPMENT_HELM].dwBgImage = BITMAP_INVENTORY + 3;

		m_EquipmentSlots[EQUIPMENT_WING].x = m_Pos.x + 116;
		m_EquipmentSlots[EQUIPMENT_WING].y = m_Pos.y + 46;
		m_EquipmentSlots[EQUIPMENT_WING].width = 60;
		m_EquipmentSlots[EQUIPMENT_WING].height = 40;
		m_EquipmentSlots[EQUIPMENT_WING].dwBgImage = BITMAP_INVENTORY + 14;
		//--
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].x = m_Pos.x + 135;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].y = m_Pos.y + 89;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].width = 40;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].height = 60;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].dwBgImage = BITMAP_INVENTORY + 16;

		m_EquipmentSlots[EQUIPMENT_ARMOR].x = m_Pos.x + 75;
		m_EquipmentSlots[EQUIPMENT_ARMOR].y = m_Pos.y + 89;
		m_EquipmentSlots[EQUIPMENT_ARMOR].width = 40;
		m_EquipmentSlots[EQUIPMENT_ARMOR].height = 60;
		m_EquipmentSlots[EQUIPMENT_ARMOR].dwBgImage = BITMAP_INVENTORY + 4;

		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].x = m_Pos.x + 15;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].y = m_Pos.y + 89;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].width = 40;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].height = 60;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].dwBgImage = BITMAP_INVENTORY + 6;
		//--
		m_EquipmentSlots[EQUIPMENT_GLOVES].x = m_Pos.x + 15;
		m_EquipmentSlots[EQUIPMENT_GLOVES].y = m_Pos.y + 152;
		m_EquipmentSlots[EQUIPMENT_GLOVES].width = 40;
		m_EquipmentSlots[EQUIPMENT_GLOVES].height = 40;
		m_EquipmentSlots[EQUIPMENT_GLOVES].dwBgImage = BITMAP_INVENTORY + 7;

		m_EquipmentSlots[EQUIPMENT_PANTS].x = m_Pos.x + 75;
		m_EquipmentSlots[EQUIPMENT_PANTS].y = m_Pos.y + 152;
		m_EquipmentSlots[EQUIPMENT_PANTS].width = 40;
		m_EquipmentSlots[EQUIPMENT_PANTS].height = 40;
		m_EquipmentSlots[EQUIPMENT_PANTS].dwBgImage = BITMAP_INVENTORY + 5;

		m_EquipmentSlots[EQUIPMENT_BOOTS].x = m_Pos.x + 135;
		m_EquipmentSlots[EQUIPMENT_BOOTS].y = m_Pos.y + 152;
		m_EquipmentSlots[EQUIPMENT_BOOTS].width = 40;
		m_EquipmentSlots[EQUIPMENT_BOOTS].height = 40;
		m_EquipmentSlots[EQUIPMENT_BOOTS].dwBgImage = BITMAP_INVENTORY + 8;

		m_EquipmentSlots[EQUIPMENT_RING_LEFT].x = m_Pos.x + 115;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].y = m_Pos.y + 152;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].width = 20;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].height = 20;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].dwBgImage = BITMAP_INVENTORY + 10;

		m_EquipmentSlots[EQUIPMENT_AMULET].x = m_Pos.x + 55;
		m_EquipmentSlots[EQUIPMENT_AMULET].y = m_Pos.y + 89;
		m_EquipmentSlots[EQUIPMENT_AMULET].width = 20;
		m_EquipmentSlots[EQUIPMENT_AMULET].height = 20;
		m_EquipmentSlots[EQUIPMENT_AMULET].dwBgImage = BITMAP_INVENTORY + 9;

		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].x = m_Pos.x + 55;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].y = m_Pos.y + 152;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].width = 20;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].height = 20;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].dwBgImage = BITMAP_INVENTORY + 10;
	}
	else
	{
		m_EquipmentSlots[EQUIPMENT_HELPER].x = m_Pos.x + 15;
		m_EquipmentSlots[EQUIPMENT_HELPER].y = m_Pos.y + 44;
		m_EquipmentSlots[EQUIPMENT_HELPER].width = 46;
		m_EquipmentSlots[EQUIPMENT_HELPER].height = 46;
		m_EquipmentSlots[EQUIPMENT_HELPER].dwBgImage = IMAGE_INVENTORY_ITEM_FAIRY;

		m_EquipmentSlots[EQUIPMENT_HELM].x = m_Pos.x + 75;
		m_EquipmentSlots[EQUIPMENT_HELM].y = m_Pos.y + 44;
		m_EquipmentSlots[EQUIPMENT_HELM].width = 46;
		m_EquipmentSlots[EQUIPMENT_HELM].height = 46;
		m_EquipmentSlots[EQUIPMENT_HELM].dwBgImage = IMAGE_INVENTORY_ITEM_HELM;

		m_EquipmentSlots[EQUIPMENT_WING].x = m_Pos.x + 120;
		m_EquipmentSlots[EQUIPMENT_WING].y = m_Pos.y + 44;
		m_EquipmentSlots[EQUIPMENT_WING].width = 61;
		m_EquipmentSlots[EQUIPMENT_WING].height = 46;
		m_EquipmentSlots[EQUIPMENT_WING].dwBgImage = IMAGE_INVENTORY_ITEM_WING;

		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].x = m_Pos.x + 135;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].y = m_Pos.y + 87;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].width = 46;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].height = 66;
		m_EquipmentSlots[EQUIPMENT_WEAPON_LEFT].dwBgImage = IMAGE_INVENTORY_ITEM_LEFT;

		m_EquipmentSlots[EQUIPMENT_ARMOR].x = m_Pos.x + 75;
		m_EquipmentSlots[EQUIPMENT_ARMOR].y = m_Pos.y + 87;
		m_EquipmentSlots[EQUIPMENT_ARMOR].width = 46;
		m_EquipmentSlots[EQUIPMENT_ARMOR].height = 66;
		m_EquipmentSlots[EQUIPMENT_ARMOR].dwBgImage = IMAGE_INVENTORY_ITEM_ARMOR;

		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].x = m_Pos.x + 15;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].y = m_Pos.y + 87;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].width = 46;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].height = 66;
		m_EquipmentSlots[EQUIPMENT_WEAPON_RIGHT].dwBgImage = IMAGE_INVENTORY_ITEM_RIGHT;

		m_EquipmentSlots[EQUIPMENT_GLOVES].x = m_Pos.x + 15;
		m_EquipmentSlots[EQUIPMENT_GLOVES].y = m_Pos.y + 150;
		m_EquipmentSlots[EQUIPMENT_GLOVES].width = 46;
		m_EquipmentSlots[EQUIPMENT_GLOVES].height = 46;
		m_EquipmentSlots[EQUIPMENT_GLOVES].dwBgImage = IMAGE_INVENTORY_ITEM_GLOVES;

		m_EquipmentSlots[EQUIPMENT_PANTS].x = m_Pos.x + 75;
		m_EquipmentSlots[EQUIPMENT_PANTS].y = m_Pos.y + 150;
		m_EquipmentSlots[EQUIPMENT_PANTS].width = 46;
		m_EquipmentSlots[EQUIPMENT_PANTS].height = 46;
		m_EquipmentSlots[EQUIPMENT_PANTS].dwBgImage = IMAGE_INVENTORY_ITEM_PANTS;

		m_EquipmentSlots[EQUIPMENT_BOOTS].x = m_Pos.x + 135;
		m_EquipmentSlots[EQUIPMENT_BOOTS].y = m_Pos.y + 150;
		m_EquipmentSlots[EQUIPMENT_BOOTS].width = 46;
		m_EquipmentSlots[EQUIPMENT_BOOTS].height = 46;
		m_EquipmentSlots[EQUIPMENT_BOOTS].dwBgImage = IMAGE_INVENTORY_ITEM_BOOT;

		m_EquipmentSlots[EQUIPMENT_RING_LEFT].x = m_Pos.x + 114;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].y = m_Pos.y + 150;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].width = 28;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].height = 28;
		m_EquipmentSlots[EQUIPMENT_RING_LEFT].dwBgImage = IMAGE_INVENTORY_ITEM_RING;

		m_EquipmentSlots[EQUIPMENT_AMULET].x = m_Pos.x + 54;
		m_EquipmentSlots[EQUIPMENT_AMULET].y = m_Pos.y + 87;
		m_EquipmentSlots[EQUIPMENT_AMULET].width = 28;
		m_EquipmentSlots[EQUIPMENT_AMULET].height = 28;
		m_EquipmentSlots[EQUIPMENT_AMULET].dwBgImage = IMAGE_INVENTORY_ITEM_NECKLACE;

		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].x = m_Pos.x + 54;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].y = m_Pos.y + 150;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].width = 28;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].height = 28;
		m_EquipmentSlots[EQUIPMENT_RING_RIGHT].dwBgImage = IMAGE_INVENTORY_ITEM_RING;
	}
}

void SEASON3B::CNewUIMyInventory::SetButtonInfo()
{
	int X = m_Pos.x + 7;
	int Y = m_Pos.y + 391;

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		SetButtonInterface(&m_BtnExit, m_Pos.x, m_Pos.y, 1);
		SetButtonInterface(&m_BtnRepair, m_Pos.x, m_Pos.y, 3);
		SetButtonInterface(&m_BtnMyShop, m_Pos.x, m_Pos.y, 2);
		SetButtonInterface(&m_BtnJewel, m_Pos.x, m_Pos.y, 4);

		m_BtnExit.ChangeToolTipText(GlobalText[225], true);
		m_BtnRepair.ChangeToolTipText(GlobalText[233], true);
		m_BtnMyShop.ChangeToolTipText(GlobalText[1125], true);
		m_BtnJewel.ChangeToolTipText(GlobalText[102], true);

		if (GMProtect->IsInvExt())
		{
			SetButtonImagen(&m_BtnExpansion, BITMAP_INTERFACE, BITMAP_INTERFACE + 1);
			m_BtnExpansion.ChangeButtonInfo(m_Pos.x + 132.0, m_Pos.y + 395.0, 24, 24);
		}
	}
	else
	{
		if (GMProtect->IsInvExt() == false)
		{
			X = m_Pos.x + 13;
		}

		m_BtnExit.ChangeButtonImgState(true, IMAGE_INVENTORY_EXIT_BTN, false);
		m_BtnExit.ChangeButtonInfo(X, Y, 36, 29); X += 36;
		m_BtnExit.ChangeToolTipText(GlobalText[225], true);

		m_BtnRepair.ChangeButtonImgState(true, IMAGE_INVENTORY_REPAIR_BTN, false);
		m_BtnRepair.ChangeButtonInfo(X, Y, 36, 29); X += 36;
		m_BtnRepair.ChangeToolTipText(GlobalText[233], true);

		m_BtnMyShop.ChangeButtonImgState(true, IMAGE_INVENTORY_MYSHOP_OPEN_BTN, false);
		m_BtnMyShop.ChangeButtonInfo(X, Y, 36, 29); X += 36;
		m_BtnMyShop.ChangeToolTipText(GlobalText[1125], true);

		m_BtnJewel.ChangeButtonImgState(true, IMAGE_INVENTORY_JEWEL_BTN, false);
		m_BtnJewel.ChangeButtonInfo(X, Y, 36, 29); X += 36;
		m_BtnJewel.ChangeToolTipText(GlobalText[102], true);

		if (GMProtect->IsInvExt())
		{
			m_BtnExpansion.ChangeButtonImgState(true, IMAGE_INVENTORY_EXPANSION_BTN, false);
			m_BtnExpansion.ChangeButtonInfo(X, Y, 36, 29); X += 36;
			m_BtnExpansion.ChangeToolTipText(GlobalText[3322], true);
		}
	}
}

void SEASON3B::CNewUIMyInventory::LoadImages()
{
	LoadBitmap("Interface\\newui_msgbox_back.jpg", IMAGE_INVENTORY_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back01.tga", IMAGE_INVENTORY_BACK_TOP, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back04.tga", IMAGE_INVENTORY_BACK_TOP2, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back02-L.tga", IMAGE_INVENTORY_BACK_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back02-R.tga", IMAGE_INVENTORY_BACK_RIGHT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back03.tga", IMAGE_INVENTORY_BACK_BOTTOM, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_boots.tga", IMAGE_INVENTORY_ITEM_BOOT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_cap.tga", IMAGE_INVENTORY_ITEM_HELM, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_fairy.tga", IMAGE_INVENTORY_ITEM_FAIRY, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_wing.tga", IMAGE_INVENTORY_ITEM_WING, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_weapon(L).tga", IMAGE_INVENTORY_ITEM_RIGHT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_weapon(R).tga", IMAGE_INVENTORY_ITEM_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_upper.tga", IMAGE_INVENTORY_ITEM_ARMOR, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_gloves.tga", IMAGE_INVENTORY_ITEM_GLOVES, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_lower.tga", IMAGE_INVENTORY_ITEM_PANTS, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_ring.tga", IMAGE_INVENTORY_ITEM_RING, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_necklace.tga", IMAGE_INVENTORY_ITEM_NECKLACE, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_money.tga", IMAGE_INVENTORY_MONEY, GL_LINEAR);
	LoadBitmap("Interface\\newui_exit_00.tga", IMAGE_INVENTORY_EXIT_BTN, GL_LINEAR);
	LoadBitmap("Interface\\newui_repair_00.tga", IMAGE_INVENTORY_REPAIR_BTN, GL_LINEAR);
	LoadBitmap("Interface\\newui_Bt_openshop.tga", IMAGE_INVENTORY_MYSHOP_OPEN_BTN, GL_LINEAR);
	LoadBitmap("Interface\\newui_Bt_closeshop.tga", IMAGE_INVENTORY_MYSHOP_CLOSE_BTN, GL_LINEAR);
	LoadBitmap("Interface\\newui_bankjewell_btn.tga", IMAGE_INVENTORY_JEWEL_BTN, GL_LINEAR);
	if (GMProtect->IsInvExt())
	{
		LoadBitmap("Interface\\newui_expansion_btn.tga", IMAGE_INVENTORY_EXPANSION_BTN, GL_LINEAR);
	}

}

void SEASON3B::CNewUIMyInventory::UnloadImages()
{
	DeleteBitmap(IMAGE_INVENTORY_MYSHOP_CLOSE_BTN);
	DeleteBitmap(IMAGE_INVENTORY_MYSHOP_OPEN_BTN);
	DeleteBitmap(IMAGE_INVENTORY_REPAIR_BTN);
	DeleteBitmap(IMAGE_INVENTORY_EXIT_BTN);
	DeleteBitmap(IMAGE_INVENTORY_MONEY);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_NECKLACE);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_RING);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_PANTS);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_GLOVES);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_ARMOR);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_RIGHT);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_LEFT);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_WING);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_FAIRY);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_HELM);
	DeleteBitmap(IMAGE_INVENTORY_ITEM_BOOT);
	DeleteBitmap(IMAGE_INVENTORY_BACK_BOTTOM);
	DeleteBitmap(IMAGE_INVENTORY_BACK_RIGHT);
	DeleteBitmap(IMAGE_INVENTORY_BACK_LEFT);
	DeleteBitmap(IMAGE_INVENTORY_BACK_TOP2);
	DeleteBitmap(IMAGE_INVENTORY_BACK_TOP);
	DeleteBitmap(IMAGE_INVENTORY_BACK);
	DeleteBitmap(IMAGE_INVENTORY_JEWEL_BTN);
	if (GMProtect->IsInvExt())
	{
		DeleteBitmap(IMAGE_INVENTORY_EXPANSION_BTN);
	}
}

void SEASON3B::CNewUIMyInventory::RenderFrame()
{
	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		RenderInventoryInterface(m_Pos.x, m_Pos.y, 0);
	}
	else
	{
		RenderImage(IMAGE_INVENTORY_BACK, m_Pos.x, m_Pos.y, 190.f, 429.f);
		RenderImage(IMAGE_INVENTORY_BACK_TOP2, m_Pos.x, m_Pos.y, 190.f, 64.f);
		RenderImage(IMAGE_INVENTORY_BACK_LEFT, m_Pos.x, m_Pos.y + 64, 21.f, 320.f);
		RenderImage(IMAGE_INVENTORY_BACK_RIGHT, m_Pos.x + INVENTORY_WIDTH - 21, m_Pos.y + 64, 21.f, 320.f);
		RenderImage(IMAGE_INVENTORY_BACK_BOTTOM, m_Pos.x, m_Pos.y + INVENTORY_HEIGHT - 45, 190.f, 45.f);
	}
}

void SEASON3B::CNewUIMyInventory::RenderEquippedItem()
{
	for (int i = 0; i < MAX_EQUIPMENT_INDEX; i++)
	{
		if (i == EQUIPMENT_HELM)
		{
			if (Hero->GetBaseClass() == CLASS_DARK)
			{
				continue;
			}
		}

		if ((i == EQUIPMENT_GLOVES) && (Hero->GetBaseClass() == CLASS_RAGEFIGHTER))
			continue;

		EnableAlphaTest();

		EQUIPMENT_ITEM* info = &m_EquipmentSlots[i];

		RenderImageF(info->dwBgImage, info->x, info->y, info->width, info->height);
		DisableAlphaBlend();

		ITEM* pEquipmentItemSlot = &CharacterMachine->Equipment[i];

		if (pEquipmentItemSlot->Type != -1)
		{
			Script_Item* pItemAttr = GMItemMng->find(pEquipmentItemSlot->Type);
			int iLevel = (pEquipmentItemSlot->Level >> 3) & 15;
			int iMaxDurability = calcMaxDurability(pEquipmentItemSlot, pItemAttr, iLevel);

			if (i == EQUIPMENT_RING_LEFT || i == EQUIPMENT_RING_RIGHT)
			{
				if (pEquipmentItemSlot->Type == ITEM_HELPER + 20 && iLevel == 1
					|| iLevel == 2)
				{
					continue;
				}
			}

			if ((pEquipmentItemSlot->bPeriodItem == true) && (pEquipmentItemSlot->bExpiredPeriod == false))
				continue;

			if (pEquipmentItemSlot->Durability <= 0)
				glColor4f(1.f, 0.f, 0.f, 0.25f);
			else if (pEquipmentItemSlot->Durability <= (iMaxDurability * 0.2f))
				glColor4f(1.f, 0.15f, 0.f, 0.25f);
			else if (pEquipmentItemSlot->Durability <= (iMaxDurability * 0.3f))
				glColor4f(1.f, 0.5f, 0.f, 0.25f);
			else if (pEquipmentItemSlot->Durability <= (iMaxDurability * 0.5f))
				glColor4f(1.f, 1.f, 0.f, 0.25f);
			else if (IsEquipable(i, pEquipmentItemSlot) == false)
				glColor4f(1.f, 0.f, 0.f, 0.25f);
			else
			{
				continue;
			}

			EnableAlphaTest();
			if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
			{
				RenderColor(info->x + 2, info->y, info->width - 2, info->height - 2);
			}
			else
			{
				RenderColor(info->x + 1, info->y, info->width - 4, info->height - 4);
			}
			EndRenderColor();
		}
	}

	if (CNewUIInventoryCtrl::GetPickedItem() && m_iPointedSlot != -1)
	{
		ITEM* pItemObj = CNewUIInventoryCtrl::GetPickedItem()->GetItem();
		ITEM* pEquipmentItemSlot = &CharacterMachine->Equipment[m_iPointedSlot];
		if (pItemObj && (pEquipmentItemSlot->Type != -1 || false == IsEquipable(m_iPointedSlot, pItemObj)) && !((Hero->GetBaseClass() == CLASS_RAGEFIGHTER) && (m_iPointedSlot == EQUIPMENT_GLOVES)))
		{
			EQUIPMENT_ITEM* info = &m_EquipmentSlots[m_iPointedSlot];

			EnableAlphaTest();
			glColor4f(0.9f, 0.1f, 0.1f, 0.4f);

			if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
			{
				RenderColor(info->x + 2, info->y, info->width - 2, info->height - 2);
			}
			else
			{
				RenderColor(info->x + 1, info->y, info->width - 4, info->height - 4);
			}
			EndRenderColor();
		}
	}

	if (m_iPointedSlot != -1 && m_pNewUI3DRenderMng)
	{
		if (m_iPointedSlot != m_iBackPointed)
		{
			if (m_iPointedSlot == EQUIPMENT_HELPER)
			{
				if (CharacterMachine->Equipment[EQUIPMENT_HELPER].Type == ITEM_HELPER + 4)
				{
					SendRequestPetInfo(PET_TYPE_DARK_HORSE, 0, EQUIPMENT_HELPER);
				}
			}
			if (m_iPointedSlot == EQUIPMENT_WEAPON_LEFT)
			{
				if (CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type == ITEM_HELPER + 5)
				{
					SendRequestPetInfo(PET_TYPE_DARK_SPIRIT, 0, EQUIPMENT_WEAPON_LEFT);
				}
			}
		}
		m_pNewUI3DRenderMng->RenderUI2DEffect(TOOLTIP_CAMERA_Z_ORDER, UI2DEffectCallback, this, m_iPointedSlot, 0);
	}
	m_iBackPointed = m_iPointedSlot;
}

void SEASON3B::CNewUIMyInventory::RenderButtons()
{
	EnableAlphaTest();
	if (!g_pNewUISystem->IsVisible(INTERFACE_NPCSHOP))
	{
		if (!g_pNewUISystem->IsVisible(INTERFACE_TRADE))
		{
			if (!g_pNewUISystem->IsVisible(INTERFACE_DEVILSQUARE))
			{
				if (!g_pNewUISystem->IsVisible(INTERFACE_BLOODCASTLE))
				{
					if (!g_pNewUISystem->IsVisible(INTERFACE_MIXINVENTORY))
					{
						if (!g_pNewUISystem->IsVisible(INTERFACE_LUCKYITEMWND))
						{
							if (!g_pNewUISystem->IsVisible(INTERFACE_STORAGE))
							{
								if (m_bRepairEnableLevel == true)
									m_BtnRepair.Render();
								if (m_bMyShopOpen == true)
									m_BtnMyShop.Render();
								m_BtnJewel.Render();
							}
						}
					}
				}
			}
		}
	}
	m_BtnExit.Render();

	if (CanOpenExpansionInterface())
		this->UnLockBtnExpansion();
	else
		this->LockBtnExpansion();

	if (GMProtect->IsInvExt())
	{
		m_BtnExpansion.Render();
	}

	DisableAlphaBlend();
}

void SEASON3B::CNewUIMyInventory::RenderInventoryDetails()
{
	EnableAlphaTest();

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	g_pRenderText->SetTextColor(255, 255, 255, 255);

	if (gmProtect->LookAndFeel == 1 || gmProtect->LookAndFeel == 2)
	{
		g_pRenderText->SetBgColor(20, 20, 20, 255);
		g_pRenderText->RenderText(m_Pos.x + 35, m_Pos.y + 12, GlobalText[223], 120, 0, RT3_SORT_CENTER);
		g_pRenderText->SetBgColor(0, 0, 0, 0);

		DWORD dwZen = CharacterMachine->Gold;

		unicode::t_char Text[256] = { 0, };
		ConvertGold(dwZen, Text);

		g_pRenderText->SetTextColor(getGoldColor(dwZen));

		RenderImage(BITMAP_INVENTORY + 11, m_Pos.x + 50.0, m_Pos.y + 368, 113.0, 18.0, 0, 0, 0.8828125, 0.5625);
		g_pRenderText->RenderText((int)m_Pos.x + 60, (int)m_Pos.y + 368, Text, 0, 18);

		g_pRenderText->SetBgColor(40, 40, 40, 255);
		g_pRenderText->SetTextColor(255, 220, 150, 255);
		g_pRenderText->RenderText(m_Pos.x + 30, m_Pos.y + 373, "Zen", 0, 0);
	}
	else
	{
		g_pRenderText->RenderText(m_Pos.x, m_Pos.y + 12, GlobalText[223], INVENTORY_WIDTH, 0, RT3_SORT_CENTER);

		DWORD dwZen = CharacterMachine->Gold;

		unicode::t_char Text[256] = { 0, };
		ConvertGold(dwZen, Text);

		g_pRenderText->SetTextColor(getGoldColor(dwZen));

		RenderImage(IMAGE_INVENTORY_MONEY, m_Pos.x + 11, m_Pos.y + 364, 170.f, 26.f);

		g_pRenderText->RenderText((int)m_Pos.x + 50, (int)m_Pos.y + 371, Text);
	}

	g_pRenderText->SetFont(g_hFont);

	DisableAlphaBlend();
}

bool SEASON3B::CNewUIMyInventory::EquipmentWindowProcess()
{
	if (m_iPointedSlot == -1)
		return false;

	if (SEASON3B::IsRelease(VK_LBUTTON))
	{
		CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();

		if (pPickedItem)
		{
			ITEM* pItemObj = pPickedItem->GetItem();

			if (pItemObj->bPeriodItem && pItemObj->bExpiredPeriod)
			{
				g_pChatListBox->AddText("", GlobalText[2285], SEASON3B::TYPE_ERROR_MESSAGE);
				CNewUIInventoryCtrl::BackupPickedItem();
				ResetMouseLButton();
				return false;
			}

			ITEM* pEquipmentItemSlot = &CharacterMachine->Equipment[m_iPointedSlot];

			if (pEquipmentItemSlot && pEquipmentItemSlot->Type != -1)
				return true;

			if (g_ChangeRingMgr->CheckChangeRing(pItemObj->Type))
			{
				ITEM* pItemRingLeft = &CharacterMachine->Equipment[EQUIPMENT_RING_LEFT];
				ITEM* pItemRingRight = &CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT];

				if (g_ChangeRingMgr->CheckChangeRing(pItemRingLeft->Type) || g_ChangeRingMgr->CheckChangeRing(pItemRingRight->Type))
				{
					g_pChatListBox->AddText("", GlobalText[3089], SEASON3B::TYPE_ERROR_MESSAGE);
					CNewUIInventoryCtrl::BackupPickedItem();
					ResetMouseLButton();
					return false;
				}
			}

			if (IsEquipable(m_iPointedSlot, pItemObj))
			{
				int iTargetIndex = m_iPointedSlot;
				int InvenIndex = this->IsInvenctrlSame(pPickedItem->GetOwnerInventory());

				if (InvenIndex != -1)
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					iSourceIndex = this->SlotCntSendSrcInvenIndex(iSourceIndex, InvenIndex);
					SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iSourceIndex, pItemObj, REQUEST_EQUIPMENT_INVENTORY, iTargetIndex);
					return true;
				}

				if (pPickedItem->GetOwnerInventory() == g_pMixInventory->GetInventoryCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					SendRequestEquipmentItem(g_MixRecipeMgr.GetMixInventoryEquipmentIndex(), iSourceIndex, pItemObj, REQUEST_EQUIPMENT_INVENTORY, iTargetIndex);
					return true;
				}

				if (g_pStorageInventory->IsInvenctrlSame(pPickedItem->GetOwnerInventory()) != -1)
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					iSourceIndex = g_pStorageInventory->GetSourceStorage(iSourceIndex, pPickedItem->GetOwnerInventory());
					g_pStorageInventory->SendRequestItemToMyInven(pItemObj, iSourceIndex, iTargetIndex);
					return true;
				}

				if (pPickedItem->GetOwnerInventory() == g_pTrade->GetMyInvenCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					g_pTrade->SendRequestItemToMyInven(pItemObj, iSourceIndex, iTargetIndex);
					return true;
				}
				if (pPickedItem->GetOwnerInventory() == g_pMyShopInventory->GetInventoryCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					SendRequestEquipmentItem(REQUEST_EQUIPMENT_MYSHOP, MAX_MY_INVENTORY_INDEX + iSourceIndex, pItemObj, REQUEST_EQUIPMENT_INVENTORY, iTargetIndex);
					return true;
				}
				if (pItemObj->ex_src_type == ITEM_EX_SRC_EQUIPMENT && !EquipmentItem)
				{
					if (pPickedItem->GetSourceLinealPos() != m_iPointedSlot)
					{
						int iSourceIndex = pItemObj->lineal_pos;
						SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, iSourceIndex, pItemObj, REQUEST_EQUIPMENT_INVENTORY, iTargetIndex);
						return true;
					}
					CNewUIInventoryCtrl::BackupPickedItem();
				}
			}

			return false;
		}

		if (GetRepairMode() != REPAIR_MODE_ON)
		{
			ITEM* pEquippedItem = &CharacterMachine->Equipment[m_iPointedSlot];
			if (pEquippedItem->Type >= 0)
			{
				if (gMapManager.WorldActive == WD_10HEAVEN)
				{
					ITEM* pEquippedPetItem = &CharacterMachine->Equipment[EQUIPMENT_HELPER];
					bool bPicked = true;

					if (m_iPointedSlot == EQUIPMENT_HELPER || m_iPointedSlot == EQUIPMENT_WING)
					{
						if (((m_iPointedSlot == EQUIPMENT_HELPER) && !gCharacterManager.IsEquipedWing()))
						{
							bPicked = false;
						}
						else if (((m_iPointedSlot == EQUIPMENT_WING) && !((pEquippedPetItem->Type == ITEM_HELPER + 3) || (pEquippedPetItem->Type == ITEM_HELPER + 4) || (pEquippedPetItem->Type == ITEM_HELPER + 37))))
						{
							bPicked = false;
						}
					}

					if (bPicked == true)
					{
						if (CNewUIInventoryCtrl::CreatePickedItem(NULL, pEquippedItem))
						{
							UnequipItem(m_iPointedSlot);
						}
					}
				}
				else
				{
					if (CNewUIInventoryCtrl::CreatePickedItem(NULL, pEquippedItem))
					{
						UnequipItem(m_iPointedSlot);
					}
				}
			}
			return false;
		}

		if (GetRepairMode() == REPAIR_MODE_ON)
		{
			ITEM* pEquippedItem = &CharacterMachine->Equipment[m_iPointedSlot];

			if (pEquippedItem == NULL)
			{
				return true;
			}
			else if (IsRepairBan(pEquippedItem) == true)
			{
				return true;
			}
			else
			{
				if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP) && g_pNPCShop->IsRepairShop())
				{
					SendRequestRepair(m_iPointedSlot, 0);
				}
				else if (m_bRepairEnableLevel == true)
				{
					SendRequestRepair(m_iPointedSlot, 1);
				}
				return true;
			}
		}
	}

	if (SEASON3B::IsPress(VK_RBUTTON) && this->GetRepairMode() != REPAIR_MODE_ON && EquipmentItem == false)
	{
		int iSrcIndex = this->m_iPointedSlot;

		if (iSrcIndex != -1)
		{
			ResetMouseRButton();

			ITEM* pItemObj = &CharacterMachine->Equipment[iSrcIndex];

			if (pItemObj->Type != -1)
			{
				bool bPicked = true;

				if (gMapManager.WorldActive == WD_10HEAVEN)
				{
					ITEM* pEquippedPetItem = &CharacterMachine->Equipment[EQUIPMENT_HELPER];

					if (m_iPointedSlot == EQUIPMENT_HELPER || m_iPointedSlot == EQUIPMENT_WING)
					{
						if (((m_iPointedSlot == EQUIPMENT_HELPER) && !gCharacterManager.IsEquipedWing()))
						{
							bPicked = false;
						}
						else if (((m_iPointedSlot == EQUIPMENT_WING) && !((pEquippedPetItem->Type == ITEM_HELPER + 3) || (pEquippedPetItem->Type == ITEM_HELPER + 4) || (pEquippedPetItem->Type == ITEM_HELPER + 37))))
						{
							bPicked = false;
						}
					}
				}

				if (bPicked)
				{
					int iDstIndex = this->FindEmptySlot(pItemObj);

					if (iDstIndex != -1)
					{
						if (CNewUIInventoryCtrl::CreatePickedItem(NULL, pItemObj))
						{
							SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, iSrcIndex, pItemObj, REQUEST_EQUIPMENT_INVENTORY, iDstIndex + MAX_EQUIPMENT_INDEX);

							CNewUIInventoryCtrl::GetPickedItem()->HidePickedItem();

							this->UnequipItem(iSrcIndex);

							SetCharacterClass(Hero);

							PlayBuffer(SOUND_GET_ITEM01);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool SEASON3B::CNewUIMyInventory::InventoryProcess()
{
	if (!SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, INVENTORY_WIDTH, INVENTORY_HEIGHT))
	{
		if (!g_pInvenExpansion->CheckExpansionInventory())
			return false;
	}

	CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();

	bool success = false;

	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		success = m_pNewInventoryCtrl[i] != NULL;
	}

	if (success && pPickedItem && SEASON3B::IsRelease(VK_LBUTTON))
	{
		ITEM* pPickItem = pPickedItem->GetItem();

		if (pPickItem == NULL)
			return false;

		int InvenIndex = this->IsInvenctrlSame(pPickedItem->GetOwnerInventory());

		if (InvenIndex == -1)
		{
			if (g_pStorageInventory->IsInvenctrlSame(pPickedItem->GetOwnerInventory()) == -1)
			{
				if (pPickedItem->GetOwnerInventory() == g_pTrade->GetMyInvenCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					int iTargetIndex = this->GetPickItemTarget(pPickedItem);

					if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
					{
						iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
						g_pTrade->SendRequestItemToMyInven(pPickItem, iSourceIndex, MAX_EQUIPMENT_INDEX + iTargetIndex);
						return true;
					}
				}
				else if (pPickedItem->GetOwnerInventory() == g_pLuckyItemWnd->GetInventoryCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					int iTargetIndex = this->GetPickItemTarget(pPickedItem);
					if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
					{
						iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
						SendRequestEquipmentItem(g_pLuckyItemWnd->SetWndAction(eLuckyItem_Move), iSourceIndex,
							pPickItem, REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iTargetIndex);
						return true;
					}
				}
				else if (pPickedItem->GetOwnerInventory() == g_pMixInventory->GetInventoryCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					int iTargetIndex = this->GetPickItemTarget(pPickedItem);
					if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
					{
						iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
						SendRequestEquipmentItem(g_MixRecipeMgr.GetMixInventoryEquipmentIndex(), iSourceIndex,
							pPickItem, REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iTargetIndex);
						return true;
					}
				}
				else if (pPickedItem->GetOwnerInventory() == g_pMyShopInventory->GetInventoryCtrl())
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					int iTargetIndex = this->GetPickItemTarget(pPickedItem);

					if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
					{
						iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
						SendRequestEquipmentItem(REQUEST_EQUIPMENT_MYSHOP, MAX_MY_INVENTORY_INDEX + iSourceIndex, pPickItem, REQUEST_EQUIPMENT_INVENTORY, iTargetIndex + MAX_EQUIPMENT_INDEX);
						return true;
					}
				}
				else if (pPickItem->ex_src_type == ITEM_EX_SRC_EQUIPMENT)
				{
					int iSourceIndex = pPickedItem->GetSourceLinealPos();
					int iTargetIndex = this->GetPickItemTarget(pPickedItem);
					if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
					{
						iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
						SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, iSourceIndex, pPickItem, REQUEST_EQUIPMENT_INVENTORY, iTargetIndex + MAX_EQUIPMENT_INDEX);
						return true;
					}
				}
			}
			else
			{
				int iSourceIndex = pPickedItem->GetSourceLinealPos();
				int iTargetIndex = GetPickItemTarget(pPickedItem);

				if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
				{
					iSourceIndex = g_pStorageInventory->GetSourceStorage(iSourceIndex, pPickedItem->GetOwnerInventory());
					iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
					g_pStorageInventory->SendRequestItemToMyInven(pPickItem, iSourceIndex, iTargetIndex + MAX_EQUIPMENT_INDEX);
					return true;
				}
			}
		}
		else
		{
			int iSourceIndex = pPickedItem->GetSourceLinealPos();

			iSourceIndex = this->SlotCntSendSrcInvenIndex(iSourceIndex, InvenIndex);

			int iTargetIndex = this->GetPickItemTarget(pPickedItem);

			if (pPickItem->Type == ITEM_POTION + 13
				|| pPickItem->Type == ITEM_POTION + 14
				|| pPickItem->Type == ITEM_POTION + 16
				|| pPickItem->Type == ITEM_POTION + 42
				|| pPickItem->Type == ITEM_POTION + 43
				|| pPickItem->Type == ITEM_POTION + 44
				|| pPickItem->Type == ITEM_POTION + 160
				|| pPickItem->Type == ITEM_POTION + 161
				)
			{
				ITEM* pItem = this->FindItem(iTargetIndex, m_InvenIndex);

				if (pItem)
				{
					int	iType = pItem->Type;
					int	iLevel = (pItem->Level >> 3) & 15;
					int	iDurability = pItem->Durability;

					bool bSuccess = true;

					if (iType > ITEM_WING + 6
						&& iType != ITEM_HELPER + 30
						&& !(iType >= ITEM_WING + 36 && iType <= ITEM_WING + 43)
						&& !(ITEM_WING + 130 <= iType && iType <= ITEM_WING + 134)
#ifdef PBG_ADD_NEWCHAR_MONK_ITEM
						&& !(iType >= ITEM_WING + 49 && iType <= ITEM_WING + 50)
						&& (iType != ITEM_WING + 135)
#endif //PBG_ADD_NEWCHAR_MONK_ITEM
						)
					{
						bSuccess = false;
					}

					if (iType == ITEM_BOW + 7 || iType == ITEM_BOW + 15)
					{
						bSuccess = false;
					}

					if ((pPickItem->Type == ITEM_POTION + 13 && iLevel >= 6) || (pPickItem->Type == ITEM_POTION + 14 && iLevel >= 9))
					{
						bSuccess = false;
					}

					if (pPickItem->Type == ITEM_POTION + 13 && iType == ITEM_HELPER + 37 && iDurability != 255)
					{
						SEASON3B::CFenrirRepairMsgBox* pMsgBox = NULL;
						SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CFenrirRepairMsgBoxLayout), &pMsgBox);
						pMsgBox->SetSourceIndex(iSourceIndex);

						int iIndex = this->GetInvenSlotIndex(pItem->x, pItem->y, m_InvenIndex);
						pMsgBox->SetTargetIndex(MAX_EQUIPMENT_INDEX + iIndex);
						pPickedItem->HidePickedItem();
						return true;
					}

					if (pPickItem->Type == ITEM_POTION + 42)
					{
						if (g_SocketItemMgr.IsSocketItem(pItem))
						{
							bSuccess = false;
						}
						else if (pItem->Jewel_Of_Harmony_Option != 0)
						{
							bSuccess = false;
						}
						else
						{
							StrengthenItem strengthitem = g_pUIJewelHarmonyinfo->GetItemType(static_cast<int>(pItem->Type));

							if (strengthitem == SI_None)
							{
								bSuccess = false;
							}
						}
					}

					if (pPickItem->Type == ITEM_POTION + 43 || pPickItem->Type == ITEM_POTION + 44)
					{
						if (g_SocketItemMgr.IsSocketItem(pItem))
						{
							bSuccess = false;
						}
						else if (pItem->Jewel_Of_Harmony_Option == 0)
						{
							bSuccess = false;
						}
					}
					if (Check_LuckyItem(pItem->Type))
					{
						bSuccess = false;
						if (pPickItem->Type == ITEM_POTION + 161)
						{
							if (pItem->Jewel_Of_Harmony_Option == 0)
								bSuccess = true;
						}
						else if (pPickItem->Type == ITEM_POTION + 160)
						{
							if (pItem->Durability > 0)
								bSuccess = true;
						}
					}

					if (bSuccess)
					{
						int iTargetBaseIndex = m_pNewInventoryCtrl[m_InvenIndex]->FindBaseIndexByITEM(pItem);

						iTargetBaseIndex = this->SlotCntSendDstInvenIndex(iTargetBaseIndex, m_InvenIndex);

						SendRequestUse(iSourceIndex, MAX_EQUIPMENT_INDEX + iTargetBaseIndex);
						PlayBuffer(SOUND_GET_ITEM01);
						return true;
					}
				}
			}
			else if (iTargetIndex != -1)
			{
				ITEM* pItem = this->FindItem(iTargetIndex, m_InvenIndex);
				if (pItem)
				{
					bool bOverlay = m_pNewInventoryCtrl[m_InvenIndex]->IsOverlayItem(pPickItem, pItem);

					if (bOverlay)
					{
						SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iSourceIndex, pPickItem, REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iTargetIndex);
					}
				}
			}

			if (iTargetIndex != -1 && m_pNewInventoryCtrl[m_InvenIndex]->CanMove(iTargetIndex, pPickItem))
			{
				if (pPickedItem->GetSourceLinealPos() == iTargetIndex && m_InvenIndex == InvenIndex)
				{
					SEASON3B::CNewUIInventoryCtrl::BackupPickedItem();
				}
				else
				{
					iTargetIndex = this->SlotCntSendDstInvenIndex(iTargetIndex, m_InvenIndex);
					SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iSourceIndex, pPickItem, REQUEST_EQUIPMENT_INVENTORY, MAX_EQUIPMENT_INDEX + iTargetIndex);
				}
				return true;
			}
		}
	}
	else if (success && GetRepairMode() == REPAIR_MODE_OFF && SEASON3B::IsPress(VK_RBUTTON))
	{
		ResetMouseRButton();

#ifdef JEWEL_BANK
		if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_JEWEL_BANK) == true)
		{
			g_pNewUIJewelBank->ProcessInvenItem();
		}
#endif

		if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE))
		{
			g_pStorageInventory->ProcessMyInvenItemAutoMove();
		}
		else if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY)
			&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP) == false
			&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_TRADE) == false
			&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_DEVILSQUARE) == false
			&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_BLOODCASTLE) == false
			&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_LUCKYITEMWND) == false
			&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MIXINVENTORY) == false)
		{
			int InvenIndex = -1;

			ITEM* pItem = this->FindItemAtPt(MouseX, MouseY, &InvenIndex);

			if (pItem == NULL)
			{
				return false;
			}

			int iIndex = this->GetInvenSlotIndex(pItem->x, pItem->y, InvenIndex);
			if (pItem->Type == ITEM_POTION + 10)
			{
				if (!Teleport)
				{
					SendRequestUse(iIndex, 0);
					return true;
				}
			}

			if ((pItem->Type >= ITEM_POTION + 0 && pItem->Type <= ITEM_POTION + 9)
				|| (pItem->Type == ITEM_POTION + 20 && ((pItem->Level >> 3) & 15) == 0)
				|| (pItem->Type >= ITEM_POTION + 35 && pItem->Type <= ITEM_POTION + 40)
				|| (pItem->Type >= ITEM_POTION + 46 && pItem->Type <= ITEM_POTION + 50)
				|| (pItem->Type == ITEM_POTION + 11 && ((pItem->Level >> 3) & 15) == 14)
				|| (pItem->Type >= ITEM_POTION + 70 && pItem->Type <= ITEM_POTION + 71)
				|| (pItem->Type >= ITEM_POTION + 72 && pItem->Type <= ITEM_POTION + 77)
				|| pItem->Type == ITEM_HELPER + 60
				|| pItem->Type == ITEM_POTION + 94
				|| (pItem->Type >= ITEM_POTION + 85 && pItem->Type <= ITEM_POTION + 87)
				|| (pItem->Type >= ITEM_POTION + 97 && pItem->Type <= ITEM_POTION + 98)
				|| pItem->Type == ITEM_HELPER + 81
				|| pItem->Type == ITEM_HELPER + 82
				|| pItem->Type == ITEM_POTION + 133
				)
			{
				SendRequestUse(iIndex, 0);
				return true;
			}
			else if ((pItem->Type >= ITEM_POTION + 78 && pItem->Type <= ITEM_POTION + 82))
			{
				std::list<eBuffState> secretPotionbufflist;
				secretPotionbufflist.push_back(eBuff_SecretPotion1);
				secretPotionbufflist.push_back(eBuff_SecretPotion2);
				secretPotionbufflist.push_back(eBuff_SecretPotion3);
				secretPotionbufflist.push_back(eBuff_SecretPotion4);
				secretPotionbufflist.push_back(eBuff_SecretPotion5);

				if (g_isCharacterBufflist((&Hero->Object), secretPotionbufflist) == eBuffNone) {
					SendRequestUse(iIndex, 0);
					return true;
				}
				else {
					SEASON3B::CreateOkMessageBox(GlobalText[2530], RGBA(255, 30, 0, 255));
				}
			}
			else if ((pItem->Type >= ITEM_HELPER + 54 && pItem->Type <= ITEM_HELPER + 57) || (pItem->Type == ITEM_HELPER + 58 && Hero->GetBaseClass() == CLASS_DARK_LORD))
			{
				bool result = true;
				WORD point[5] = { 0, };

				point[0] = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
				point[1] = CharacterAttribute->Dexterity + CharacterAttribute->AddDexterity;
				point[2] = CharacterAttribute->Vitality + CharacterAttribute->AddVitality;
				point[3] = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;
				point[4] = CharacterAttribute->Charisma + CharacterAttribute->AddCharisma;

#ifdef PBG_ADD_NEWCHAR_MONK
				char nStat[MAX_CLASS][5] =
#else //PBG_ADD_NEWCHAR_MONK
				char nStat[6][5] =
#endif //PBG_ADD_NEWCHAR_MONK
				{
					18, 18, 15, 30,	0,
					28, 20, 25, 10,	0,
					22, 25, 20, 15,	0,
					26, 26, 26, 26,	0,
					26, 20, 20, 15, 25,
					21, 21, 18, 23,	0,
#ifdef PBG_ADD_NEWCHAR_MONK
					32, 27, 25, 20, 0,
#endif //PBG_ADD_NEWCHAR_MONK
				};
				point[pItem->Type - (ITEM_HELPER + 54)] -= nStat[Hero->GetBaseClass()][pItem->Type - (ITEM_HELPER + 54)];

				if (point[pItem->Type - (ITEM_HELPER + 54)] < (pItem->Durability * 10))
				{
					result = false;
				}

				if (result == false)
				{
					SetStandbyItemKey(pItem->Key);
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CUsePartChargeFruitMsgBoxLayout));
				}
				else
				{
					SendRequestUse(iIndex, 0);
					return true;
				}
			}
			else if (pItem->Type == ITEM_HELPER + 58 && Hero->GetBaseClass() != CLASS_DARK_LORD)
			{
				SEASON3B::CreateOkMessageBox(GlobalText[1905]);
				return true;
			}
			else if (pItem->Type == ITEM_HELPER + 29)
			{
				if (true == IsUnitedMarketPlace())
				{
					char	szOutputText[512];
					sprintf(szOutputText, "%s %s", GlobalText[3014], GlobalText[3015]);

					SEASON3B::CreateOkMessageBox(szOutputText);
					return true;
				}

				if (Hero->SafeZone == false)
				{
					SEASON3B::CreateOkMessageBox(GlobalText[2330]);
				}
				else
				{
					SendRequestEventZoneOpenTime(4, ((pItem->Level >> 3) & 15));
					SetStandbyItemKey(pItem->Key);
					return true;
				}
			}
			else if (pItem->Type == ITEM_HELPER + 46)
			{
				BYTE byPossibleLevel = CaculateFreeTicketLevel(FREETICKET_TYPE_DEVILSQUARE);
				SendRequestEventZoneOpenTime(1, byPossibleLevel);
			}
			else if (pItem->Type == ITEM_HELPER + 47)
			{
				BYTE byPossibleLevel = CaculateFreeTicketLevel(FREETICKET_TYPE_BLOODCASTLE);
				SendRequestEventZoneOpenTime(2, byPossibleLevel);
			}
			else if (pItem->Type == ITEM_HELPER + 48)
			{
				if (Hero->SafeZone || gMapManager.InHellas())
				{
					g_pChatListBox->AddText("", GlobalText[1238], SEASON3B::TYPE_ERROR_MESSAGE);
				}
				else
				{
					SendRequestUse(iIndex, 0);
				}
			}
			else if (pItem->Type == ITEM_HELPER + 61)
			{
				BYTE byPossibleLevel = CaculateFreeTicketLevel(FREETICKET_TYPE_CURSEDTEMPLE);
				SendRequestEventZoneOpenTime(5, byPossibleLevel);
			}
			else if (pItem->Type == ITEM_HELPER + 121)
			{
				if (Hero->SafeZone == false)
				{
					SEASON3B::CreateOkMessageBox(GlobalText[2330]);
				}
				else
				{
					SendRequestEventZoneOpenTime(4, ((pItem->Level >> 3) & 15));
					SetStandbyItemKey(pItem->Key);
					return true;
				}
			}
			else if (pItem->Type == ITEM_HELPER + 51)
			{
				SendRequestEventZoneOpenTime(5, ((pItem->Level >> 3) & 15));
				return true;
			}
			else if (pItem->Type == ITEM_POTION + 19)
			{
				SendRequestEventZoneOpenTime(1, ((pItem->Level >> 3) & 15));
				return true;
			}
			else if (pItem->Type == ITEM_HELPER + 18)
			{
				if (pItem->Level == 0)
				{
					g_pChatListBox->AddText("", GlobalText[2089], SEASON3B::TYPE_ERROR_MESSAGE);
				}
				else
				{
					SendRequestEventZoneOpenTime(2, ((pItem->Level >> 3) & 15) - 1);
				}
				return true;
			}
			else if ((pItem->Type >= ITEM_ETC + 0 && pItem->Type < ITEM_ETC + MAX_ITEM_INDEX)
				|| (pItem->Type >= ITEM_WING + 7 && pItem->Type <= ITEM_WING + 14)
				|| (pItem->Type >= ITEM_WING + 16 && pItem->Type <= ITEM_WING + 19)
				|| (pItem->Type == ITEM_WING + 20)
				|| (pItem->Type >= ITEM_WING + 21 && pItem->Type <= ITEM_WING + 24)
				|| (pItem->Type == ITEM_WING + 35)
				|| (pItem->Type == ITEM_WING + 44)
				|| (pItem->Type == ITEM_WING + 47)
				|| (pItem->Type == ITEM_WING + 46)
				|| (pItem->Type == ITEM_WING + 45)
				|| (pItem->Type == ITEM_WING + 48)
				|| (pItem->Type == ITEM_ETC + 29)
				|| (pItem->Type == ITEM_ETC + 28)
				)
			{
				bool bReadBookGem = true;

				if ((pItem->Type == ITEM_ETC + 18)
					|| (pItem->Type == ITEM_ETC + 28)
					|| pItem->Type == ITEM_WING + 45
					|| (pItem->Type == ITEM_WING + 46)
					|| (pItem->Type == ITEM_WING + 44)
					)
				{
					if (g_csQuest.getQuestState2(QUEST_CHANGE_UP_3) != QUEST_END)
						bReadBookGem = false;
				}
				if (pItem->Type == ITEM_WING + 48)
				{
					int Level = CharacterAttribute->Level;
					if (Level < 220)
						bReadBookGem = false;
				}
				if (bReadBookGem)
				{
					WORD Strength, Energy;
					Strength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
					Energy = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;

					if (CharacterAttribute->Level >= GMItemMng->find(pItem->Type)->RequireLevel && Energy >= pItem->RequireEnergy && Strength >= pItem->RequireStrength)
					{
						SendRequestUse(iIndex, 0);
					}
					return true;
				}
			}
			else if (pItem->Type == ITEM_HELPER + 15)
			{
				int Level = CharacterAttribute->Level;

				if (Level >= 10)
				{
					bool bEquipmentEmpty = true;
					for (int i = 0; i < MAX_EQUIPMENT; i++)
					{
						if (CharacterMachine->Equipment[i].Type != -1)
						{
							bEquipmentEmpty = false;
						}
					}
					int Class = CharacterAttribute->Class;

					if (bEquipmentEmpty == true)
					{
						if (pItem->Level == 32)
						{
							if (gCharacterManager.GetBaseClass(Class) != CLASS_DARK_LORD)
							{
								SEASON3B::CreateOkMessageBox(GlobalText[1905]);
								return true;
							}
						}
						SetStandbyItemKey(pItem->Key);
						SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CUseFruitMsgBoxLayout));
						return true;
					}
					else
					{
						SEASON3B::CreateOkMessageBox(GlobalText[1909]);
						return true;
					}
				}
				else
				{
					SEASON3B::CreateOkMessageBox(GlobalText[749]);
					return true;
				}
			}
			else if (pItem->Type == ITEM_HELPER + 11)
			{
				bool bUse = false;
				int  Level = (pItem->Level >> 3) & 15;
				switch (Level)
				{
				case 0:
					bUse = true;
					break;
				case 1:
					if (Hero->GuildStatus != G_MASTER)
						bUse = true;
					break;
				}
				if (bUse)
				{
					SendRequestUse(iIndex, 0);
					return true;
				}
			}
			else if (pItem->Type == ITEM_HELPER + 69)
			{
				if (g_PortalMgr.IsRevivePositionSaved())
				{
					if (g_PortalMgr.IsPortalUsable())
					{
						SetStandbyItemKey(pItem->Key);
						SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CUseReviveCharmMsgBoxLayout));
					}
					else
					{
						SEASON3B::CreateOkMessageBox(GlobalText[2608]);
					}
				}
			}
			else if (pItem->Type == ITEM_HELPER + 70)
			{
				if (g_PortalMgr.IsPortalUsable())
				{
					if (pItem->Durability == 2)
					{
						if (g_PortalMgr.IsPortalPositionSaved())
						{
							SEASON3B::CreateOkMessageBox(GlobalText[2610]);
						}
						else
						{
							SetStandbyItemKey(pItem->Key);
							SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CUsePortalCharmMsgBoxLayout));
						}
					}
					else if (pItem->Durability == 1)
					{
						SetStandbyItemKey(pItem->Key);
						SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CReturnPortalCharmMsgBoxLayout));
					}
				}
				else
				{
					SEASON3B::CreateOkMessageBox(GlobalText[2608]);
				}
			}
			else if (pItem->Type == ITEM_HELPER + 66)
			{
				SetStandbyItemKey(pItem->Key);
				SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CUseSantaInvitationMsgBoxLayout));
			}
			else if (this->IsInvenItem(pItem->Type))
			{
				if (MouseLButton || MouseLButtonPop || MouseLButtonPush)
					return false;

				if (pItem->Durability == 0)
					return false;

				BYTE iChangeInvenItemStatus = 0;
				if (pItem->Durability == 0xFF)
				{
					iChangeInvenItemStatus = 0xFE;
				}
				else
				{
					iChangeInvenItemStatus = 0xFF;
				}
				SendRequestEquippingInventoryItem(iIndex + MAX_EQUIPMENT_INDEX, iChangeInvenItemStatus);
				return false;
			}
			else if (pItem->Type == ITEM_POTION + 162 || pItem->Type == ITEM_POTION + 163)
			{
				SendRequestUse(iIndex, 0);
			}
			else
			{
				if (SEASON3B::IsRepeat(VK_CONTROL))
				{
					SetStandbyItemKey(pItem->Key);
					SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CNewUIItem3DInputMsgBoxLayout));
					return true;
				}
				else
				{
					return MoveItemProcess();
				}
			}
		}
		else
		{
			return MoveItemProcess();
		}
	}
	else
	{
		if (success && GetRepairMode() == REPAIR_MODE_ON && SEASON3B::IsPress(VK_LBUTTON))
		{
			int InvenIndex = -1;

			ITEM* pItem = this->FindItemAtPt(MouseX, MouseY, &InvenIndex);

			if (pItem)
			{
				if (IsRepairBan(pItem) == true)
				{
					return false;
				}
				else
				{
					int iIndex = this->GetInvenSlotIndex(pItem->x, pItem->y, InvenIndex) + MAX_EQUIPMENT_INDEX;

					if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP) && g_pNPCShop->IsRepairShop())
					{
						SendRequestRepair(iIndex, 0);
					}
					else if (m_bRepairEnableLevel == true)
					{
						SendRequestRepair(iIndex, 1);
					}
				}
			}
			else
			{
				return false;
			}
			return true;
		}
	}
	return false;
}

bool SEASON3B::CNewUIMyInventory::BtnProcess()
{
	POINT ptExitBtn1 = { m_Pos.x + 169, m_Pos.y + 7 };

	if (SEASON3B::IsRelease(VK_LBUTTON) && CheckMouseIn(ptExitBtn1.x, ptExitBtn1.y, 13, 12))
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY);
		return true;
	}
	if (m_BtnExit.UpdateMouseEvent() == true)
	{
		if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MYSHOP_INVENTORY))
		{
			g_pNewUISystem->Hide(SEASON3B::INTERFACE_MYSHOP_INVENTORY);
		}
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY);
		return true;
	}

	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP) == false
		&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_TRADE) == false
		&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_DEVILSQUARE) == false
		&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_BLOODCASTLE) == false
		&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_LUCKYITEMWND) == false
		&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MIXINVENTORY) == false
		&& g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE) == false)
	{
		if (m_bRepairEnableLevel == true && m_BtnRepair.UpdateMouseEvent() == true)
		{
			ToggleRepairMode();
			return true;
		}

		if (m_bMyShopOpen == true && m_BtnMyShop.UpdateMouseEvent() == true)
		{
			if (m_MyShopMode == MYSHOP_MODE_OPEN)
			{
				ChangeMyShopButtonStateClose();
				g_pNewUISystem->Show(SEASON3B::INTERFACE_MYSHOP_INVENTORY);
			}
			else if (m_MyShopMode == MYSHOP_MODE_CLOSE)
			{
				ChangeMyShopButtonStateOpen();
				g_pNewUISystem->Hide(SEASON3B::INTERFACE_MYSHOP_INVENTORY);
				g_pNewUISystem->Hide(SEASON3B::INTERFACE_PURCHASESHOP_INVENTORY);
			}

			return true;
		}
		if (m_BtnJewel.UpdateMouseEvent() == true)
		{
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY_JEWEL);
			return true;
		}
	}

	if (m_bMyShopOpen == false || m_BtnMyShop.UpdateMouseEvent() == false)
	{
		if (m_BtnExpansion.UpdateMouseEvent() && !CNewUIInventoryCtrl::GetPickedItem()
			&& GMProtect->IsInvExt())
		{
			if (m_ExpansionState == TRUE)
			{
				this->SetExpansionCloseState();
				g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY_EXTENSION);
			}
			else
			{
				this->SetExpansionOpenState();
				g_pNewUISystem->Show(SEASON3B::INTERFACE_INVENTORY_EXTENSION);
			}
		}

		if (SEASON3B::IsRelease(VK_LBUTTON) && m_StraightMode == TRUE)
		{
			this->SendStraighItem();
			m_StraightMode = FALSE;
		}
	}

	return false;
}

void SEASON3B::CNewUIMyInventory::RenderItemToolTip(int iSlotIndex)
{
	if (m_iPointedSlot != -1 && CNewUIInventoryCtrl::GetPickedItem() == NULL)
	{
		ITEM* pEquipmentItemSlot = &CharacterMachine->Equipment[iSlotIndex];

		if (pEquipmentItemSlot->Type != -1)
		{
			int iTargetX = m_EquipmentSlots[iSlotIndex].x + m_EquipmentSlots[iSlotIndex].width / 2;
			int iTargetY = m_EquipmentSlots[iSlotIndex].y + m_EquipmentSlots[iSlotIndex].height / 2;

			pEquipmentItemSlot->bySelectedSlotIndex = iSlotIndex;

			if (m_RepairMode == REPAIR_MODE_OFF)
			{
				g_pNewItemTooltip->RenderItemTooltip(iTargetX, iTargetY, pEquipmentItemSlot, false);
			}
			else
			{
				RenderRepairInfo(iTargetX, iTargetY, pEquipmentItemSlot, false);
			}
		}
	}
}

bool SEASON3B::CNewUIMyInventory::CanRegisterItemHotKey(int iType)
{
	switch (iType)
	{
	case ITEM_POTION + 0:
	case ITEM_POTION + 1:
	case ITEM_POTION + 2:
	case ITEM_POTION + 3:
	case ITEM_POTION + 4:
	case ITEM_POTION + 5:
	case ITEM_POTION + 6:
	case ITEM_POTION + 7:
	case ITEM_POTION + 8:
	case ITEM_POTION + 9:
	case ITEM_POTION + 10:
	case ITEM_POTION + 20:
	case ITEM_POTION + 35:
	case ITEM_POTION + 36:
	case ITEM_POTION + 37:
	case ITEM_POTION + 38:
	case ITEM_POTION + 39:
	case ITEM_POTION + 40:
	case ITEM_POTION + 46:
	case ITEM_POTION + 47:
	case ITEM_POTION + 48:
	case ITEM_POTION + 49:
	case ITEM_POTION + 50:
	case ITEM_POTION + 70:
	case ITEM_POTION + 71:
	case ITEM_POTION + 78:
	case ITEM_POTION + 79:
	case ITEM_POTION + 80:
	case ITEM_POTION + 81:
	case ITEM_POTION + 82:
	case ITEM_POTION + 94:
	case ITEM_POTION + 85:
	case ITEM_POTION + 86:
	case ITEM_POTION + 87:
	case ITEM_POTION + 133:
		return true;
	}

	return false;
}

bool SEASON3B::CNewUIMyInventory::CanOpenMyShopInterface()
{
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP)
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE)
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MIXINVENTORY)
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_LUCKYITEMWND)
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_TRADE)
		|| gMapManager.IsCursedTemple()
		)
	{
		return false;
	}
	return true;
}

bool SEASON3B::CNewUIMyInventory::IsRepairEnableLevel()
{
	return m_bRepairEnableLevel;
}

void SEASON3B::CNewUIMyInventory::SetRepairEnableLevel(bool bOver)
{
	m_bRepairEnableLevel = bOver;
}

void SEASON3B::CNewUIMyInventory::ChangeMyShopButtonStateOpen()
{
	m_MyShopMode = MYSHOP_MODE_OPEN;
	m_BtnMyShop.UnRegisterButtonState();
	m_BtnMyShop.RegisterButtonState(BUTTON_STATE_UP, IMAGE_INVENTORY_MYSHOP_OPEN_BTN, 0);
	m_BtnMyShop.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_INVENTORY_MYSHOP_OPEN_BTN, 1);
	m_BtnMyShop.ChangeImgIndex(IMAGE_INVENTORY_MYSHOP_OPEN_BTN, 0);
	m_BtnMyShop.ChangeToolTipText(GlobalText[1125], true);


	SetButtonInterface(&m_BtnMyShop, m_Pos.x, m_Pos.y, 2);
}

void SEASON3B::CNewUIMyInventory::ChangeMyShopButtonStateClose()
{
	m_MyShopMode = MYSHOP_MODE_CLOSE;
	m_BtnMyShop.UnRegisterButtonState();
	m_BtnMyShop.RegisterButtonState(BUTTON_STATE_UP, IMAGE_INVENTORY_MYSHOP_CLOSE_BTN, 0);
	m_BtnMyShop.RegisterButtonState(BUTTON_STATE_DOWN, IMAGE_INVENTORY_MYSHOP_CLOSE_BTN, 1);
	m_BtnMyShop.ChangeImgIndex(IMAGE_INVENTORY_MYSHOP_CLOSE_BTN, 0);
	m_BtnMyShop.ChangeToolTipText(GlobalText[1127], true);

	SetButtonInterface(&m_BtnMyShop, m_Pos.x, m_Pos.y, 12);

}

void SEASON3B::CNewUIMyInventory::LockMyShopButtonOpen()
{
	m_BtnMyShop.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
	m_BtnMyShop.ChangeTextColor(RGBA(100, 100, 100, 255));
	m_BtnMyShop.Lock();
	m_BtnMyShop.ChangeToolTipText(GlobalText[1125], true);
}

void SEASON3B::CNewUIMyInventory::UnlockMyShopButtonOpen()
{
	m_BtnMyShop.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnMyShop.ChangeTextColor(RGBA(255, 255, 255, 255));
	m_BtnMyShop.UnLock();
	m_BtnMyShop.ChangeToolTipText(GlobalText[1125], true);
}

void SEASON3B::CNewUIMyInventory::ToggleRepairMode()
{
	if (m_RepairMode == REPAIR_MODE_OFF)
	{
		SetRepairMode(true);
}
	else if (m_RepairMode == REPAIR_MODE_ON)
	{
		SetRepairMode(false);
	}
}

bool SEASON3B::CNewUIMyInventory::IsItem(short int siType, bool bcheckPick)
{
	if (bcheckPick == true)
	{
		CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();

		if (pPickedItem)
		{
			ITEM* pItemObj = pPickedItem->GetItem();

			if (pItemObj->Type == siType) return true;
		}
	}

	ITEM* pholyitemObj = this->FindTypeItem(siType);

	if (pholyitemObj)
		return true;

	return false;
}

int SEASON3B::CNewUIMyInventory::GetNumItemByKey(DWORD dwItemKey)
{
	int NumItemByKey = 0;

	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		NumItemByKey += m_pNewInventoryCtrl[i]->GetNumItemByKey(dwItemKey);
	}
	return NumItemByKey;
}

int SEASON3B::CNewUIMyInventory::GetNumItemByType(short sItemType)
{
	int NumItemByType = 0;

	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		NumItemByType += m_pNewInventoryCtrl[i]->GetNumItemByType(sItemType);

	}
	return NumItemByType;
}

int SEASON3B::CNewUIMyInventory::GetPointedItemIndex()
{
	if (m_iPointedSlot == -1)
	{
		int iPointedItemIndex = -1;

		for (int i = 0; i < this->GetInvenEnableCnt(); i++)
		{
			iPointedItemIndex = m_pNewInventoryCtrl[i]->GetPointedSquareIndex();

			if (iPointedItemIndex != -1)
				return this->SlotCntSendSrcInvenIndex(iPointedItemIndex, i) + MAX_EQUIPMENT_INDEX;
		}
		if (iPointedItemIndex == -1)
			return -1;
		else
			return iPointedItemIndex + MAX_EQUIPMENT_INDEX;
	}
	else
	{
		return m_iPointedSlot;
	}
}

int SEASON3B::CNewUIMyInventory::FindManaItemIndex()
{
	for (int i = ITEM_POTION + 6; i >= ITEM_POTION + 4; i--)
	{
		int iIndex = FindItemReverseIndex(i);
		if (iIndex != -1)
		{
			return iIndex;
		}
	}

	return -1;
}

void SEASON3B::CNewUIMyInventory::ResetMouseLButton()
{
	MouseLButton = false;
	MouseLButtonPop = false;
	MouseLButtonPush = false;
}

void SEASON3B::CNewUIMyInventory::ResetMouseRButton()
{
	MouseRButton = false;
	MouseRButtonPop = false;
	MouseRButtonPush = false;
}

BOOL SEASON3B::CNewUIMyInventory::IsInvenItem(const short sType)
{
	BOOL bInvenItem = FALSE;

	if ((sType >= ITEM_HELPER + 128 && sType <= ITEM_HELPER + 134)
		|| (sType >= ITEM_HELPER + 177 && sType <= ITEM_HELPER + 179)
		|| sType == ITEM_HELPER + 476
		)
	{
		bInvenItem = TRUE;
	}

	return bInvenItem;
}

BOOL SEASON3B::CNewUIMyInventory::IsUsedItem(const short sType)
{
	BOOL bInvenItem = FALSE;

	if ((sType == ITEM_POTION + 162 || sType == ITEM_HELPER + 163)
		|| (sType >= ITEM_POTION + 169 && sType <= ITEM_HELPER + 179)
		|| sType == ITEM_HELPER + 162
		|| sType == ITEM_HELPER + 163
		|| sType == ITEM_HELPER + 169
		)
	{
		bInvenItem = TRUE;
	}

	return bInvenItem;
}

void SEASON3B::CNewUIMyInventory::ExpansionInit()
{
	m_StraightMode = FALSE;
	m_ExpansionState = FALSE;
	m_InvenIndex = 0;
	unknown_988 = 0;
}

void SEASON3B::CNewUIMyInventory::SetExpansionOpenState()
{
	m_ExpansionState = TRUE;

	for (int i = 1; i < this->GetInvenEnableCnt(); i++)
	{
		m_pNewInventoryCtrl[i]->UnlockInventory();
		m_pNewInventoryCtrl[i]->ShowInventory();
	}
}

void SEASON3B::CNewUIMyInventory::SetExpansionCloseState()
{
	m_ExpansionState = FALSE;

	for (int i = 1; i < this->GetInvenEnableCnt(); i++)
	{
		m_pNewInventoryCtrl[i]->LockInventory();
		m_pNewInventoryCtrl[i]->HideInventory();
	}
}

void SEASON3B::CNewUIMyInventory::LockBtnExpansion()
{
	m_BtnExpansion.Lock();
	m_BtnExpansion.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
	m_BtnExpansion.ChangeTextColor(RGBA(100, 100, 100, 255));
}

void SEASON3B::CNewUIMyInventory::UnLockBtnExpansion()
{
	m_BtnExpansion.UnLock();
	m_BtnExpansion.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
	m_BtnExpansion.ChangeTextColor(RGBA(255, 255, 255, 255));
}

bool SEASON3B::CNewUIMyInventory::IsStraightMode()
{
	bool success = false;

	if (m_BtnExpansion.GetBTState() == BUTTON_STATE_DOWN
		|| m_BtnExpansion.GetBTState() == BUTTON_STATE_OVER && CNewUIInventoryCtrl::GetPickedItem())
	{
		m_StraightMode = TRUE;
		success = true;
	}
	else
	{
		m_StraightMode = FALSE;
	}

	if (!m_StraightMode)
		success = false;

	return success;
}

bool SEASON3B::CNewUIMyInventory::CanOpenExpansionInterface()
{
	return this->IsVisible();
}

int SEASON3B::CNewUIMyInventory::FindInvenIndex(int index)
{
	int InvenIndex = 0;

	if (index < 0 || index >= 64)
	{
		if (index < 64 || index >= 96)
		{
			if (index < 96 || index >= 128)
			{
				if (index < 128 || index >= 160)
				{
					if (index < 160 || index > 192)
						InvenIndex = 5;
					else
						InvenIndex = 4;
				}
				else
				{
					InvenIndex = 3;
				}
			}
			else
			{
				InvenIndex = 2;
			}
		}
		else
		{
			InvenIndex = 1;
		}
	}

	return InvenIndex;
}

int SEASON3B::CNewUIMyInventory::SlotCntSendDstInvenIndex(int index, int InvenIndex)
{
	if (m_InvenIndex)
		index += 8 * (4 * InvenIndex - 4) + 64;
	return index;
}

int SEASON3B::CNewUIMyInventory::SlotCntSendSrcInvenIndex(int index, int InvenIndex)
{
	if (InvenIndex > 0)
		index += 8 * (4 * InvenIndex - 4) + 64;
	return index;
}

int SEASON3B::CNewUIMyInventory::SlotCntReceiveInvenIndex(int index, int Size)
{
	if (index > 204)
		return -1;
	if (index >= Size)
	{
		if (index - Size < 0)
			return index;
		index = SlotCntReceiveInvenIndex(index - Size, 32);
	}
	return index;
}

void SEASON3B::CNewUIMyInventory::RenderExpansionInven()
{
	for (int i = 1; i < this->GetInvenEnableCnt(); i++)
	{
		if (m_pNewInventoryCtrl[i])
		{
			m_pNewInventoryCtrl[i]->Render(false);
		}
	}
}

void SEASON3B::CNewUIMyInventory::SetInvenPos()
{
	for (int i = 1; i < this->GetInvenEnableCnt(); i++)
	{
		int x = g_pInvenExpansion->GetIEStartPos(i - 1)->x;
		int y = g_pInvenExpansion->GetIEStartPos(i - 1)->y;

		m_pNewInventoryCtrl[i]->SetPos(x, y);
		m_pNewInventoryCtrl[i]->LockInventory();
		m_pNewInventoryCtrl[i]->HideInventory();
	}
}

bool SEASON3B::CNewUIMyInventory::SendStraighItem()
{
	bool success;
	CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();

	if (pPickedItem != NULL)
	{
		ITEM* pPickItem = pPickedItem->GetItem();

		int nDstIndex;
		if (pPickItem && (nDstIndex = FindEmptySlot(pPickItem), nDstIndex != -1))
		{
			int iSourceIndex = pPickItem->lineal_pos + m_pNewInventoryCtrl[0]->GetNumberOfColumn() * pPickItem->ex_src_type;

			SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, iSourceIndex + MAX_EQUIPMENT_INDEX, pPickItem, REQUEST_EQUIPMENT_INVENTORY, nDstIndex + MAX_EQUIPMENT_INDEX);
			success = true;
		}
		else
		{
			success = false;
		}
	}
	else
	{
		success = false;
	}
	return success;
}

int SEASON3B::CNewUIMyInventory::GetPickItemTarget(CNewUIPickedItem* pPickedItem)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		int iTargetIndex = pPickedItem->GetTargetLinealPos(m_pNewInventoryCtrl[i]);
		if (iTargetIndex != -1)
		{
			m_InvenIndex = i;
			return iTargetIndex;
		}
	}
	return -1;
}

int SEASON3B::CNewUIMyInventory::IsInvenctrlSame(CNewUIInventoryCtrl* InventoryCtrl)
{
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (InventoryCtrl == m_pNewInventoryCtrl[i])
		{
			return i;
		}
	}
	return -1;
}

int SEASON3B::CNewUIMyInventory::GetInvenSlotIndex(int cx, int cy, int InvenIndex)
{
	int Index = cx + 8 * cy;

	if (InvenIndex > 0)
		Index = cx + 8 * (cy + 4 * InvenIndex - 4) + 64;

	return Index;
}

ITEM* SEASON3B::CNewUIMyInventory::FindItemMousePt(int x, int y, int* InvenIndex)
{
	ITEM* pItem = NULL;
	(*InvenIndex) = 0;

	for (int i = this->GetInvenEnableCnt(); (*InvenIndex) < i; i = this->GetInvenEnableCnt())
	{
		pItem = m_pNewInventoryCtrl[(*InvenIndex)]->FindItemAtPt(x, y);
		if (pItem)
		{
			break;
		}
		++(*InvenIndex);
	}
	return pItem;
}

void SEASON3B::CNewUIMyInventory::LockInventory(int InvenIndex)
{
	if (InvenIndex == (MAX_INVENT_EXPANSION + 1))
	{
		for (int i = 0; i < this->GetInvenEnableCnt(); i++)
		{
			m_pNewInventoryCtrl[i]->LockInventory();
		}
	}
	else
	{
		m_pNewInventoryCtrl[InvenIndex]->LockInventory();
	}
}

void SEASON3B::CNewUIMyInventory::UnlockInventory(int InvenIndex)
{
	if (InvenIndex == (MAX_INVENT_EXPANSION + 1))
	{
		for (int i = 0; i < this->GetInvenEnableCnt(); i++)
		{
			m_pNewInventoryCtrl[i]->UnlockInventory();
		}
	}
	else
	{
		m_pNewInventoryCtrl[InvenIndex]->UnlockInventory();
	}
}

int SEASON3B::CNewUIMyInventory::GetItemCount(short int siType, int iLevel)
{
	int NumberOfItems = 0;
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		NumberOfItems += m_pNewInventoryCtrl[i]->GetItemCount(siType, iLevel);
	}
	return NumberOfItems;
}

int SEASON3B::CNewUIMyInventory::GetNumberOfItems()
{
	int NumberOfItems = 0;
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		NumberOfItems += m_pNewInventoryCtrl[i]->GetNumberOfItems();
	}
	return NumberOfItems;
}

ITEM* SEASON3B::CNewUIMyInventory::GetItem(int lineal_pos)
{
	ITEM* pItem = NULL;
	int linealPos = lineal_pos;
	int NumberOfItems = m_pNewInventoryCtrl[0]->GetNumberOfItems() - 1;

	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		if (lineal_pos <= NumberOfItems)
			return m_pNewInventoryCtrl[i]->GetItem(linealPos);

		if (i == MAX_INVENT_EXPANSION)
			return pItem;

		linealPos -= m_pNewInventoryCtrl[i]->GetNumberOfItems();
		NumberOfItems += m_pNewInventoryCtrl[i + 1]->GetNumberOfItems();
	}
	return pItem;
}

bool SEASON3B::CNewUIMyInventory::IsCheckInvenIndex(int InvenIndex)
{
	bool success = true;
	if (!m_pNewInventoryCtrl[InvenIndex])
		success = false;
	if (InvenIndex < 0 || InvenIndex >= this->GetInvenEnableCnt())
		success = false;

	return success;
}

ITEM* SEASON3B::CNewUIMyInventory::FindItemAtPt(int x, int y, int* InvenIndex)
{
	ITEM* pItem = NULL;
	(*InvenIndex) = 0;

	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		(*InvenIndex) = i;
		pItem = m_pNewInventoryCtrl[i]->FindItemAtPt(x, y);
		if (pItem)
			break;
	}
	return pItem;
}

ITEM* SEASON3B::CNewUIMyInventory::FindTypeItem(short int siType)
{
	ITEM* pItem = NULL;
	for (int i = 0; i < this->GetInvenEnableCnt(); i++)
	{
		pItem = m_pNewInventoryCtrl[i]->FindTypeItem(siType);

		if (pItem != NULL)
			break;
	}
	return pItem;
}

void SEASON3B::CNewUIMyInventory::SetInvenEnableCnt(int size)
{
	m_ExpansionBack.clear();
	for (int i = 0; i < size + 1 && i < (MAX_INVENT_EXPANSION + 1); ++i)
	{
		m_ExpansionBack.push_back(i);
	}
}

int SEASON3B::CNewUIMyInventory::GetInvenEnableCnt()
{
	int size = m_ExpansionBack.size();

	if (size <= (MAX_INVENT_EXPANSION + 1))
		return size;
	else
		return 0;
}


bool SEASON3B::CNewUIMyInventory::IsItemAutoMove()
{
	return m_bItemAutoMove;
}

bool SEASON3B::CNewUIMyInventory::MoveItemProcess()
{
	CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();

	if (!(g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_TRADE)
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_STORAGE)
		|| g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_NPCSHOP))
		&& pPickedItem == NULL)
	{
		if (SEASON3B::IsPress(VK_RBUTTON) && this->GetRepairMode() != REPAIR_MODE_ON && EquipmentItem == false)
		{
			int PosX = 0, PosY = 0;

			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MIXINVENTORY) && g_pMixInventory->GetMixState() == CNewUIMixInventory::MIX_READY)
			{
				PosX = g_pMixInventory->GetPos().x;
				PosY = g_pMixInventory->GetPos().y;

				int iMixType = g_MixRecipeMgr.GetMixInventoryEquipmentIndex();

				if (SEASON3B::CheckMouseIn(PosX, PosY, 190, 429))
				{
					if (this->ProcessItemAutoMove(g_pMixInventory->GetInventoryCtrl(), iMixType))
					{
						ResetMouseRButton();
						return true;
					}
				}
				else
				{
					if (this->ProcessMyInvenItemAutoMove(g_pMixInventory->GetInventoryCtrl(), iMixType))
					{
						ResetMouseRButton();
						return true;
					}
				}
			}
#ifdef LEM_ADD_LUCKYITEM
			else if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_LUCKYITEMWND))
			{
				PosX = g_pLuckyItemWnd->GetPos().x;
				PosY = g_pLuckyItemWnd->GetPos().y;

				int iMixType = g_pLuckyItemWnd->SetWndAction(eLUCKYITEM::eLuckyItem_Move);

				if (SEASON3B::CheckMouseIn(PosX, PosY, 190, 429))
				{
					if (this->ProcessItemAutoMove(g_pLuckyItemWnd->GetInventoryCtrl(), iMixType))
					{
						ResetMouseRButton();
						return true;
					}
				}
				else
				{
					if (this->ProcessMyInvenItemAutoMove(g_pLuckyItemWnd->GetInventoryCtrl(), iMixType))
					{
						ResetMouseRButton();
						return true;
					}
				}
			}
#endif
			else
			{
				int InvenIndex = -1;
				ITEM* pItemObj = this->FindItemAtPt(MouseX, MouseY, &InvenIndex);

				if (pItemObj)
				{
					ResetMouseRButton();
					int iSrcIndex = this->GetInvenSlotIndex(pItemObj->x, pItemObj->y, InvenIndex);

					Script_Item* pItemAttr = GMItemMng->find(pItemObj->Type);

					int iDstIndex = pItemAttr->m_byItemSlot;

					if (iDstIndex >= EQUIPMENT_WEAPON_RIGHT && iDstIndex <= EQUIPMENT_RING_LEFT)
					{
						ITEM* pItem = &CharacterMachine->Equipment[iDstIndex];
REPEAT_COM:
						if (pItem->Type != -1)
						{
							if ((iDstIndex == EQUIPMENT_WEAPON_RIGHT && !pItemAttr->TwoHand) || iDstIndex == EQUIPMENT_RING_RIGHT)
							{
								iDstIndex++;
								pItem = &CharacterMachine->Equipment[iDstIndex];
								goto REPEAT_COM;
							}
						}
						else
						{
							CNewUIInventoryCtrl* pSrc = this->GetInventoryCtrl(InvenIndex);

							if (this->IsEquipable(iDstIndex, pItemObj) && pSrc != NULL /*&& CNewUIInventoryCtrl::CreatePickedItem(pSrc, pItemObj)*/)
							{
								SetItemAutoMove(true);

								m_bInventoryBase = NULL;

								SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, iSrcIndex + MAX_EQUIPMENT_INDEX, pItemObj, REQUEST_EQUIPMENT_INVENTORY, iDstIndex);

								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

void SEASON3B::CNewUIMyInventory::SetItemAutoMove(bool bItemAutoMove)
{
	m_bItemAutoMove = bItemAutoMove;

	if (bItemAutoMove)
	{
		m_nBackupMouseX = MouseX;
		m_nBackupMouseY = MouseY;
	}
	else
	{
		m_nBackupMouseX = 0;
		m_nBackupMouseY = 0;
		m_bInventoryBase = NULL;
	}
}

bool SEASON3B::CNewUIMyInventory::ProcessItemAutoMove(CNewUIInventoryCtrl* pNewInventory, BYTE SourceFlag)
{
	if (IsItemAutoMove() == false)
	{
		ITEM* pItemObj = pNewInventory->FindItemAtPt(MouseX, MouseY);

		if (pItemObj)
		{
			int nDstIndex = this->FindEmptySlot(pItemObj);

			if (-1 != nDstIndex)
			{
				SetItemAutoMove(true);

				m_bInventoryBase = pNewInventory;

				int nSrcIndex = pNewInventory->FindBaseIndexByITEM(pItemObj);

				SendRequestEquipmentItem(SourceFlag, nSrcIndex, pItemObj, REQUEST_EQUIPMENT_INVENTORY, nDstIndex + MAX_EQUIPMENT_INDEX);

				PlayBuffer(SOUND_GET_ITEM01);
				return true;
			}
		}
	}
	return false;
}

bool SEASON3B::CNewUIMyInventory::ProcessMyInvenItemAutoMove(CNewUIInventoryCtrl* pNewInventory, BYTE TargetFlag)
{
	if (IsItemAutoMove() == false)
	{
		int InvenIndex = -1;

		ITEM* pItemObj = this->FindItemAtPt(MouseX, MouseY, &InvenIndex);

		if (pItemObj)
		{
			if (TargetFlag == g_MixRecipeMgr.GetMixInventoryEquipmentIndex() && !g_MixRecipeMgr.IsMixSource(pItemObj))
			{
				return false;
			}

			if (TargetFlag == g_pLuckyItemWnd->SetWndAction(eLUCKYITEM::eLuckyItem_Move) && !g_pLuckyItemWnd->Check_LuckyItem(pItemObj))
			{
				return false;
			}
			int nDstIndex = pNewInventory->FindEmptySlot(pItemObj);

			if (-1 != nDstIndex)
			{
				SetItemAutoMove(true);

				m_bInventoryBase = NULL;

				int nSrcIndex = this->GetInvenSlotIndex(pItemObj->x, pItemObj->y, InvenIndex);

				SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, nSrcIndex + MAX_EQUIPMENT_INDEX, pItemObj, TargetFlag, nDstIndex);

				PlayBuffer(SOUND_GET_ITEM01);
				return true;
			}
		}
	}
	return false;
}

void SEASON3B::CNewUIMyInventory::ProcessItemAutoMoveSuccess()
{
	if (IsItemAutoMove())
	{
		if (m_bInventoryBase != nullptr)
		{
			ITEM* pItemObj = m_bInventoryBase->FindItemAtPt(m_nBackupMouseX, m_nBackupMouseY);
			m_bInventoryBase->RemoveItem(pItemObj);
		}
		else
		{
			int InvenIndex = -1;
			ITEM* pItemObj = this->FindItemAtPt(m_nBackupMouseX, m_nBackupMouseY, &InvenIndex);
			int nSrcIndex = this->GetInvenSlotIndex(pItemObj->x, pItemObj->y, InvenIndex);
			this->DeleteItem(nSrcIndex);
		}
		SetItemAutoMove(false);
	}
}

void SEASON3B::CNewUIMyInventory::ProcessItemAutoMoveFailure()
{
	if (IsItemAutoMove())
	{
		SetItemAutoMove(false);
	}
}
