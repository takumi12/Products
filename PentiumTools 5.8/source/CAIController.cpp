#include "stdafx.h"
#include "ZzzAI.h"
#include "CComGem.h"
#include "MapManager.h"
#include "NewUICommon.h"
#include "NewUISystem.h"
#include "ZzzInterface.h"
#include "SkillManager.h"
#include "wsclientinline.h"
#include "CAIController.h"
#include "CGMProtect.h"

void RANGE_FIXED(MUAction_info* val, int min, int max, int condition)
{
	if (!(val->Percent >= min && val->Percent <= max))
	{
		val->Percent = condition;
	}
}

bool CAIController::IsRunning()
{
	return m_IsStart;
}

bool CAIController::CanUseAIController()
{
	bool success = false;

	if (CharacterAttribute->Level >= gmProtect->HelperActiveLevel)
	{
		if (sCharacter->SafeZone || gMapManager.InChaosCastle())
		{
			SEASON3B::CreateOkMessageBox(GlobalText[3574], -1);
			success = false;
		}
		else
		{
#ifdef INVENTORY_HELPER_ACTIVE
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
			{
				SEASON3B::CreateOkMessageBox(GlobalText[3575], -1);
				success = false;
			}
			else
			{
				if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MACRO_OFICIAL))
				{
					SEASON3B::CreateOkMessageBox(GlobalText[3576], -1);
					success = false;
				}
				else if (m_SkillBasic > 0 || m_SkillSecond[0].ID > 0 || m_SkillSecond[1].ID > 0 || sCharacter->GetBaseClass() == Fairy_Elf)
				{
					success = true;
				}
				else
				{
					SEASON3B::CreateOkMessageBox(GlobalText[3577], -1);
					success = false;
				}
			}
#else
			if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MACRO_OFICIAL))
			{
				SEASON3B::CreateOkMessageBox(GlobalText[3576], -1);
				success = false;
			}
			else if (m_SkillBasic > 0 || m_SkillSecond[0].ID > 0 || m_SkillSecond[1].ID > 0 || sCharacter->GetBaseClass() == Fairy_Elf)
			{
				success = true;
			}
			else
			{
				SEASON3B::CreateOkMessageBox(GlobalText[3577], -1);
				success = false;
			}
#endif // INVENTORY_HELPER_ACTIVE
		}
	}
	else
	{
		char Src[255];
		sprintf_s(Src, GlobalText[3573], gmProtect->HelperActiveLevel);
		SEASON3B::CreateOkMessageBox(Src, -1);
		success = false;
	}
	return success;
}

void CAIController::Start()
{
	if (!m_IsStart)
	{
		m_IsAutoAttack = g_pOption->IsAutoAttack();
		g_pOption->SetAutoAttack(!m_Combo);

		m_IsStart = true;
		m_SendDarkAttack = false;
		sMonsterATK = NULL;
		sMonsterPre = NULL;
		cur_item = NULL;
		m_StartPositionX = sCharacter->PositionX;
		m_StartPositionY = sCharacter->PositionY;
		m_DelayFindTarget = 0;
		m_WaitTimeReturn = 0;
		m_WaitTimeHealAT = 0;
		m_WaitTimeConsume = 0;
		m_WaitTimeBuffAT = 0;
		m_TimeCondition[0] = 0;
		m_TimeCondition[1] = 0;
		m_TimeAction = 0;
		m_TimeBuffParty = 0;
		m_IsReturn = false;

		for (int i = 0; i < 3; ++i)
		{
			m_DelayThird[i] = 0;
			for (int j = 0; j < MAX_PARTYS; ++j)
				m_DelayMemberParty[i][j] = 0;
		}

		SetPartyBuff();
		g_pMacroGaugeBar->StartAccumulate();
		g_pChatListBox->AddText("", GlobalText[3578], SEASON3B::TYPE_ERROR_MESSAGE, SEASON3B::TYPE_ALL_MESSAGE);
		g_pChatListBox->AddText("", GlobalText[3582], SEASON3B::TYPE_ERROR_MESSAGE, SEASON3B::TYPE_ALL_MESSAGE); // message "Certain amount of zen is spent every 5 minutes in implementing Official MU Helper"
	}
}

void CAIController::Stop()
{
	if (m_IsStart == true)
	{
		m_IsStart = false;
		g_pOption->SetAutoAttack(m_IsAutoAttack);
		g_pMacroGaugeBar->StopAccumulate();
		g_pChatListBox->AddText("", GlobalText[3579], SEASON3B::TYPE_ERROR_MESSAGE, SEASON3B::TYPE_ALL_MESSAGE);
	}
}

CAIController::CAIController(CHARACTER* Character)
{
	m_IsStart = false;
	NameCharacter = NULL;
	m_WaitTime = 200;
	ResetConfig();
	SetActiveCharacter(Character);
}

CAIController::~CAIController()
{
	if (NameCharacter)
	{
		SAFE_DELETE(NameCharacter);
	}
}

void CAIController::SetActiveCharacter(CHARACTER* pCha)
{
	sCharacter = pCha;
	if (NameCharacter)
	{
		SAFE_DELETE(NameCharacter);
	}
	int len = strlen(sCharacter->ID);
	NameCharacter = new char[len];
	m_SendDarkAttack = false;
	m_CounATK = 0;
	sMonsterATK = NULL;
	sMonsterPre = NULL;
	cur_item = NULL;
	prev_item = NULL;
}

