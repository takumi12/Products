#pragma once
#define MAX_CUSTOM_MAPE						150
#define MAX_CUSTOM_ITEM						2048
#define MAX_CUSTOM_IMAGE					512
#define MAX_CUSTOM_MONSTER					512
#define MAX_CUSTOM_SMOKE					512
#define MAX_CUSTOM_NPC_NAME					100
#define MAX_CUSTOM_WINGS					256
#define MAX_CUSTOM_STACK					256
#define MAX_CUSTOM_PET						256
#define MAX_CUSTOM_EFFECT					5000

class CUSTOM_ITEM_INFO
{
public:
	int Itemindex;
	float Color[3];
	char filename[32];
public:
	CUSTOM_ITEM_INFO(){
		release();
	};
	void release();
	bool isUsable();
	void OpenLoad();
	void OpenBattle(int modelId, bool Left);
};

class CUSTOM_MONSTER_INFO
{
public:
	int monsterIndex;
	int RenderIndex;
	int Kind;
	float fSize;
	char Name[32];
	char directory[64];
	char filename[32];
public:
	CUSTOM_MONSTER_INFO() {
		release();
	};
	void release();
	bool isUsable();
	void OpenLoad();
};

class CUSTOM_NPC_NAME
{
public:
	int IndexNpc;
	int PositionX;
	int PositionY;
	char Name[32];
	int WorldIndex;
public:
	CUSTOM_NPC_NAME() {
		release();
	};
	void release();
	bool isUsable();
};

class CUSTOM_MAPE_INFO
{
public:
	int mapIndex;
	int IndexText;
	int TerrainExt;
	char ImgName[32];
	char lpszMp3[32];
public:
	CUSTOM_MAPE_INFO() {
		release();
	};
	void release();
	bool isUsable();
};

class CUSTOM_IMAGE_INFO
{
public:
	int ImgIndex;
	char ImgName[32];
	int Wrap;
	int Type;
public:
	CUSTOM_IMAGE_INFO() {
		release();
	};
	void release();
	bool isUsable();
	void OpenLoad();
};

class CUSTOM_WING_INFO
{
public:
	int ItemIndex;
	int DefenseConstA;
	int IncDamageConstA;
	int IncDamageConstB;
	int DecDamageConstA;
	int DecDamageConstB;
	int OptionIndex1;
	int OptionValue1;
	int OptionIndex2;
	int OptionValue2;
	int OptionIndex3;
	int OptionValue3;
	int NewOptionIndex1;
	int NewOptionValue1;
	int NewOptionIndex2;
	int NewOptionValue2;
	int NewOptionIndex3;
	int NewOptionValue3;
	int NewOptionIndex4;
	int NewOptionValue4;
	int ModelLinkType;
	char filename[32];
public:
	CUSTOM_WING_INFO() {
		release();
	};
	void release();
	bool isUsable();
	void OpenLoad();
};

class CUSTOM_ITEM_STACK
{
public:
	int ItemIndex;
	int MaxStack;
	int ItemConvert;
public:
	CUSTOM_ITEM_STACK() {
		release();
	};
	void release();
	bool isUsable();
};

class CUSTOM_SMOKE_STACK
{
public:
	int ItemIndex;
	float Color[3];
public:
	CUSTOM_SMOKE_STACK() {
		release();
	};
	void release();
	bool isUsable();
};

class CUSTOM_PET_STACK
{
public:
	int ItemIndex;
	int RenderType;
	int MovementType;
	int RenderEffect;
	float RenderScale;
	char filename[32];
public:
	CUSTOM_PET_STACK() {
		release();
	};
	void release();
	bool isUsable();
	void OpenLoad();
};

class CUSTOM_EFFECT_STACK
{
public:
	int ItemIndex;
	int Texture;
	int JoinBy;
	int SubType;
	float Scale;
	float Angle;
	float Color[3];
public:
	CUSTOM_EFFECT_STACK() {
		release();
	};
	void release();
	bool isUsable();
};
