#include "framework.h"
#include "CGMJSon.h"
#include "CGMFileSystem.h"
#include "CGMLibrayMng.h"

std::vector<IMAGE_LIBRARY> loadedFunctions;

CGMLibrayMng::CGMLibrayMng()
{
}

CGMLibrayMng::~CGMLibrayMng()
{
}

bool CGMLibrayMng::DinamicLibrary(const char* Libraryname, int& index)
{
	HMODULE library = LoadLibrary(Libraryname);

	if (library != nullptr)
	{
		IMAGE_LIBRARY func;

		func.save_image = (SaveImageLibrary)(GetProcAddress(library, "SaveImage"));
		func.open_image = (OpenImageLibrary)(GetProcAddress(library, "OpenImage"));

		if (func.save_image != nullptr && func.open_image != nullptr)
		{
			loadedFunctions.push_back(func);

			index = loadedFunctions.size() - 1;
			return true;
		}
		FreeLibrary(library);
	}
	return false;
}

void CGMLibrayMng::Initialize()
{
	CGMJSon FileJson(LinkFileHome("Libraryes.json"));

	json data = FileJson.OpenTemplate();


	if (data.empty() || !data.is_array() || data.size() == 0)
	{
		return;
	}

	auto files = data[0]["Files"];
	auto libraries = data[0]["LIBRARY"];

	for (auto& libraryEntry : libraries.items())
	{
		std::string libraryName = libraryEntry.key();
		int libraryIdentifier = libraryEntry.value();

		int index = -1;
		if (DinamicLibrary(LinkFileHome(libraryName).c_str(), index))
		{
			// Cargar funciones para extensiones asociadas a la librería
			for (auto& fileEntry : files.items())
			{
				std::string fileExtension = fileEntry.key();
				int fileLibraryIdentifier = fileEntry.value();

				// Si la extensión está asociada a la librería actual, registrar la función
				if (fileLibraryIdentifier == libraryIdentifier && index != -1)
				{
					map_dinamic[fileExtension] = index;
				}
			}
		}
	}
}

bool CGMLibrayMng::IsFileValid(std::string FileId)
{
	type_func_map::iterator it = map_dinamic.find(FileId);

	return (it != map_dinamic.end());
}

bool CGMLibrayMng::SaveImagen(std::string FileId, std::string FileName, bool modulo)
{
	type_func_map::iterator it = map_dinamic.find(FileId);

	if (it != map_dinamic.end())
	{
		int fileLibraryIdentifier = it->second;

		SaveImageLibrary myFunction = loadedFunctions[fileLibraryIdentifier].save_image;

		return myFunction(FileName.c_str(), modulo);
	}
	return false;
}

bool CGMLibrayMng::OpenImagen(std::string FileId, std::string FileName, BITMAP_t* bitmap, GLuint uiFilter, GLuint uiWrapMode)
{
	type_func_map::iterator it = map_dinamic.find(FileId);

	if (it != map_dinamic.end())
	{
		int fileLibraryIdentifier = it->second;

		OpenImageLibrary myFunction = loadedFunctions[fileLibraryIdentifier].open_image;

		return myFunction(FileName.c_str(), bitmap, uiFilter, uiWrapMode);
	}
	return false;
}