void CAIController::ResetConfig()
{
	m_ATTKDistance = 1;
	m_Potion.Enable = false;
	m_Potion.Percent = 50;
	m_HealLife.Enable = false;
	m_HealLife.Percent = 50;
	m_DrainLife.Enable = false;
	m_DrainLife.Percent = 50;
	m_DistanceLong = false;
	m_DistanceReturn = false;
	m_DistanceOnSec = 0;
	m_SkillBasic = 0;
	m_Combo = false;
	m_PartyMode = false;
	m_PartyHeal.Enable = false;
	m_PartyHeal.Percent = 50;
	m_IsPartyBuff = false;
	m_PartyBuffSeconds = 0;
	m_DarkSpirit = false;
	m_BuffDuration = false;
	m_PickDistance = 1;
	m_RepairFix = false;
	m_Pick.Near = false;
	m_Pick.Select = true;
	m_Pick.Jewel = false;
	m_Pick.Ancient = false;
	m_Pick.Excellent = false;
	m_Pick.Zen = false;
	m_Pick.Extra = false;
	m_AutoFriend = false;
	m_AutoGuildMember = false;
	m_EliteManaPotion = false;

	memset(&m_SkillSecond, 0, sizeof(MUSkill_info));
	memset(&m_SkillSecond[1], 0, sizeof(MUSkill_info));
	memset(&m_SkillBuff, 0, sizeof(m_SkillBuff));

	for (int i = 0; i < MAX_PICK_EXTRA; ++i)
	{
		memset(m_Pick.Name[i], 0, sizeof(m_Pick.Name[i]));
	}
}

void CAIController::LoadDefaultConfg()
{
	m_ATTKDistance = 6;
	m_Potion.Enable = false;
	m_Potion.Percent = 50;
	m_HealLife.Enable = false;
	m_HealLife.Percent = 50;
	m_DrainLife.Enable = false;
	m_DrainLife.Percent = 50;
	m_DistanceLong = false;
	m_DistanceReturn = false;
	m_DistanceOnSec = 10;
	m_SkillBasic = 0;
	m_Combo = false;
	m_PartyMode = false;
	m_PartyHeal.Enable = false;
	m_PartyHeal.Percent = 50;
	m_IsPartyBuff = false;
	m_PartyBuffSeconds = 0;
	m_DarkSpirit = false;
	m_BuffDuration = false;
	m_PickDistance = 8;
	m_RepairFix = false;
	m_Pick.Near = true;
	m_Pick.Select = false;
	m_Pick.Jewel = false;
	m_Pick.Ancient = false;
	m_Pick.Excellent = false;
	m_Pick.Zen = false;
	m_Pick.Extra = false;
	m_AutoFriend = false;
	m_AutoGuildMember = false;
	m_EliteManaPotion = false;
	memset(&m_SkillSecond, 0, sizeof(MUSkill_info));
	memset(&m_SkillSecond[1], 0, sizeof(MUSkill_info));
	memset(&m_SkillBuff, 0, sizeof(m_SkillBuff));

	for (int i = 0; i < MAX_PICK_EXTRA; ++i)
	{
		memset(m_Pick.Name[i], 0, sizeof(m_Pick.Name[i]));
	}
}

void CAIController::SaveConfig()
{
	BYTE byPack[256];

	if (this->Pack(byPack))
	{
		SendRequestSaveHelperSetting(byPack, sizeof(byPack));
	}
	g_pChatListBox->AddText("", GlobalText[3580], SEASON3B::TYPE_SYSTEM_MESSAGE, SEASON3B::TYPE_ALL_MESSAGE);
}

float CAIController::GetDistance(CHARACTER* pCha)
{
	float fDistance = 0.0;

	if (pCha)
	{
		float fDis_x, fDis_y;
		fDis_x = (double)(sCharacter->PositionX - pCha->PositionX);
		fDis_y = (double)(sCharacter->PositionY - pCha->PositionY);
		fDistance = sqrtf(fDis_x * fDis_x + fDis_y * fDis_y);
	}
	return fDistance;
}

float CAIController::GetDistance(OBJECT* o)
{
	float fDistance = 0.0;

	if (o)
	{
		float fDis_x, fDis_y;
		fDis_x = (double)(sCharacter->PositionX - o->Position[0] / TERRAIN_SCALE);
		fDis_y = (double)(sCharacter->PositionY - o->Position[1] / TERRAIN_SCALE);
		fDistance = sqrtf(fDis_x * fDis_x + fDis_y * fDis_y);
	}
	return fDistance;
}

