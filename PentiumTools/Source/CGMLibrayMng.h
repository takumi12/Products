#pragma once
class BITMAP_t;

typedef std::map<std::string, int> type_func_map;
typedef bool (*SaveImageLibrary)(const char*, bool);
typedef bool (*OpenImageLibrary)(const char*, BITMAP_t*, GLuint, GLuint);

typedef struct
{
	SaveImageLibrary save_image;
	OpenImageLibrary open_image;
} IMAGE_LIBRARY;

class CGMLibrayMng
{
public:
	CGMLibrayMng();
	~CGMLibrayMng();
private:
	type_func_map map_dinamic;
public:
	bool DinamicLibrary(const char* Libraryname, int& index);

	void Initialize();
	bool IsFileValid(std::string FileId);
	bool SaveImagen(std::string FileId, std::string FileName, bool modulo);
	bool OpenImagen(std::string FileId, std::string FileName, BITMAP_t* bitmap, GLuint uiFilter, GLuint uiWrapMode);

	static CGMLibrayMng* Instance()
	{
		static CGMLibrayMng tc; return &tc;
	}
};

#define GMLibrayMng             (CGMLibrayMng::Instance())