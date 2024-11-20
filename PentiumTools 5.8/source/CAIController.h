#pragma once
#include "Singleton.h"

#define INVENTORY_HELPER_ACTIVE
#define MOUSE_WHELL_HELPER_ON

#define MAX_PICK_EXTRA				12

typedef struct
{
	bool Enable;
	BYTE Percent;
}MUAction_info;

typedef struct
{
	int ID;
	bool Delay;
	time_t TimeDelay;
	bool Counter;
	int CounterPre;
	int CounterSub;
}MUSkill_info;

typedef struct
{
	bool Near;
	bool Select;
	bool Jewel;
	bool Ancient;
	bool Excellent;
	bool Zen;
	bool Extra;
	unicode::t_char Name[MAX_PICK_EXTRA][28];
}MUPick_info;

class CAIController : public Singleton<CAIController>
{
private:
	CHARACTER* sCharacter;
	bool m_IsStart;
	CHARACTER* sMonsterATK;
	CHARACTER* sMonsterPre;
	ITEM_t* prev_item;
	ITEM_t* cur_item;
	char* NameCharacter;
	time_t m_TimeAction;
	time_t m_CurTimeATK1;
	time_t m_TimeCondition[2];
	time_t m_WaitTimeHealAT;
	time_t m_WaitTimeConsume;
	time_t m_WaitTimeBuffAT;
	time_t m_TimeBuffParty;
	time_t m_WaitTime;
	int m_StartPositionX;
	int m_StartPositionY;
	time_t m_WaitTimeReturn;
	int m_CounATK;
	bool m_IsAutoAttack;
	time_t m_DelayThird[3];
	time_t m_DelayMemberParty[3][MAX_PARTYS];
	bool m_SendDarkAttack;
	time_t m_DelayFindTarget;
	bool m_IsReturn;
public:
	int m_ATTKDistance;
	MUAction_info m_Potion;
	MUAction_info m_HealLife;
	MUAction_info m_DrainLife;
	bool m_DistanceLong;
	bool m_DistanceReturn;
	int m_DistanceOnSec;
	int m_SkillBasic;
	MUSkill_info m_SkillSecond[2];
	bool m_Combo;
	bool m_PartyMode;
	MUAction_info m_PartyHeal;
	bool m_IsPartyBuff;
	int m_PartyBuffSeconds;
	bool m_DarkSpirit;
	int m_DarkSpiritType;
	bool m_BuffDuration;
	int m_SkillBuff[3];
	int m_PartySkillBuff[3];
	int m_PickDistance;
	bool m_RepairFix;
	MUPick_info m_Pick;
	bool m_AutoFriend;
	bool m_AutoGuildMember;
	bool m_EliteManaPotion;
public:
	CAIController(CHARACTER* Character);
	virtual ~CAIController();
public:
	bool IsRunning();
	bool CanUseAIController();
	void Start();
	void Stop();
	void SetActiveCharacter(CHARACTER* pCha);
	void ResetConfig();
	void LoadDefaultConfg();
	void SaveConfig();
	float GetDistance(CHARACTER* pCha);
	float GetDistance(OBJECT* o);
	void WhatToDoNext();
	bool ConsumePotion();
	int ChooseTarget();
	int GetHostileEnemyNum();
	CHARACTER* RankEnemy(CHARACTER* Rank1, CHARACTER* Rank2);
	bool IsEnemyAttaced();
	bool IsEnemyAttacking(CHARACTER* Target);
	bool CheckPickup();
	int RateItem(ITEM_t* item);
	bool PickupItem(ITEM_t* item);
	void RepairItem();
	int GetSkillIndex(int SkillKey);
	bool SkillAt(int SkillKey, CHARACTER* Target, bool bActive);
	bool UseConditionalSkill(MUSkill_info* Skill, unsigned __int32* cTime);
	bool IsBuffActive(CHARACTER* pCha, int SkillIndex);
	bool SelfBuffAt();
	bool HealAt(CHARACTER* Target);
	bool PartyHeal();
	bool PartyBuff();
	void MoveTo(int x, int y);
	bool PlayerReached(int x, int y);
	bool RecievePartyRequest(int a2);
	bool CanUsingEliteManaPotion();
	void SetPartyBuff();
	bool Pack(BYTE* BufferUnpack);
	bool Unpack(BYTE* BufferPack);
};

#define gmAIController			(CAIController::GetSingletonPtr())