void CAIController::WhatToDoNext()
{
	MouseRButtonPush = 0;
	if (sMonsterATK && (!sMonsterATK->Object.Live || sMonsterATK->Object.CurrentAction == MONSTER01_DIE))
	{
		sMonsterATK = NULL;
		SelectedCharacter = -1;
	}

	time_t time_current = timeGetTime();
	if ((time_current - m_TimeAction) >= m_WaitTime)
	{
		m_WaitTime = 200;
		m_TimeAction = time_current;

		if (sCharacter->SafeZone)
		{
			g_pChatListBox->AddText("", GlobalText[3581], SEASON3B::TYPE_SYSTEM_MESSAGE, SEASON3B::TYPE_ALL_MESSAGE);
			SendRequestStartHelper(TRUE);
			return;
		}
		if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY) || g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MACRO_OFICIAL))
		{
			SendRequestStartHelper(TRUE);
			return;
		}

		if (m_DarkSpirit && !m_SendDarkAttack)
		{
			SendRequestPetCommand(PET_TYPE_DARK_SPIRIT, m_DarkSpiritType, 0xffff);
			m_SendDarkAttack = true;
		}

		DWORD MaxLife = CharacterAttribute->LifeMax;

		if (m_Potion.Enable)
		{
			if (CharacterAttribute->Life < MaxLife * m_Potion.Percent / 100
				&& sCharacter->Object.Live == true
				&& CharacterAttribute->Life > 0)
			{
				this->ConsumePotion();
			}
		}
		if (m_HealLife.Enable && (GetSkillIndex(AT_SKILL_HEALING) >= 0 || GetSkillIndex(413) >= 0))
		{
			if (CharacterAttribute->Life < MaxLife * m_HealLife.Percent / 100
				&& sCharacter->Object.Live == true
				&& CharacterAttribute->Life > 0 && this->HealAt(sCharacter))
			{
				return;
			}
		}
		if (m_PartyMode && m_PartyHeal.Enable && PartyNumber > 0)
		{
			if ((GetSkillIndex(AT_SKILL_HEALING) >= 0 || GetSkillIndex(413) >= 0) && PartyHeal())
			{
				return;
			}
		}
		if (m_PartyMode && m_IsPartyBuff && PartyNumber > 0)
		{
			if (PartyBuff())
			{
				return;
			}
		}
		if (m_DrainLife.Enable && (GetSkillIndex(214) >= 0 || GetSkillIndex(458) >= 0))
		{
			if (CharacterAttribute->Life < MaxLife * m_DrainLife.Percent / 100
				&& sCharacter->Object.Live == true
				&& CharacterAttribute->Life > 0 && this->ChooseTarget())
			{
				if (GetSkillIndex(214) >= 0)
				{
					if (SkillAt(214, sMonsterATK, true))
						return;
				}
				else if (GetSkillIndex(458) >= 0)
				{
					if (SkillAt(458, sMonsterATK, true))
						return;
				}
			}
		}

		if (m_BuffDuration && SelfBuffAt())
		{
			return;
		}

		if (CheckPickup())
		{
			PickupItem(cur_item); return;
		}

		if (m_RepairFix)
			RepairItem();

		if (sCharacter->Movement == 1)
		{
			if (m_IsReturn == true && IsEnemyAttaced() == true)
			{
				LetHeroStop(sCharacter, TRUE);
				SetPlayerStop(sCharacter);
			}
		}
		else
		{
			if (m_Combo && m_CounATK > 0 && sMonsterPre && sMonsterPre->Object.Live == true && sMonsterPre->Object.CurrentAction != MONSTER01_DIE)
			{
				if (sCharacter->Object.CurrentAction > PLAYER_STOP_RIDE_WEAPON && (sCharacter->Object.CurrentAction < PLAYER_FENRIR_STAND || sCharacter->Object.CurrentAction > PLAYER_FENRIR_STAND_WEAPON_LEFT))
				{
					return;
				}
				if (m_CounATK == 1 && SkillAt(m_SkillSecond[0].ID, sMonsterPre, true))
				{
					++m_CounATK;
					return;
				}
				else if (m_CounATK == 2 && SkillAt(m_SkillSecond[1].ID, sMonsterPre, true))
				{
					m_CounATK = 0;
					return;
				}
			}
			//if ( (!m_PartyMode || PartyNumber <= 0 || !m_IsPartyBuff || !PartyBuff()))
			//{
			if (m_DistanceReturn)
			{
				if (PlayerReached(m_StartPositionX, m_StartPositionY))
				{
					if (m_IsReturn == true)
					{
						m_IsReturn = false;
						sMonsterATK = NULL;
						SelectedCharacter = -1;
					}
					m_WaitTimeReturn = time_current;
				}
				else if ((time_current - m_WaitTimeReturn) > CLOCKS_PER_SEC * m_DistanceOnSec)
				{
					m_WaitTimeReturn += 5000;
					sMonsterPre = NULL;
					sMonsterATK = NULL;
					MoveTo(m_StartPositionX, m_StartPositionY);
					m_IsReturn = true;
					return;
				}
			}
			if ((m_Combo || !UseConditionalSkill(&m_SkillSecond[1], (unsigned int*)&m_TimeCondition[1]) && !UseConditionalSkill(&m_SkillSecond[0], (unsigned int*)&m_TimeCondition[0]))
				&& m_SkillBasic && ChooseTarget()
				&& (!m_Combo || m_CounATK || sCharacter->Object.CurrentAction <= PLAYER_STOP_RIDE_WEAPON || sCharacter->Object.CurrentAction >= PLAYER_FENRIR_STAND && sCharacter->Object.CurrentAction <= PLAYER_FENRIR_STAND_WEAPON_LEFT)
				&& SkillAt(m_SkillBasic, sMonsterATK, true))
			{
				m_CounATK = 1;
			}
			//}
		}
	}
}

bool CAIController::ConsumePotion()
{
	DWORD cur_time = timeGetTime();
	if (cur_time - m_WaitTimeConsume < 500)
	{
		return false;
	}
	int hp_good[9] = {
		ITEM_POTION,
		ITEM_POTION + 1,
		ITEM_POTION + 2,
		ITEM_POTION + 3,
		ITEM_POTION + 38,
		ITEM_POTION + 39,
		ITEM_POTION + 40,
		ITEM_POTION + 70,
		ITEM_POTION + 94
	};
	int use_index = -1;
	for (int i = 0; i < 9; i++)
	{
		use_index = g_pMyInventory->FindItemReverseIndex(hp_good[i]);
		if (-1 != use_index)
		{
			break;
		}
	}
	if (-1 != use_index)
	{
		SendRequestUse(use_index, 0);
	}
	m_WaitTimeConsume = cur_time;
	return true;
}

