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

	if ((Success = FileName.find(".PNG"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".png"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".OZT"), Success != -1))
	{
		Encrypt = true;
	}
	else if ((Success = FileName.find(".ozt"), Success != -1))
	{
		Encrypt = true;
	}
	/*else if ((Success = FileName.find(".OZP"), Success != -1))
	{
		Encrypt = true;
	}
	else if ((Success = FileName.find(".ozp"), Success != -1))
	{
		Encrypt = true;
	}*/
	return (Success != -1);
}

bool Export(std::string FileName, std::string& FileExport, bool& Encrypt)
{
	size_t Success = -1;

	if ((Success = FileName.find(".PNG"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".OZT");
	}
	else if ((Success = FileName.find(".png"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".ozt");
	}
	else if ((Success = FileName.find(".OZT"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".PNG");
	}
	else if ((Success = FileName.find(".ozt"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".png");
	}

	/*if ((Success = FileName.find(".PNG"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".OZP");
	}
	else if ((Success = FileName.find(".png"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".ozp");
	}
	else if ((Success = FileName.find(".OZP"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".PNG");
	}
	else if ((Success = FileName.find(".ozp"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".png");
	}*/
	return (Success != -1);
}

bool ImageEncrypt(const char* filename, const char* fileExport, int DumpHeader)
{
	int Width = 0;
	int Height = 0;
	int channels = 0;

	BYTE* image = SOIL_load_image(filename, &Width, &Height, &channels, SOIL_LOAD_RGBA);

	if (!image)
	{
		return false;
	}

	if (SOIL_save_image(fileExport, SOIL_SAVE_TYPE_TGA, Width, Height, channels, image))
	{
		SOIL_free_image_data(image);

		FILE* fp;

		if ((fp = fopen(fileExport, "rb"), fp != NULL))
		{
			fseek(fp, 0, SEEK_END);
			int size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* pTempBuff = new char[size];
			fread(pTempBuff, 1, size, fp);
			fclose(fp);

			if ((fp = fopen(fileExport, "wb"), fp != NULL))
			{
				fwrite(pTempBuff, 1, DumpHeader, fp);
				fwrite(pTempBuff, 1, size, fp);
				fclose(fp);
				CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE, "Imagen convertida Correctamente");

				SAFE_DELETE_ARRAY(pTempBuff);
				return true;
			}
			else
			{
				SAFE_DELETE_ARRAY(pTempBuff);
				CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE, "No se pudo crear el archivo en la ruta: %s", fileExport);
			}
		}
		else
		{
			CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE, "Archivo no encontrado: %s", filename);
		}
	}
	else
	{
		SOIL_free_image_data(image);
	}

	return false;
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool ImageDecrypt(const char* filename, const char* fileExport, int DumpHeader)
{
	FILE* fp; bool succes = false;

	if ((fp = fopen(filename, "rb"), fp != NULL))
	{
		fseek(fp, 0, SEEK_END);
		int Size = (ftell(fp) - DumpHeader);
		fseek(fp, DumpHeader, SEEK_SET);

		BYTE* PakBuffer = new BYTE[Size];
		fread(PakBuffer, 1, Size, fp);
		fclose(fp);

		int Width = 0;
		int Height = 0;
		int channels = 0;

		BYTE* image = SOIL_load_image_from_memory(PakBuffer, Size, &Width, &Height, &channels, SOIL_LOAD_RGBA);

		SAFE_DELETE_ARRAY(PakBuffer);

		if (!image)
		{
			return false;
		}

		if (stbi_write_png(fileExport, Width, Height, channels, image, Width * channels))
		{
			CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE, "Imagen convertida Correctamente");
		}

		SOIL_free_image_data(image);
		succes = true;

		//if ((fp = fopen(fileExport, "wb"), fp != NULL))
		//{
		//	fwrite(pTempBuff, 1, size, fp);
		//	fclose(fp);
		//	CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE, "Imagen convertida Correctamente");
		//}
		//else
		//{
		//	CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE, "No se pudo crear el archivo en la ruta: %s", fileExport);
		//}
	}
	else
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE, "Archivo no encontrado: %s", filename);
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

	if (Encrypt)
	{
		Size -= 4;
		fseek(fp, 4, SEEK_SET);
	}

	BYTE* PakBuffer = new BYTE[Size];
	fread(PakBuffer, 1, Size, fp);
	fclose(fp);

	int Width = 0;
	int Height = 0;
	int channels = 0;

	BYTE* image = SOIL_load_image_from_memory(PakBuffer, Size, &Width, &Height, &channels, SOIL_LOAD_RGBA);

	if (!image)
	{
		return false;
	}

	//SOIL_save_image("Image.dds", SOIL_SAVE_TYPE_DDS, Width, Height, channels, image);

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

	SAFE_DELETE_ARRAY(PakBuffer);

	return true;
}

extern "C" _declspec(dllexport) bool SaveImage(const char* Filename, bool Encrypt)
{
	int DumpHeader = 4;
	std::string FileExport;

	if(Export(Filename, FileExport, Encrypt))
	{
		if(Encrypt)
		{
			return ImageEncrypt(Filename, FileExport.c_str(), DumpHeader);
		}
		else
		{
			return ImageDecrypt(Filename, FileExport.c_str(), DumpHeader);
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