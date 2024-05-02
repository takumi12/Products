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

	if ((Success = FileName.find(".JPG"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".jpg"), Success != -1))
	{
		Encrypt = false;
	}
	else if ((Success = FileName.find(".OZJ"), Success != -1))
	{
		Encrypt = true;
	}
	else if ((Success = FileName.find(".ozj"), Success != -1))
	{
		Encrypt = true;
	}
	return (Success != -1);
}

bool Export(std::string FileName, std::string& FileExport, bool& Encrypt)
{
	size_t Success = -1;

	if ((Success = FileName.find(".JPG"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".OZJ");
	}
	else if ((Success = FileName.find(".jpg"), Success != -1))
	{
		Encrypt = true;
		FileExport = FileName.replace(Success, 4, ".ozj");
	}
	else if ((Success = FileName.find(".OZJ"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".JPG");
	}
	else if ((Success = FileName.find(".ozj"), Success != -1))
	{
		Encrypt = false;
		FileExport = FileName.replace(Success, 4, ".jpg");
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

void my_error_exit(j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr)cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

extern "C" _declspec(dllexport) bool OpenImage(const char* Filename, BITMAP_t* bitmap , GLuint uiFilter, GLuint uiWrapMode)
{
	bool Encrypt = false;

	if(!IsFileEnc(Filename, Encrypt))
	{
		return false;
	}

	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	FILE* infile = fopen(Filename, "rb");

	if (infile == NULL)
	{
		return false;
	}

	if (Encrypt) fseek(infile, 24, SEEK_SET);

	if (setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return false;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);

	(void)jpeg_read_header(&cinfo, TRUE);
	(void)jpeg_start_decompress(&cinfo);

	int Width = (int)cinfo.output_width, Height = (int)cinfo.output_height;

	bitmap->output_width = cinfo.output_width;

	bitmap->output_height = cinfo.output_height;

	bitmap->Components = 3;

	size_t BufferSize = Width * Height * bitmap->Components;

	BYTE* Buffer = new BYTE[BufferSize];

	int offset = 0;
	int row_stride = cinfo.output_width * cinfo.output_components;
	JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		if (offset + row_stride > (int)BufferSize)
			break;

		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(Buffer + (cinfo.output_scanline - 1) * Width * cinfo.output_components, buffer[0], row_stride);
		offset += row_stride;
	}

	glGenTextures(1, &(bitmap->uiBitmapIndex));

	glBindTexture(GL_TEXTURE_2D, bitmap->uiBitmapIndex);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, Buffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, uiFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, uiFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, uiWrapMode);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, uiWrapMode);

	SAFE_DELETE_ARRAY(Buffer);

	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	if (infile)
	{
		fclose(infile);
	}
	return true;
}

extern "C" _declspec(dllexport) bool SaveImage(const char* Filename, bool Encrypt)
{
	int DumpHeader = 24;
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