int CAIController::ChooseTarget()
{
	int EnemyNum = 0;
	CHARACTER* pChar = NULL;
	CHARACTER* pHero = NULL;

	if (sMonsterATK && sMonsterATK->Object.Kind != KIND_MONSTER)
		sMonsterATK = NULL;

	if (sMonsterATK && !CheckWall(sCharacter->PositionX, sCharacter->PositionY, sMonsterATK->PositionX, sMonsterATK->PositionY))
	{
		sMonsterATK = 0;
	}

	if (sCharacter->Object.CurrentAction != sCharacter->Object.PriorAction)
		m_DelayFindTarget = timeGetTime();

	time_t time_current = timeGetTime();

	if ((timeGetTime() - m_DelayFindTarget) > 5000)
	{
		m_DelayFindTarget = timeGetTime();
		sMonsterATK = 0;
	}
	if (!sMonsterATK)
		SelectedCharacter = -1;
	sMonsterPre = sMonsterATK;

	for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
	{
		pHero = gmCharacters->GetCharacter(i);

		if (pHero->Object.Live == true
			&& pHero->Object.Kind == KIND_MONSTER
			&& pHero->Object.CurrentAction != MONSTER01_DIE
			&& pHero->Object.CurrentAction != MONSTER01_APEAR)
		{
			if (GetDistance(pHero) <= (double)m_ATTKDistance || m_DistanceLong && IsEnemyAttacking(pHero))
			{
				++EnemyNum;
				pChar = RankEnemy(pChar, pHero);
			}
		}
	}
	sMonsterATK = pChar;
	return EnemyNum;
}

int CAIController::GetHostileEnemyNum()
{
	int EnemyNum = 0;
	CHARACTER* pHero = NULL;

	for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
	{
		pHero = gmCharacters->GetCharacter(i);
		if (pHero->Object.Live == true
			&& pHero->Object.Kind == KIND_MONSTER
			&& (double)m_ATTKDistance >= GetDistance(pHero))
		{
			if (IsEnemyAttacking(pHero))
				++EnemyNum;
		}
	}
	return EnemyNum;
}

CHARACTER* CAIController::RankEnemy(CHARACTER* Rank1, CHARACTER* Rank2)
{
	CHARACTER* pCha;

	if (Rank2 && !CheckWall(sCharacter->PositionX, sCharacter->PositionY, Rank2->PositionX, Rank2->PositionY))
	{
		Rank2 = NULL;
	}
	if (Rank1)
	{
		if (sMonsterATK && (sMonsterATK == Rank2 || sMonsterATK == Rank1))
		{
			pCha = sMonsterATK;
		}
		else if (Rank1 && Rank2)
		{
			if (GetDistance(Rank1) <= GetDistance(Rank2))
				pCha = Rank1;
			else
				pCha = Rank2;
		}
		else if (Rank1)
		{
			pCha = Rank1;
		}
		else if (Rank2)
		{
			pCha = Rank2;
		}
		else
		{
			pCha = NULL;
		}
	}
	else
	{
		pCha = Rank2;
	}
	return pCha;
}

bool CAIController::IsEnemyAttaced()
{
	CHARACTER* pCha;
	for (int i = 0; i < 400; ++i)
	{
		pCha = gmCharacters->GetCharacter(i);
		if (pCha->Object.Live == true
			&& pCha->Object.Kind == KIND_MONSTER
			&& pCha->Object.CurrentAction != MONSTER01_DIE
			&& pCha->Object.CurrentAction != MONSTER01_APEAR
			&& IsEnemyAttacking(pCha) == true)
		{
			return true;
		}
	}
	return false;
}

bool CAIController::IsEnemyAttacking(CHARACTER* Target)
{
	bool success = false;

	CHARACTER* pCha = gmCharacters->GetCharacter(Target->TargetCharacter);

	if (pCha)
	{
		switch (Target->Object.CurrentAction)
		{
		case MONSTER01_ATTACK1:
		case MONSTER01_ATTACK2:
		case MONSTER01_ATTACK3:
		case MONSTER01_ATTACK4:
		case MONSTER01_ATTACK5:
			if (pCha == sCharacter)
				success = true;
			break;
		default:
			break;
		}
	}
	return success;
}

bool CAIController::CheckPickup()
{
	int Rate = 0;
	ITEM_t* item = NULL;
	for (int i = 0; i < MAX_ITEMS; i++)
	{
		ITEM_t* temp = &Items[i];
		if (true == temp->Object.Live && temp->Object.Visible && (double)m_PickDistance >= this->GetDistance(&temp->Object))
		{
			int cur_Weight = this->RateItem(temp);
			if (cur_Weight > Rate)
			{
				Rate = cur_Weight;
				item = temp;
			}
		}
	}
	if (!item || item == this->prev_item)
	{
		return false;
	}
	this->cur_item = item;
	return true;
}

int CAIController::RateItem(ITEM_t* item)
{
	int percent = 0;

	MUPick_info* MuPick = &m_Pick;

	if (MuPick->Near)
	{
		percent = 100;
	}
	else if (MuPick->Select)
	{
		int selectedItemOption = item->Item.ExtOption % 0x04;

		if (MuPick->Jewel && -1 != COMGEM::Check_Jewel(item->Item.Type))
		{
			percent = 100;
		}
		if (MuPick->Ancient && selectedItemOption > 0)
		{
			percent = 100;
		}
		if (MuPick->Excellent && (item->Item.Option1 & 0x3F || selectedItemOption > 0))
		{
			percent = 100;
		}
		if (MuPick->Zen && ITEM_POTION + 15 == item->Item.Type)
		{
			percent = 100;
		}
		if (MuPick->Extra)
		{
			char Text[100];
			GetItemName(item->Item.Type, (item->Item.Level >> 3) & 0xF, Text);
			for (int i = 0; i < MAX_PICK_EXTRA; i++)
			{
				if (unicode::_strlen(m_Pick.Name[i]) > 0)
				{
					if (unicode::_strstr(Text, m_Pick.Name[i]))
					{
						percent = 100;
						break;
					}
				}

			}
		}
	}
	if (percent > 0)
	{
		if (-1 == g_pMyInventory->FindEmptySlot(&item->Item) && ITEM_POTION + 15 != item->Item.Type)
		{
			percent = 0;
		}
	}
	return percent;
}

