#include "stdafx.h"
#include "CGMProtect.h"
#include "CGMMonsterMng.h"
#include "CGMItemEffect.h"
#include "CGMItemWings.h"
#include "w_PetProcess.h"

extern BYTE Version[SIZE_PROTOCOLVERSION];
extern BYTE Serial[SIZE_PROTOCOLSERIAL + 1];

CGMProtect::CGMProtect()
{
	memset(&m_MainInfo, 0, sizeof(m_MainInfo));
}

CGMProtect::~CGMProtect()
{
}

CGMProtect* CGMProtect::Instance()
{
	static CGMProtect sInstance;
	return &sInstance;
}

WORD CGMProtect::sin_port()
{
	return m_MainInfo.IpAddressPort;
}

char* CGMProtect::GetWinName()
{
	return m_MainInfo.WindowName;
}

char* CGMProtect::GetScreenPath()
{
	return m_MainInfo.ScreenShotPath;
}

void CGMProtect::GenerateKey()
{
	WORD Key = 0;

	for (int n = 0; n < sizeof(m_MainInfo.CustomerName); n++)
	{
		Key += (BYTE)(m_MainInfo.CustomerName[n] ^ m_MainInfo.ClientSerial[(n % sizeof(m_MainInfo.ClientSerial))]);
	}

	this->EncDecKey[0] = (BYTE)0xE2;
	this->EncDecKey[1] = (BYTE)0x02;

	this->EncDecKey[0] += LOBYTE(Key);
	this->EncDecKey[1] += HIBYTE(Key);
}

void CGMProtect::HookComplete()
{
	szServerIpAddress = new char[32];

	memset(szServerIpAddress, 0, sizeof(char) * 32);
	memcpy(szServerIpAddress, m_MainInfo.IpAddress, sizeof(char) * 32);

	g_ServerPort = m_MainInfo.IpAddressPort;

	Version[0] = (m_MainInfo.ClientVersion[0] + 1);
	Version[1] = (m_MainInfo.ClientVersion[2] + 2);
	Version[2] = (m_MainInfo.ClientVersion[3] + 3);
	Version[3] = (m_MainInfo.ClientVersion[5] + 4);
	Version[4] = (m_MainInfo.ClientVersion[6] + 5);
	
	memcpy(Serial, m_MainInfo.ClientSerial, sizeof(Serial));
}

bool CGMProtect::ReadMainFile(std::string filename)
{
	BYTE* Buffer = NULL;
	int Size = sizeof(MAIN_FILE_INFO);

	int MaxLine = PackFileDecrypt(filename, &Buffer, 1, Size, 0x5A18);

	if (MaxLine == 0 || Buffer == NULL)
		return false;

	memcpy(&m_MainInfo, Buffer, Size);

	this->GenerateKey();

	SAFE_DELETE_ARRAY(Buffer);

	return true;
}

bool CGMProtect::IsInvExt()
{
	return m_MainInfo.InventoryExtension != 0;
}

bool CGMProtect::IsBaulExt()
{
	return m_MainInfo.WareHouseExtension != 0;
}

bool CGMProtect::IsMuHelper()
{
	return m_MainInfo.MuHelperOficial != 0;
}

bool CGMProtect::IsInGameShop()
{
	return m_MainInfo.CashShopInGame != 0;
}

bool CGMProtect::IsMasterSkill()
{
	return m_MainInfo.WinMasterSkill != 0;
}

bool CGMProtect::ChatEmoticon()
{
	return m_MainInfo.ChatEmoticon;
}

bool CGMProtect::CastleSkillEnable()
{
	return m_MainInfo.CastleSkillEnable;
}

int CGMProtect::CharInfoBalloonType()
{
	return m_MainInfo.CharInfoBalloonType;
}

int CGMProtect::LookAndFeelType()
{
	return m_MainInfo.LookAndFeel;
}

int CGMProtect::GetAttackTimeForClass(BYTE Class)
{
	int AttackSpeedForClass = 15;

	if (m_MainInfo.MaxAttackSpeed[Class] > 0xFFFF)
	{
		switch (Class)
		{
		case Dark_Wizard:
			AttackSpeedForClass = 0x02;
			break;
		case Dark_Knight:
			AttackSpeedForClass = 0x0F;
			break;
		case Fairy_Elf:
			AttackSpeedForClass = 0x02;
			break;
		case Magic_Gladiator:
			AttackSpeedForClass = 0x02;
			break;
		case Dark_Lord:
			AttackSpeedForClass = 0x02;
			break;
		case Summoner:
			AttackSpeedForClass = 0x02;
			break;
		case Rage_Fighter:
			AttackSpeedForClass = 0x0F;
			break;
		}
	}

	return AttackSpeedForClass;
}

