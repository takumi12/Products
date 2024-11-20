#pragma once
#define TW_SAFEZONE								( 0x0001)
#define TW_CHARACTER							( 0x0002)
#define TW_NOMOVE								( 0x0004)
#define TW_NOGROUND								( 0x0008)
#define TW_WATER								( 0x0010)
#define TW_ACTION								( 0x0020)
#define TW_HEIGHT								( 0x0040)
#define TW_CAMERA_UP							( 0x0080)
#define TW_NOATTACKZONE							( 0x0100)
#define TW_ATT1									( 0x0200)
#define TW_ATT2									( 0x0400)
#define TW_ATT3									( 0x0800)
#define TW_ATT4									( 0x1000)
#define TW_ATT5									( 0x2000)
#define TW_ATT6									( 0x4000)
#define TW_ATT7									( 0x8000)
#define TERRAIN_SCALE							100.f
#define TERRAIN_SIZE							256
#define TERRAIN_SIZE_MASK						255


typedef struct
{
	short Index;
	float Position[3];
	float Angle[3];
	float Scale;
	bool LightEnable;
	bool EnableFullLight;
	bool EnablePrimaryLight;
	float Light[3];
} OBJECT_OBB;

typedef std::vector<OBJECT_OBB> type_vect_obj;

class CGMFilesOBB
{
public:
	CGMFilesOBB();
	~CGMFilesOBB();
	static CGMFilesOBB* Instance();

private:
	int currentMap;
	int g_iTotalObj;
	BYTE currentVersion;
	type_vect_obj obj_bb;
public:
	void OpenFileObject();
	void ExportFileObject();
	void CreateTableObj(const char* filename);

	void RenderTableObjectBB();
	void RenderInformationOBB();


	void OpenFileMap();
	void ReadDataMap(const char* filename);
	void WriteDataMap(const char* filename, unsigned char* EncData, int EncBytes);


	void OpenFileATT();
	void ReadDataATT(const char* filename);
	void WriteDataATT(const char* filename, unsigned char* EncData, int EncBytes);


	void OpenTerrainHeight();
	void ReadDataOZB(const char* filename);
	void WriteDataOZB(const char* filename, int EncBytes);
};


#define GMFilesMap					(CGMFilesOBB::Instance())

extern bool WIN_OBJECT;
extern void RenderInterfaceMaps();