bool CAIController::PickupItem(ITEM_t* item)
{
	if (item != nullptr)
	{
		vec3_t vLength;
		SelectedItem = ((int)item - (int)&Items[0]) / sizeof(ITEM_t);
		sCharacter->MovementType = MOVEMENT_GET;

		int tx = (int)(Items[SelectedItem].Object.Position[0] / TERRAIN_SCALE);
		int ty = (int)(Items[SelectedItem].Object.Position[1] / TERRAIN_SCALE);

		vLength[0] = item->Object.Position[0] - sCharacter->Object.Position[0];
		vLength[1] = item->Object.Position[1] - sCharacter->Object.Position[1];
		vLength[2] = item->Object.Position[2] - sCharacter->Object.Position[2];

		if (VectorLength(vLength) > 200.0)
		{
			if (PathFinding2(sCharacter->PositionX, sCharacter->PositionY, tx, ty, &sCharacter->Path, 0.0, 2))
			{
				SendMove(sCharacter, &item->Object);
			}
		}
		else
		{
			ItemKey = SelectedItem;
			SendRequestGetItem(ItemKey);
			prev_item = item;
		}
		return true;
	}
	return false;
}

void CAIController::RepairItem()
{
	if (CharacterAttribute->Level >= 50)
	{
		for (int i = 0; i < MAX_EQUIPMENT; i++)
		{
			ITEM* pEquipmentItemSlot = &CharacterMachine->Equipment[i];
			if (false == IsRepairBan(pEquipmentItemSlot) && pEquipmentItemSlot->Type >= 0)
			{
				int level = (pEquipmentItemSlot->Level >> 3) & 0xF;

				unsigned short Durability = calcMaxDurability(pEquipmentItemSlot, GMItemMng->find(pEquipmentItemSlot->Type), level);

				if (pEquipmentItemSlot->Durability < (7 * Durability / 10))
				{
					SendRequestRepair(i, 1);
					return;
				}
			}
		}
	}
}

int CAIController::GetSkillIndex(int SkillKey)
{
	if (SkillKey)
	{
		for (int i = 0; i < CharacterAttribute->SkillNumber; ++i)
		{
			if (CharacterAttribute->Skill[i] == SkillKey)
				return i;
		}
	}
	return -1;
}

bool CAIController::SkillAt(int SkillKey, CHARACTER* Target, bool bActive)
{
	bool success = false;

	if (Target && (!Target || bActive != true || Target->Object.Kind == KIND_MONSTER))
	{
		int index = GetSkillIndex(SkillKey);

		if (index == -1)
		{
			success = false;
		}
		else
		{
			int piMana, piDistance, piSkillMana;
			gSkillManager.GetSkillInformation(SkillKey, 1, NULL, &piMana, &piDistance, &piSkillMana);

			if ((DWORD)piMana <= CharacterAttribute->Mana
				|| (g_pMyInventory->FindManaItemIndex() != -1))
			{
				if ((DWORD)piSkillMana <= CharacterAttribute->SkillMana)
				{
					if (!m_Combo)
					{
						if (GetDistance(Target) > (float)piDistance)
						{
							if (PathFinding2(sCharacter->PositionX, sCharacter->PositionY, Target->PositionX, Target->PositionY, &sCharacter->Path, (float)piDistance, 2))
							{
								sCharacter->Movement = 1;
								SendMove(sCharacter, &sCharacter->Object);
							}
						}
					}
					sCharacter->CurrentSkill = index;
					SelectedCharacter = gmCharacters->GetCharacterIndex(Target);
					MouseRButtonPush = true;
					m_CurTimeATK1 = timeGetTime();
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
		}
	}
	else
	{
		sCharacter->CurrentSkill = 0;
		SelectedCharacter = -1;
		success = false;
	}
	return success;
}

bool CAIController::UseConditionalSkill(MUSkill_info* Skill, unsigned __int32* cTime)
{
	if (!Skill->ID)
		return false;

	if (Skill->Delay)
	{
		if ((timeGetTime() - *cTime) > (unsigned int)CLOCKS_PER_SEC * Skill->TimeDelay && ChooseTarget() && SkillAt(Skill->ID, sMonsterATK, true))
		{
			*cTime = timeGetTime();
			return true;
		}
		return false;
	}
	if (!Skill->Counter)
		return false;

	bool Success = false;
	int EnemyNum = 0;
	int CounterPre = Skill->CounterPre;

	if (CounterPre)
	{
		if (CounterPre == 1)
			EnemyNum = ChooseTarget();
	}
	else
	{
		EnemyNum = ChooseTarget();
	}
	switch (Skill->CounterSub)
	{
	case 0:
		if (EnemyNum < 2)
			return false;
		SkillAt(Skill->ID, sMonsterATK, true);
		Success = 1;
		break;
	case 1:
		if (EnemyNum < 3)
			return false;
		SkillAt(Skill->ID, sMonsterATK, true);
		Success = 1;
		break;
	case 2:
		if (EnemyNum < 4)
			return false;
		SkillAt(Skill->ID, sMonsterATK, true);
		Success = 1;
		break;
	case 3:
		if (EnemyNum < 5)
			return false;
		SkillAt(Skill->ID, sMonsterATK, true);
		Success = 1;
		break;
	default:
		return Success;
	}
	return Success;
}

bool CAIController::IsBuffActive(CHARACTER* pCha, int SkillIndex)
{
	if (SkillIndex > 0 && SkillIndex < MAX_SKILLS)
	{
		eBuffState BuffIndex = eBuffState((&SkillAttribute[SkillIndex])->Effect);

		if (pCha && BuffIndex != eBuffNone)
		{
			return g_isCharacterBuff((&pCha->Object), BuffIndex);
		}
	}

	return false;
}

bool CAIController::SelfBuffAt()
{
	__time32_t GetTime = timeGetTime();

	if ((GetTime - m_WaitTimeBuffAT) >= 500)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (m_SkillBuff[i] > 0 
				&& !IsBuffActive(sCharacter, m_SkillBuff[i]) 
				&& SkillAt(m_SkillBuff[i], sCharacter, false))
			{
				m_WaitTimeBuffAT = GetTime;
				m_WaitTime = 600;
				return true;
			}
		}
	}
	return false;
}