DWORD CGMProtect::GetAttackSpeedForClass(BYTE Class)
{
	return m_MainInfo.MaxAttackSpeed[Class];
}

void CGMProtect::LoadItemModel()
{
	for (int i = 0; i < MAX_CUSTOM_ITEM; i++)
	{
		CUSTOM_ITEM_INFO* item_info = &m_MainInfo.CustomItem[i];

		if (item_info->isUsable() == false)
			return;

		item_info->OpenLoad();
	}
}

void CGMProtect::LoadWingModel()
{
	for (int i = 0; i < MAX_CUSTOM_WINGS; i++)
	{
		CUSTOM_WING_INFO* item_info = &m_MainInfo.CustomWings[i];

		if (item_info->isUsable() == false)
			return;

		item_info->OpenLoad();
	}
}

#include "GOBoid.h"

void CGMProtect::LoadPetsModel()
{
	for (int i = 0; i < MAX_CUSTOM_PET; i++)
	{
		CUSTOM_PET_STACK* fw = &m_MainInfo.CustomPetItem[i];

		if (fw->isUsable() == false)
			return;

		fw->OpenLoad();

		if (fw->MovementType == 0)
		{
			int action[] = { PC4_SATAN, 0 };
			float speed[] = {0.5, 0};
			ThePetProcess().REGCustomPet(fw->ItemIndex, fw->RenderScale, 1, action, speed);
		}
		else if (fw->MovementType == 1)
		{
			int action[] = { XMAS_RUDOLPH, 0 };
			float speed[] = { 0.5, 0 };
			ThePetProcess().REGCustomPet(fw->ItemIndex, fw->RenderScale, 1, action, speed);
		}
		else if (fw->MovementType == 2)
		{
			int action[] = { PANDA, 0 };
			float speed[] = { 0.5, 0 };
			ThePetProcess().REGCustomPet(fw->ItemIndex, fw->RenderScale, 1, action, speed);
		}
		else if (fw->MovementType == 3)
		{
			int action[] = { UNICORN, 0 };
			float speed[] = { 0.5, 0 };
			ThePetProcess().REGCustomPet(fw->ItemIndex, fw->RenderScale, 1, action, speed);
		}
		else if (fw->MovementType == 4)
		{
			int action[] = { SKELETON, 0 };
			float speed[] = { 0.5, 0 };
			ThePetProcess().REGCustomPet(fw->ItemIndex, fw->RenderScale, 1, action, speed);
		}
		else if (fw->MovementType == 8)
		{
			int action[] = { ON_PETMUUN, 0 };
			float speed[] = { 0.5, 0 };
			ThePetProcess().REGCustomPet(fw->ItemIndex, fw->RenderScale, 1, action, speed);
		}
		else if (fw->MovementType == 5 || fw->MovementType == 6 || fw->MovementType == 7 || fw->MovementType == 11)
		{
			GoBoidREG(fw->ItemIndex, fw->ItemIndex + MODEL_ITEM, fw->MovementType, fw->RenderScale);
		}
	}
}

void CGMProtect::LoadClawModel(int itemindex, int model_L, int model_R)
{
	for (int i = 0; i < MAX_CUSTOM_ITEM; i++)
	{
		CUSTOM_ITEM_INFO* item_info = &m_MainInfo.CustomItem[i];

		if (item_info->isUsable() == false)
			return;

		if (item_info->Itemindex == itemindex)
		{
			item_info->OpenBattle(model_L, 1);

			item_info->OpenBattle(model_R, 0);

			return;
		}
	}
}

void CGMProtect::LoadImageTexture()
{
	for (int i = 0; i < MAX_CUSTOM_IMAGE; i++)
	{
		CUSTOM_IMAGE_INFO* item_info = &m_MainInfo.CustomImagen[i];

		if (item_info->isUsable() == false)
			return;

		item_info->OpenLoad();
	}
}

