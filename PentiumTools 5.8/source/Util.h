#pragma once
#include "steady_clock.h"

#include "CGMFrame.h"
#include "CGMItemMng.h"
#include "CGMModelManager.h"

extern void changeExtension(std::string& FileExport);
extern std::string GetFileNameWithExtension(const std::string& filePath);

extern void BuxConvert(BYTE* pbyBuffer, int Size);
extern DWORD GenerateCheckSum2(BYTE* pbyBuffer, int dwSize, WORD Key);
extern void PackFileEncrypt(std::string filename, BYTE* pbyBuffer, int MAX_LINE, int Size);
extern void PackFileEncrypt(std::string filename, BYTE* pbyBuffer, int MAX_LINE, int Size, DWORD Key, bool WriteMax = true, bool CheckSum = true);
extern int PackFileDecrypt(std::string filename, BYTE** pbyBuffer, int MAX_LINE, int Size, DWORD Key = 0);

inline bool Strnicmp(const char* _Str1, const char* _Str)
{
	return !strnicmp(_Str1, _Str, strlen(_Str));
}

template<typename T>
inline int PackFileDecrypt(const std::string& filename, std::vector<T>& pbyBuffer, int MAX_LINE, int Size, DWORD Key)
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

			pbyBuffer.clear();
			pbyBuffer.resize(MAX_LINE);

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

				for (int i = 0; i < MAX_LINE; i++)
				{
					T info;
					BuxConvert(pSeek, Size);
					memcpy(&info, pSeek, Size);

					pbyBuffer[i] = info;
					pSeek += Size;
				}
			}
		}
		else
		{
			MessageBox(NULL, "Data Count 0", "TxtConvertor", MB_OK);
			fclose(fp);
			return 0;
		}
	}
	return MAX_LINE;
}

template<typename T>
inline int PackFileDecrypt(const std::string& filename, std::map<int, T>& pbyBuffer, int MAX_LINE, int Size, DWORD Key)
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

			pbyBuffer.clear();
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
				for (int i = 0; i < MAX_LINE; i++)
				{
					T info;
					BuxConvert(pSeek, Size);
					memcpy(&info, pSeek, Size);
					pbyBuffer.insert(std::pair<int,T>(info.index, info));
					pSeek += Size;
				}
			}
		}
		else
		{
			MessageBox(NULL, "Data Count 0", "TxtConvertor", MB_OK);
			fclose(fp);
			return 0;
		}
	}
	return MAX_LINE;
}


//=========================================================================================================================

template<typename T>
inline int PackFileDecrypt(const std::string& filename, T*& pbyBuffer, int MAX_LINE, int Size, DWORD Key)
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
			long fileSize = ftell(fp);

			if (Key != 0)
				fileSize -= 4;

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

				pbyBuffer = new T[MAX_LINE];
				for (int i = 0; i < MAX_LINE; i++)
				{
					BuxConvert(pSeek, Size);
					memcpy(&pbyBuffer[i], pSeek, Size);
					pSeek += Size;
				}
			}
		}
		else
		{
			fclose(fp);
			MessageBox(NULL, "Data Count 0", "TxtConvertor", MB_OK);
			return 0;
		}
	}
	return MAX_LINE;
}
//=========================================================================================================================


inline float Render22(float a1, float a2) //Main S13; Not here
{
	float v5;
	float v2 = a2 * 0.01745f;
	float v6 = (float)((int)(v2 * 1000.0f / a1 + timeGetTime()) % (int)(6283.185546875f / a1)) * 0.001f * a1;

	if (v6 >= 3.14f)
		v5 = cosf(v6);
	else
		v5 = -cosf(v6);
	return (float)((v5 + 1.0f) * 0.5f);
}