bool CAIController::HealAt(CHARACTER* Target)
{
	bool success = false;
	time_t time_current = timeGetTime();

	if ((time_current - m_WaitTimeHealAT) >= 500)
	{
		if (GetSkillIndex(AT_SKILL_HEALING) >= 0 && SkillAt(AT_SKILL_HEALING, Target, false))
			success = true;
		else
			success = GetSkillIndex(413) >= 0 && SkillAt(413, Target, false);

		m_WaitTimeHealAT = time_current;
	}
	else
	{
		success = false;
	}
	return success;
}

bool CAIController::PartyHeal()
{
	time_t time_current = timeGetTime();

	if ((time_current - m_TimeBuffParty) >= 500)
	{
		for (int i = 0; i < PartyNumber; ++i)
		{
			CHARACTER* pCha = gmCharacters->GetCharacter(Party[i].index);

			if (pCha && pCha != Hero)
			{
				if (Party[i].currHP < (m_PartyHeal.Percent / 10) && pCha->Object.Live == true && HealAt(pCha))
				{
					m_TimeBuffParty = time_current;
					return true;
				}
			}
		}
	}
	return false;
}

bool CAIController::PartyBuff()
{
	time_t time_current = timeGetTime();

	if ((time_current - m_TimeBuffParty) >= 500)
	{
		for (int i = 0; i < PartyNumber; ++i)
		{
			CHARACTER* pCha = gmCharacters->GetCharacter(Party[i].index);

			if (pCha && pCha != Hero)
			{
				for (int j = 0; j < 3; ++j)
				{
					if (m_PartySkillBuff[j] > 0 && (time_current - m_DelayMemberParty[j][i] >= (1000 * m_PartyBuffSeconds)
						|| !IsBuffActive(pCha, m_PartySkillBuff[j])) && pCha->Object.Live == true && SkillAt(m_PartySkillBuff[j], pCha, false))
					{
						m_DelayMemberParty[j][i] = time_current;
						m_TimeBuffParty = time_current;
						return true;
					}
				}
			}
		}
	}
	return false;
}

void CAIController::MoveTo(int x, int y)
{
	if (!PlayerReached(x, y))
	{
		if (PathFinding2(sCharacter->PositionX, sCharacter->PositionY, x, y, &sCharacter->Path, 0.0, 2))
		{
			sCharacter->Movement = 1;
			sCharacter->MovementType = 0;
			SendMove(sCharacter, &sCharacter->Object);
		}
	}
}

bool CAIController::PlayerReached(int x, int y)
{
	return (sCharacter->PositionX - x) * (sCharacter->PositionX - x) <= 2
		&& (sCharacter->PositionY - y) * (sCharacter->PositionY - y) <= 2;
}

bool CAIController::RecievePartyRequest(int a2)
{
	return false;
}

bool CAIController::CanUsingEliteManaPotion()
{
	if (m_IsStart)
		return m_EliteManaPotion;
	else
		return false;
}

void CAIController::SetPartyBuff()
{
	for (int i = 0; i < 3; ++i)
	{
		m_PartySkillBuff[i] = 0;
		if (m_SkillBuff[i] > 0 && m_SkillBuff[i] < MAX_SKILLS)
		{
			m_PartySkillBuff[i] = m_SkillBuff[i];
		}
	}
}