bool CGMProtect::GetItemColor(int itemindex, float* color)
{
	for (int i = 0; i < MAX_CUSTOM_ITEM; i++)
	{
		CUSTOM_ITEM_INFO* item_info = &m_MainInfo.CustomItem[i];

		if (item_info->isUsable() == false)
			return false;

		if (item_info->Itemindex == itemindex)
		{
			color[0] = item_info->Color[0];
			color[1] = item_info->Color[1];
			color[2] = item_info->Color[2];
			return true;
		}
	}
	return false;
}

bool CGMProtect::IsPetBug(int itemindex)
{
	if (itemindex >= 0 && itemindex < MAX_ITEM)
	{
		CUSTOM_PET_STACK* item_info;

		for (size_t i = 0; i < MAX_CUSTOM_PET; i++)
		{
			item_info = &m_MainInfo.CustomPetItem[i];

			if (item_info->isUsable() == false)
				break;

			if (item_info->ItemIndex == itemindex 
				&& (item_info->MovementType == 5
					|| item_info->MovementType == 6
					|| item_info->MovementType == 7
					|| item_info->MovementType == 11))
			{
				return true;
			}
		}
	}
	return false;
}

int CGMProtect::GetPetMovement(int itemindex)
{
	if (itemindex >= 0 && itemindex < MAX_ITEM)
	{
		for (size_t i = 0; i < MAX_CUSTOM_PET; i++)
		{
			CUSTOM_PET_STACK* item_info = &m_MainInfo.CustomPetItem[i];

			if (item_info->isUsable() == false)
				break;

			if (item_info->ItemIndex == itemindex)
			{
				return item_info->MovementType;
			}
		}
	}
	return 0;
}

CUSTOM_PET_STACK* CGMProtect::GetPetAction(int itemindex)
{
	if (itemindex >= 0 && itemindex < MAX_ITEM)
	{
		for (size_t i = 0; i < MAX_CUSTOM_PET; i++)
		{
			CUSTOM_PET_STACK* item_info = &m_MainInfo.CustomPetItem[i];

			if (item_info->isUsable() == false)
				break;

			if (item_info->ItemIndex == itemindex)
			{
				return item_info;
			}
		}
	}
	return NULL;
}

CUSTOM_ITEM_STACK* CGMProtect::GetItemStack(int itemindex)
{
	if (itemindex >= 0 && itemindex < MAX_ITEM)
	{
		for (size_t i = 0; i < MAX_CUSTOM_STACK; i++)
		{
			if (m_MainInfo.CustomStacks[i].isUsable() == false)
				break;

			if (m_MainInfo.CustomStacks[i].ItemIndex == itemindex)
			{
				return &m_MainInfo.CustomStacks[itemindex];
			}
		}
	}
	return NULL;
}

CUSTOM_SMOKE_STACK* CGMProtect::GetItemSmoke(int itemindex)
{
	if (itemindex >= 0 && itemindex < MAX_CUSTOM_SMOKE)
	{
		if (m_MainInfo.CustomSmokeItem[itemindex].isUsable())
		{
			return &m_MainInfo.CustomSmokeItem[itemindex];
		}
	}
	return NULL;
}

bool CGMProtect::GetNpcName(int IndexNpc, char* Name, int PositionX, int PositionY)
{
	for (size_t i = 0; i < MAX_CUSTOM_NPC_NAME; i++)
	{
		CUSTOM_NPC_NAME* Npc = &m_MainInfo.CustomNpcName[i];

		if (Npc->isUsable() == false)
			break;

		if (Npc->IndexNpc == IndexNpc 
			&& Npc->PositionX == PositionX
			&& Npc->PositionY == PositionY)
		{
			strcpy(Name, Npc->Name);
			return true;
		}
	}
	return false;
}

void CGMProtect::LoadingProgressive()
{
	GMItemWings->LoadDataWings(m_MainInfo.CustomWings);

	GMMonsterMng->LoadDataMonster(m_MainInfo.CustomMonster);

	GMItemEffect->LoadDataEffect(0, m_MainInfo.CustomEffectStatics);

	GMItemEffect->LoadDataEffect(1, m_MainInfo.CustomEffectDinamic);

	this->LoadItemModel();

	this->LoadPetsModel();

	this->LoadWingModel();

	this->LoadImageTexture();
}
