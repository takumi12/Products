// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"
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

	if ((Success = FileName.find(".TGA"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".tga"), Success != -1))
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
	return (Success != -1);
}

bool Export(std::string FileName, std::string& FileExport, bool& Encrypt)
{
	size_t Success = -1;

	if ((Success = FileName.find(".TGA"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".OZT");
	}
	else if ((Success = FileName.find(".tga"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".ozt");
	}
	else if ((Success = FileName.find(".OZT"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".TGA");
	}
	else if ((Success = FileName.find(".ozt"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".tga");
	}
	return (Success != -1);
}

bool ImageEncrypt(const char* filename, const char* fileExport, int DumpHeader)
{
	FILE* fp; bool succes = false;
	if ((fp = fopen(filename, "rb"), fp != NULL))
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
			succes = true;
			CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE,"Imagen convertida Correctamente");
		}
		else
		{
			CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"No se pudo crear el archivo en la ruta: %s", fileExport);
		}
		SAFE_DELETE_ARRAY(pTempBuff);
	}
	else
	{
		CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"Archivo no encontrado: %s", filename);
	}
	return succes;
}

bool ImageDecrypt(const char* filename, const char* fileExport, int DumpHeader)
{
	FILE* fp; bool succes = false;
	if ((fp = fopen(filename, "rb"), fp != NULL))
	{
		fseek(fp, 0, SEEK_END);
		int size = (ftell(fp) - DumpHeader);
		fseek(fp, DumpHeader, SEEK_SET);

		char* pTempBuff = new char[size];
		fread(pTempBuff, 1, size, fp);
		fclose(fp);

		if ((fp = fopen(fileExport, "wb"), fp != NULL))
		{
			fwrite(pTempBuff, 1, size, fp);
			fclose(fp);
			succes = true;
			CreateMessageBox(MB_OK | MB_ICONWARNING, IMAGEN_TITLE,"Imagen convertida Correctamente");
		}
		else
		{
			CreateMessageBox(MB_OK | MB_ICONERROR, IMAGEN_TITLE,"No se pudo crear el archivo en la ruta: %s", fileExport);
		}
		SAFE_DELETE_ARRAY(pTempBuff);
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
	BYTE* PakBuffer = NULL;

	fseek(fp, 0, SEEK_END);
	int Size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	PakBuffer = new BYTE[Size];
	fread(PakBuffer, 1, Size, fp);
	fclose(fp);

	int index = 12;

	if (Encrypt)
		index += 4;

	short nx = *((short*)(PakBuffer + index)); index += 2;
	short ny = *((short*)(PakBuffer + index)); index += 2;
	char bit = *((char*)(PakBuffer + index)); index += 2;

	if (bit != 32)
	{
		SAFE_DELETE_ARRAY(PakBuffer);
		return false;
	}

	int Width = nx, Height = ny;

	bitmap->output_width = (float)Width;

	bitmap->output_height = (float)Height;

	bitmap->Components = 4;

	size_t BufferSize = Width * Height * bitmap->Components;

	BYTE* Buffer = new BYTE[BufferSize];

	for (int y = 0; y < ny; y++)
	{
		unsigned char* src = &PakBuffer[index];
		index += nx * 4;
		unsigned char* dst = &Buffer[(ny - 1 - y) * Width * bitmap->Components];

		for (int x = 0; x < nx; x++)
		{
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst[3] = src[3];
			src += bitmap->Components;
			dst += bitmap->Components;
		}
	}

	glGenTextures(1, &(bitmap->uiBitmapIndex));

	glBindTexture(GL_TEXTURE_2D, bitmap->uiBitmapIndex);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, uiFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, uiFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, uiWrapMode);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, uiWrapMode);

	SAFE_DELETE_ARRAY(PakBuffer);

	SAFE_DELETE_ARRAY(Buffer);

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