bool CAIController::Pack(BYTE* BufferUnpack)
{
	memset(BufferUnpack, 0, 256);

	*(WORD*)BufferUnpack = *(WORD*)BufferUnpack & 0xFFF8 | 1;
	*(WORD*)BufferUnpack = ((m_ATTKDistance & 0xF) << 8) | *(WORD*)BufferUnpack & 0xF0FF;
	*(WORD*)(BufferUnpack + 24) = m_Potion.Enable & 1 | *(WORD*)(BufferUnpack + 24) & 0xFFFE;
	*(WORD*)(BufferUnpack + 22) = m_Potion.Percent / 10 & 0xF | *(WORD*)(BufferUnpack + 22) & 0xFFF0;
	*(WORD*)(BufferUnpack + 24) = 2 * (m_HealLife.Enable & 1) | *(WORD*)(BufferUnpack + 24) & 0xFFFD;
	*(WORD*)(BufferUnpack + 22) = 16 * (m_HealLife.Percent / 10 & 0xF) | *(WORD*)(BufferUnpack + 22) & 0xFF0F;
	*(WORD*)(BufferUnpack + 24) = 4 * (m_DrainLife.Enable & 1) | *(WORD*)(BufferUnpack + 24) & 0xFFFB;
	*(WORD*)(BufferUnpack + 22) = ((m_DrainLife.Percent / 10 & 0xF) << 8) | *(WORD*)(BufferUnpack + 22) & 0xF0FF;
	*(WORD*)(BufferUnpack + 24) = 8 * (m_DistanceLong & 1) | *(WORD*)(BufferUnpack + 24) & 0xFFF7;
	*(WORD*)(BufferUnpack + 24) = 16 * (m_DistanceReturn & 1) | *(WORD*)(BufferUnpack + 24) & 0xFFEF;
	*(WORD*)(BufferUnpack + 2) = m_DistanceOnSec;
	*(WORD*)(BufferUnpack + 4) = m_SkillBasic;
	*(WORD*)(BufferUnpack + 6) = m_SkillSecond[0].ID;
	*(WORD*)(BufferUnpack + 24) = ((m_SkillSecond[0].Delay & 1) << 11) | *(WORD*)(BufferUnpack + 24) & 0xF7FF;
	*(WORD*)(BufferUnpack + 8) = m_SkillSecond[0].TimeDelay;
	*(WORD*)(BufferUnpack + 24) = ((m_SkillSecond[0].Counter & 1) << 12) | *(WORD*)(BufferUnpack + 24) & 0xEFFF;
	*(WORD*)(BufferUnpack + 24) = ((m_SkillSecond[0].CounterPre & 1) << 13) | *(WORD*)(BufferUnpack + 24) & 0xDFFF;
	*(WORD*)(BufferUnpack + 24) = ((m_SkillSecond[0].CounterSub & 3) << 14) | *(WORD*)(BufferUnpack + 24) & 0x3FFF;
	*(WORD*)(BufferUnpack + 10) = m_SkillSecond[1].ID;
	*(WORD*)(BufferUnpack + 26) = m_SkillSecond[1].Delay & 1 | *(WORD*)(BufferUnpack + 26) & 0xFFFE;
	*(WORD*)(BufferUnpack + 12) = m_SkillSecond[1].TimeDelay;
	*(WORD*)(BufferUnpack + 26) = 2 * (m_SkillSecond[1].Counter & 1) | *(WORD*)(BufferUnpack + 26) & 0xFFFD;
	*(WORD*)(BufferUnpack + 26) = 4 * (m_SkillSecond[1].CounterPre & 1) | *(WORD*)(BufferUnpack + 26) & 0xFFFB;
	*(WORD*)(BufferUnpack + 26) = 8 * (m_SkillSecond[1].CounterSub & 3) | *(WORD*)(BufferUnpack + 26) & 0xFFE7;
	*(WORD*)(BufferUnpack + 24) = 32 * (m_Combo & 1) | *(WORD*)(BufferUnpack + 24) & 0xFFDF;
	*(WORD*)(BufferUnpack + 24) = ((m_PartyMode & 1) << 6) | *(WORD*)(BufferUnpack + 24) & 0xFFBF;
	*(WORD*)(BufferUnpack + 24) = ((m_PartyHeal.Enable & 1) << 7) | *(WORD*)(BufferUnpack + 24) & 0xFF7F;
	*(WORD*)(BufferUnpack + 22) = ((m_PartyHeal.Percent / 10 & 0xF) << 12) | *(WORD*)(BufferUnpack + 22) & 0xFFF;
	*(WORD*)(BufferUnpack + 24) = ((m_IsPartyBuff & 1) << 8) | *(WORD*)(BufferUnpack + 24) & 0xFEFF;
	*(WORD*)(BufferUnpack + 14) = m_PartyBuffSeconds;
	*(WORD*)(BufferUnpack + 24) = ((m_DarkSpirit & 1) << 9) | *(WORD*)(BufferUnpack + 24) & 0xFDFF;
	*(WORD*)(BufferUnpack + 26) = ((m_DarkSpiritType & 3) << 8) | *(WORD*)(BufferUnpack + 26) & 0xFCFF;
	*(WORD*)(BufferUnpack + 24) = ((m_BuffDuration & 1) << 10) | *(WORD*)(BufferUnpack + 24) & 0xFBFF;
	*(WORD*)(BufferUnpack + 16) = m_SkillBuff[0];
	*(WORD*)(BufferUnpack + 18) = m_SkillBuff[1];
	*(WORD*)(BufferUnpack + 20) = m_SkillBuff[2];
	*(WORD*)BufferUnpack = ((m_PickDistance & 0xF) << 12) | *(WORD*)BufferUnpack & 0xFFF;
	*(WORD*)(BufferUnpack + 26) = 32 * (m_RepairFix & 1) | *(WORD*)(BufferUnpack + 26) & 0xFFDF;
	*(WORD*)(BufferUnpack + 26) = ((m_Pick.Near & 1) << 6) | *(WORD*)(BufferUnpack + 26) & 0xFFBF;
	*(WORD*)(BufferUnpack + 26) = ((m_Pick.Select & 1) << 7) | *(WORD*)(BufferUnpack + 26) & 0xFF7F;
	*(WORD*)BufferUnpack = 8 * (m_Pick.Jewel & 1) | *(WORD*)BufferUnpack & 0xFFF7;
	*(WORD*)BufferUnpack = 16 * (m_Pick.Ancient & 1) | *(WORD*)BufferUnpack & 0xFFEF;
	*(WORD*)BufferUnpack = 32 * (m_Pick.Excellent & 1) | *(WORD*)BufferUnpack & 0xFFDF;
	*(WORD*)BufferUnpack = ((m_Pick.Zen & 1) << 6) | *(WORD*)BufferUnpack & 0xFFBF;
	*(WORD*)BufferUnpack = ((m_Pick.Extra & 1) << 7) | *(WORD*)BufferUnpack & 0xFF7F;
	*(WORD*)(BufferUnpack + 26) = ((m_AutoFriend & 1) << 10) | *(WORD*)(BufferUnpack + 26) & 0xFBFF;
	*(WORD*)(BufferUnpack + 26) = ((m_AutoGuildMember & 1) << 11) | *(WORD*)(BufferUnpack + 26) & 0xF7FF;
	*(WORD*)(BufferUnpack + 26) = ((m_EliteManaPotion & 1) << 12) | *(WORD*)(BufferUnpack + 26) & 0xEFFF;

	for (int i = 0; i < MAX_PICK_EXTRA; ++i)
	{
		memcpy(BufferUnpack + 16 * i + 64, m_Pick.Name[i], 0x10u);
	}
	return true;
}

