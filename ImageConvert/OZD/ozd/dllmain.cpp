// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"
#include "MuCrypto.h"
#include "CGMElement.h"

void CreateMessageBox(UINT uType, const char* title, char* message,...) // OK
{
	char buff[256];

	memset(buff,0,sizeof(buff));

	va_list arg;
	va_start(arg,message);
	vsprintf_s(buff,message,arg);
	va_end(arg);

	MessageBox(NULL, buff, title, uType);
}

bool IsFileEnc(std::string FileName, bool& Encrypt)
{
	size_t Success = -1;

	if ((Success = FileName.find(".DDS"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".dds"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".OZD"), Success != -1))
	{
		Encrypt = true;
	}
	else if ((Success = FileName.find(".ozd"), Success != -1))
	{
		Encrypt = true;
	}
	return (Success != -1);
}

bool Export(std::string FileName, std::string& FileExport, bool& Encrypt)
{
	size_t Success = -1;

	if ((Success = FileName.find(".DDS"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".OZD");
	}
	else if ((Success = FileName.find(".dds"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".ozd");
	}
	else if ((Success = FileName.find(".OZD"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".DDS");
	}
	else if ((Success = FileName.find(".ozd"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".dds");
	}
	return (Success != -1);
}

bool ImageEncrypt(const char* filename, const char* fileExport)
{
	FILE* fp; bool succes = false;
	if ((fp = fopen(filename, "rb"), fp != NULL))
	{
		fseek(fp, 0, SEEK_END);
		int FileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		std::vector<BYTE> fileBuffer(FileSize);
		fread(fileBuffer.data(), 1, FileSize, fp);
		fclose(fp);

		if (!gMuCrypto.ModulusEncrypt(fileBuffer))
		{
			return false;
		}

		if ((fp = fopen(fileExport, "wb"), fp != NULL))
		{
			fwrite(fileBuffer.data(), 1, fileBuffer.size(), fp);
			fclose(fp);
			succes = true;
			CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE,"Imagen convertida Correctamente");
		}
		else
		{
			CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"No se pudo crear el archivo en la ruta: %s", fileExport);
		}
	}
	else
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"Archivo no encontrado: %s", filename);
	}
	return succes;
}

bool ImageDecrypt(const char* filename, const char* fileExport)
{
	FILE* fp; bool succes = false;
	if ((fp = fopen(filename, "rb"), fp != NULL))
	{
		fseek(fp, 0, SEEK_END);
		int FileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		std::vector<BYTE> fileBuffer(FileSize);
		fread(fileBuffer.data(), 1, FileSize, fp);
		fclose(fp);

		if (!gMuCrypto.ModulusDecryptv2(fileBuffer))
		{
			return false;
		}

		if ((fp = fopen(fileExport, "wb"), fp != NULL))
		{
			fwrite(fileBuffer.data(), 1, fileBuffer.size(), fp);
			fclose(fp);
			succes = true;
			CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE,"Imagen convertida Correctamente");
		}
		else
		{
			CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"No se pudo crear el archivo en la ruta: %s", fileExport);
		}
	}
	else
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"Archivo no encontrado: %s", filename);
	}
	return succes;
}

extern "C" _declspec(dllexport) bool OpenImage(const char* Filename, BITMAP_t* bitmap , GLuint uiFilter, GLuint uiWrapMode)
{
	bool Encrypt = false;

	if(!IsFileEnc(Filename, Encrypt))
	{
		return false;
	}

	FILE* fp = fopen(Filename, "rb");
	if (fp == NULL)
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	int Size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	std::vector<BYTE> fileBuffer(Size);
	fread(fileBuffer.data(), 1, Size, fp);
	fclose(fp);

	if (Encrypt)
	{
		if (!gMuCrypto.ModulusDecryptv2(fileBuffer))
		{
			return false;
		}
	}

	int Width = 0;
	int Height = 0;
	int channels = 0;

	BYTE* image = SOIL_load_image_from_memory(fileBuffer.data(), fileBuffer.size(), &Width, &Height, &channels, SOIL_LOAD_RGBA);

	if (!image)
	{
		return false;
	}

	bitmap->output_width = (float)Width;

	bitmap->output_height = (float)Height;

	bitmap->Components = channels;

	glGenTextures(1, &(bitmap->uiBitmapIndex));
	glBindTexture(GL_TEXTURE_2D, bitmap->uiBitmapIndex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, uiFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, uiFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, uiWrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, uiWrapMode);

	SOIL_free_image_data(image);
	return true;
}

extern "C" _declspec(dllexport) bool SaveImage(const char* Filename, bool Encrypt)
{
	std::string FileExport;

	if(Export(Filename, FileExport, Encrypt))
	{
		if(Encrypt)
		{
			return ImageEncrypt(Filename, FileExport.c_str());
		}
		else
		{
			return ImageDecrypt(Filename, FileExport.c_str());
		}
	}
	else
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"Extension no encontrada en el catalogo de conversion: %s", Filename);
	}
	return false;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}