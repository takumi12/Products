#pragma once
#include "supportingfeature.h"
#define MIN_PORT 55901
#define MAX_PORT 55950
#define SIZE_PROTOCOLVERSION	( 5)
#define SIZE_PROTOCOLSERIAL		( 16)
#define PORT_RANGE(x)			(((x)<MIN_PORT)?0:((x)>MAX_PORT)?0:1)

typedef struct 
{
	char CustomerName[32];
	char IpAddress[32];
	WORD IpAddressPort;
	char ClientVersion[8];
	char ClientSerial[17];
	char WindowName[32];
	char ScreenShotPath[50];
	//------------------------------------
	UINT HelperActiveAlert;
	UINT HelperActiveLevel;
	//------------------------------------
	UINT MaxAttackSpeed[MAX_CLASS];
	UINT ReconnectTime;
	//------------------------------------
	UINT RemoveClass;
	//------------------------------------
	UINT ScreenType;
	UINT LookAndFeel;
	UINT ChatEmoticon;
	UINT CastleSkillEnable;
	UINT CharInfoBalloonType;
	UINT InventoryExtension;
	UINT WareHouseExtension;
	UINT CashShopInGame;
	UINT MuHelperOficial;
	UINT WinMasterSkill;
	//------------------------------------
	UINT SceneLogin;
	UINT SceneCharacter;

	UINT RenderMuLogo;
	UINT RenderGuildLogo;
	UINT RankUserSwitch;
	UINT RankUserShowOverHead;
	UINT RankUserOnlyOnSafeZone;
	UINT RankUserShowReset;
	UINT RankUserShowMasterReset;
	UINT RankUserNeedAltKey;

	UINT MenuSwitch;
	UINT MenuTypeButton;
	UINT MenuButtonVipShop;
	UINT MenuButtonRankTop;
	UINT MenuButtonCommand;
	UINT MenuButtonOptions;
	UINT MenuButtonEventTime;

	UINT WindowsVipShop;
	UINT WindowsRankTop;
	UINT WindowsCommand;
	UINT WindowsEventTime;
	//------------------------------------
	CUSTOM_MAPE_INFO CustomMap[MAX_CUSTOM_MAPE];
	CUSTOM_ITEM_INFO CustomItem[MAX_CUSTOM_ITEM];
	CUSTOM_IMAGE_INFO CustomImagen[MAX_CUSTOM_IMAGE];
	CUSTOM_WING_INFO CustomWings[MAX_CUSTOM_WINGS];
	CUSTOM_PET_STACK CustomPetItem[MAX_CUSTOM_PET];
	CUSTOM_ITEM_STACK CustomStacks[MAX_CUSTOM_STACK];
	CUSTOM_SMOKE_STACK CustomSmokeItem[MAX_CUSTOM_SMOKE];
	CUSTOM_MONSTER_INFO CustomMonster[MAX_CUSTOM_MONSTER];
	CUSTOM_NPC_NAME CustomNpcName[MAX_CUSTOM_NPC_NAME];
	CUSTOM_EFFECT_STACK CustomEffectDinamic[MAX_CUSTOM_EFFECT];
	CUSTOM_EFFECT_STACK CustomEffectStatics[MAX_CUSTOM_EFFECT];
}MAIN_FILE_INFO;

class CGMProtect
{
public:
	CGMProtect();
	virtual~CGMProtect();
	static CGMProtect* Instance();

	WORD sin_port();
	char* GetWinName();
	char* GetScreenPath();
	void GenerateKey();
	void HookComplete();
	bool ReadMainFile(std::string filename);

	bool IsInvExt();
	bool IsBaulExt();
	bool IsMuHelper();
	bool IsInGameShop();
	bool IsMasterSkill();

	bool ChatEmoticon();
	bool CastleSkillEnable();

	int CharInfoBalloonType();
	int LookAndFeelType();

	int GetAttackTimeForClass(BYTE Class);
	DWORD GetAttackSpeedForClass(BYTE Class);

	void LoadItemModel();
	void LoadWingModel();
	void LoadPetsModel();
	void LoadClawModel(int itemindex, int model_L, int model_R);
	void LoadImageTexture();
	bool GetItemColor(int itemindex, float* color);

	bool IsPetBug(int itemindex);
	int GetPetMovement(int itemindex);

	CUSTOM_PET_STACK* GetPetAction(int itemindex);
	CUSTOM_ITEM_STACK* GetItemStack(int itemindex);
	CUSTOM_SMOKE_STACK* GetItemSmoke(int itemindex);

	bool GetNpcName(int IndexNpc, char* Name, int PositionX, int PositionY);


	void LoadingProgressive();
public:
	BYTE EncDecKey[2];
	MAIN_FILE_INFO m_MainInfo;
};

#define GMProtect				(CGMProtect::Instance())
#define gmProtect				(&(CGMProtect::Instance()->m_MainInfo))