bool CAIController::Unpack(BYTE* BufferPack)
{
	if (*BufferPack == 0xFF)
	{
		LoadDefaultConfg();
	}
	else
	{
		m_ATTKDistance = (*(WORD*)BufferPack >> 8) & 0xF;
		m_Potion.Enable = *((WORD*)BufferPack + 12) & 1;
		m_Potion.Percent = 10 * (*((WORD*)BufferPack + 11) & 0xF);
		m_HealLife.Enable = (*((WORD*)BufferPack + 12) >> 1) & 1;
		m_HealLife.Percent = 10 * ((*((WORD*)BufferPack + 11) >> 4) & 0xF);
		m_DrainLife.Enable = (*((WORD*)BufferPack + 12) >> 2) & 1;
		m_DrainLife.Percent = 10 * ((*((WORD*)BufferPack + 11) >> 8) & 0xF);
		m_DistanceLong = (*((WORD*)BufferPack + 12) >> 3) & 1;
		m_DistanceReturn = (*((WORD*)BufferPack + 12) >> 4) & 1;
		m_DistanceOnSec = *((WORD*)BufferPack + 1);
		m_SkillBasic = *((WORD*)BufferPack + 2);
		m_SkillSecond[0].ID = *((WORD*)BufferPack + 3);
		m_SkillSecond[0].Delay = (*((WORD*)BufferPack + 12) >> 11) & 1;
		m_SkillSecond[0].TimeDelay = *((WORD*)BufferPack + 4);
		m_SkillSecond[0].Counter = (*((WORD*)BufferPack + 12) >> 12) & 1;
		m_SkillSecond[0].CounterPre = ((*((WORD*)BufferPack + 12) >> 13) & 1) % 2;
		m_SkillSecond[0].CounterSub = ((unsigned __int8)(*((WORD*)BufferPack + 12) >> 8) >> 6) % 4;
		m_SkillSecond[1].ID = *((WORD*)BufferPack + 5);
		m_SkillSecond[1].Delay = *((WORD*)BufferPack + 13) & 1;
		m_SkillSecond[1].TimeDelay = *((WORD*)BufferPack + 6);
		m_SkillSecond[1].Counter = (*((WORD*)BufferPack + 13) >> 1) & 1;
		m_SkillSecond[1].CounterPre = ((*((WORD*)BufferPack + 13) >> 2) & 1) % 2;
		m_SkillSecond[1].CounterSub = ((*((WORD*)BufferPack + 13) >> 3) & 3) % 4;
		m_Combo = (*((WORD*)BufferPack + 12) >> 5) & 1;
		m_PartyMode = (*((WORD*)BufferPack + 12) >> 6) & 1;
		m_PartyHeal.Enable = (*((WORD*)BufferPack + 12) >> 7) & 1;
		m_PartyHeal.Percent = 10 * ((*((WORD*)BufferPack + 11) >> 12) & 0xF);

		m_IsPartyBuff = (*((WORD*)BufferPack + 12) >> 8) & 1;
		m_PartyBuffSeconds = *((WORD*)BufferPack + 7);
		m_DarkSpirit = (*((WORD*)BufferPack + 12) >> 9) & 1;
		m_DarkSpiritType = ((*((WORD*)BufferPack + 13) >> 8) & 3) % 4;
		m_BuffDuration = (*((WORD*)BufferPack + 12) >> 10) & 1;
		m_SkillBuff[0] = *((WORD*)BufferPack + 8);
		m_SkillBuff[1] = *((WORD*)BufferPack + 9);
		m_SkillBuff[2] = *((WORD*)BufferPack + 10);
		m_PickDistance = (unsigned __int8)(*(WORD*)BufferPack >> 8) >> 4;

		m_RepairFix = (*((WORD*)BufferPack + 13) >> 5) & 1;
		m_Pick.Near = (*((WORD*)BufferPack + 13) >> 6) & 1;
		m_Pick.Select = (*((WORD*)BufferPack + 13) >> 7) & 1;
		m_Pick.Jewel = (*(WORD*)BufferPack >> 3) & 1;
		m_Pick.Ancient = (*(WORD*)BufferPack >> 4) & 1;
		m_Pick.Excellent = (*(WORD*)BufferPack >> 5) & 1;
		m_Pick.Zen = (*(WORD*)BufferPack >> 6) & 1;
		m_Pick.Extra = (*(WORD*)BufferPack >> 7) & 1;
		m_AutoFriend = (*((WORD*)BufferPack + 13) >> 10) & 1;
		m_AutoGuildMember = (*((WORD*)BufferPack + 13) >> 11) & 1;
		m_EliteManaPotion = (*((WORD*)BufferPack + 13) >> 12) & 1;

		for (int i = 0; i < MAX_PICK_EXTRA; ++i)
		{
			memset(m_Pick.Name[i], 0, 0x11u);
			memcpy(m_Pick.Name[i], &BufferPack[16 * i + 64], 0x10u);
		}

		if (m_ATTKDistance < 1 || m_ATTKDistance > 8)
			m_ATTKDistance = 8;
		if (m_PickDistance < 1 || m_PickDistance > 8)
			m_PickDistance = 8;

		RANGE_FIXED(&m_Potion, 0, 100, 50);
		RANGE_FIXED(&m_HealLife, 0, 100, 50);
		RANGE_FIXED(&m_DrainLife, 0, 100, 50);
		RANGE_FIXED(&m_PartyHeal, 0, 100, 50);
	}
	return true;
}
