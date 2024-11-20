#include "StdAfx.h"
#include "Util.h"

static BYTE bBuxCode[3] = { 0xfc, 0xcf, 0xab };


std::string GetFileNameWithExtension(const std::string& filePath)
{
	size_t slashPos = filePath.find_last_of("\\/"); // Encuentra la �ltima barra inclinada.

	if (slashPos != std::string::npos)
	{
		return filePath.substr(slashPos + 1);
	}
	else
	{
		return filePath; // Si no se encuentra una barra inclinada, devolver toda la ruta.
	}
}

void changeExtension(std::string& FileExport)
{
	size_t Success = -1;

	if ((Success = FileExport.find(".TGA"), Success != -1))
	{
		FileExport = FileExport.replace(Success, 4, ".OZT");
		return;
	}
	if ((Success = FileExport.find(".tga"), Success != -1))
	{
		FileExport = FileExport.replace(Success, 4, ".ozt");
		return;
	}
	if ((Success = FileExport.find(".JPG"), Success != -1))
	{
		FileExport = FileExport.replace(Success, 4, ".OZJ");
		return;
	}
	if ((Success = FileExport.find(".jpg"), Success != -1))
	{
		FileExport = FileExport.replace(Success, 4, ".ozj");
		return;
	}
}

void BuxConvert(BYTE* pbyBuffer, int Size)
{
	for (int i = 0; i < Size; i++)
	{
		pbyBuffer[i] ^= bBuxCode[i % 3];
	}
}

DWORD GenerateCheckSum2(BYTE* pbyBuffer, int dwSize, WORD Key)
{
	DWORD dwKey = (DWORD)Key;
	DWORD dwResult = (DWORD)(Key << 9);

	for (int dwChecked = 0; dwChecked <= dwSize - 4; dwChecked += 4)
	{
		DWORD dwTemp;

		memcpy(&dwTemp, pbyBuffer + dwChecked, 4);

		DWORD v4 = (Key + (dwChecked >> 2)) % 2;
		switch (v4)
		{
		case 0:
			dwResult ^= dwTemp;
			break;
		case 1:
			dwResult += dwTemp;
			break;
		}

		if (!(dwChecked % 0x10))
		{
			dwResult ^= (unsigned int)(dwResult + dwKey) >> ((dwChecked >> 2) % 8 + 1);
		}
	}
	return dwResult;
}

void PackFileEncrypt(std::string filename, BYTE* pbyBuffer, int MAX_LINE, int Size)
{
	FILE* fp = fopen(filename.c_str(), "wb");

	if (fp != NULL)
	{
		DWORD MAX_BUFFER = (MAX_LINE * Size);

		BuxConvert(pbyBuffer, MAX_BUFFER);
		fwrite(pbyBuffer, MAX_BUFFER, 1u, fp);
		fclose(fp);
	}
	else
	{
		char Text[256];
		sprintf_s(Text, "%s - File not exist.", filename.c_str());
		MessageBox(g_hWnd, Text, NULL, MB_OK);
	}
}

void PackFileEncrypt(std::string filename, BYTE* pbyBuffer, int MAX_LINE, int Size, DWORD Key, bool WriteMax, bool CheckSum)
{
	FILE* fp = fopen(filename.c_str(), "wb");

	if (fp != NULL)
	{
		DWORD MAX_BUFFER = (MAX_LINE * Size);

		BYTE* Buffer = new BYTE[MAX_BUFFER];

		if (WriteMax == true)
		{
			fwrite(&MAX_LINE, 4u, 1u, fp);
		}

		for (int i = 0; i < MAX_LINE; ++i)
		{
			BuxConvert(pbyBuffer, Size);

			memcpy((char*)(Buffer + Size * i), pbyBuffer, Size);

			fwrite((char*)(Buffer + Size * i), Size, 1u, fp);

			pbyBuffer += Size;
		}

		if (CheckSum)
		{
			DWORD dwCheckSum = GenerateCheckSum2(Buffer, MAX_BUFFER, Key);

			fwrite(&dwCheckSum, 4u, 1u, fp);
		}

		fclose(fp);

		delete[] Buffer;
	}
	else
	{
		char Text[256];
		sprintf_s(Text, "%s - File not exist.", filename.c_str());
		MessageBox(g_hWnd, Text, NULL, MB_OK);
	}
}

int PackFileDecrypt(std::string filename, BYTE** pbyBuffer, int MAX_LINE, int Size, DWORD Key)
{
	FILE* fp = fopen(filename.c_str(), "rb");

	if (fp != NULL)
	{
		if (MAX_LINE == 0)
		{
			fread(&MAX_LINE, 4u, 1u, fp);
		}
		else
		{
			fseek(fp, 0, SEEK_END);
			long fileSize = (ftell(fp) - 4);
			fseek(fp, 0, SEEK_SET);

			if (fileSize > 0 && MAX_LINE != (int)(fileSize / Size))
			{
				MAX_LINE = (fileSize / Size);
			}
		}

		if (MAX_LINE > 0)
		{
			int MAX_BUFFER = MAX_LINE * Size;
			std::vector<BYTE> buffer(MAX_BUFFER);

			fread(buffer.data(), MAX_BUFFER, 1u, fp);
			DWORD dwCheckSum;
			fread(&dwCheckSum, sizeof(DWORD), 1u, fp);
			fclose(fp);

			bool success = false;

			if (Key != 0)
			{
				success = (dwCheckSum != GenerateCheckSum2(buffer.data(), MAX_BUFFER, Key));
			}

			if (success)
			{
				char Text[256];
				sprintf_s(Text, "%s - File corrupted.", filename.c_str());
				MessageBox(NULL, Text, "TxtConvertor", MB_OK);
				return 0;
			}
			else
			{
				BYTE* pSeek = buffer.data();
				(*pbyBuffer) = new BYTE[MAX_BUFFER];

				for (int i = 0; i < MAX_LINE; i++)
				{
					BuxConvert(pSeek, Size);
					memcpy((char*)((*pbyBuffer) + Size * i), pSeek, Size);
					pSeek += Size;
				}
			}
		}
		else
		{
			MessageBox(NULL, "Data Count 0", "TxtConvertor", MB_OK);
			fclose(fp);
		}
	}
	return MAX_LINE;
}
