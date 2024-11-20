#include "framework.h"
#include "Util.h"
#include "MuCrypto.h"
#include "CGMActionLog.h"
#include "CGMFileSystem.h"
#include "CGMFilesOBB.h"

bool WIN_OBJECT = false;

CGMFilesOBB::CGMFilesOBB()
{
	g_iTotalObj = 0;
	currentVersion = 0;
}

CGMFilesOBB::~CGMFilesOBB()
{
}

CGMFilesOBB* CGMFilesOBB::Instance()
{
	static CGMFilesOBB tc;
	return &tc;
}

void CGMFilesOBB::OpenFileObject()
{
	std::string filename;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.obj" };

	obj_bb.clear();
	g_iTotalObj = 0;
	currentVersion = 0;

	if (GMFileSystem->CreateFileDialog(filename, L"Select a File", CLSID_FileOpenDialog, fileType))
	{
		CreateTableObj(filename.c_str());
	}
}

void CGMFilesOBB::ExportFileObject()
{
	std::string FileName;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.obj" };

	if (GMFileSystem->CreateFileDialog(FileName, L"Save File", CLSID_FileSaveDialog, fileType, true, FOS_OVERWRITEPROMPT))
	{
		FILE* fp = fopen(FileName.c_str(), "wb");

		BYTE Version = 0;

		fwrite(&Version, sizeof(BYTE), 1, fp);

		BYTE mape = currentMap;

		if (currentMap > 99)
		{
			mape = 99;
			fwrite(&mape, 1, 1, fp);
		}
		else
		{
			fwrite(&mape, 1, 1, fp);
		}
		fseek(fp, 4, SEEK_SET);

		int numerador = 0;
		for (auto& obb : obj_bb)
		{
			if (obb.Index < 160)
			{
				fwrite(&obb.Index, 2, 1, fp);
				fwrite(&obb.Position, 12, 1, fp);
				fwrite(&obb.Angle, 12, 1, fp);
				fwrite(&obb.Scale, sizeof(float), 1, fp);
				numerador++;
			}
		}

		int EndPoint = ftell(fp);
		fseek(fp, 2, SEEK_SET);
		fwrite(&numerador, 2, 1, fp);
		fseek(fp, EndPoint, SEEK_SET);
		fclose(fp);

		fp = fopen(FileName.c_str(), "rb");
		fseek(fp, 0, SEEK_END);
		int EncBytes = ftell(fp);	//
		fseek(fp, 0, SEEK_SET);
		unsigned char* EncData = new unsigned char[EncBytes];	//
		fread(EncData, 1, EncBytes, fp);	//
		fclose(fp);

		int DataBytes = MapFileEncrypt(NULL, EncData, EncBytes);	//
		unsigned char* Data = new unsigned char[DataBytes];		//
		MapFileEncrypt(Data, EncData, EncBytes);
		delete[] EncData;

		fp = fopen(FileName.c_str(), "wb");
		fwrite(Data, DataBytes, 1, fp);
		fclose(fp);
		delete[] Data;

		GMActionLog->Push("Se exporto el archivo Obj a:" + FileName);

		CreateMessageBox(MB_OK | MB_ICONWARNING, "Object Export", "Objects exported successfully");
	}
	else
	{
		GMActionLog->Push("Se cancelo la exportacion del archivo .obj");
	}
}

void CGMFilesOBB::CreateTableObj(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL)
	{
		char Text[256];
		sprintf(Text, "%s file not found.", filename);
		MessageBox(g_hWnd, Text, NULL, MB_OK);
		return;
	}
	fseek(fp, 0, SEEK_END);
	int EncBytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* EncData = new unsigned char[EncBytes];
	fread(EncData, 1, EncBytes, fp);
	fclose(fp);

	int DataBytes = MapFileDecrypt(NULL, EncData, EncBytes);
	unsigned char* Data = new unsigned char[DataBytes];
	MapFileDecrypt(Data, EncData, EncBytes);
	delete[] EncData;

	int DataPtr;

	DataPtr = 0;
	BYTE Version = *((BYTE*)(Data + DataPtr));
	DataPtr++;
	int iMapNumber = (int)(*((BYTE*)(Data + DataPtr)));
	DataPtr++;
	short Count = *((short*)(Data + DataPtr));
	DataPtr = 4;

	g_iTotalObj = Count;
	currentMap = iMapNumber;
	currentVersion = Version;

	for (int i = 0; i < Count; i++)
	{
		OBJECT_OBB Obj;

		Obj.Index = *((short*)(Data + DataPtr));
		DataPtr += 2;

		memcpy(Obj.Position, Data + DataPtr, 12);
		DataPtr += 12;

		memcpy(Obj.Angle, Data + DataPtr, 12);
		DataPtr += 12;

		Obj.Scale = *((float*)(Data + DataPtr));
		DataPtr += 4;

		if (Version == 1 || Version == 2 || Version == 3)
		{
			Obj.LightEnable = *((BYTE*)(Data + DataPtr++));
			Obj.EnableFullLight = *((BYTE*)(Data + DataPtr++));
			if (Version == 2 || Version == 3)
			{
				Obj.EnablePrimaryLight = *((BYTE*)(Data + DataPtr++));
				if (Version == 3)
				{
					memcpy(Obj.Light, Data + DataPtr, 12);
					DataPtr += 12;
				}
			}
		}
		obj_bb.push_back(Obj);
	}
	delete[] Data;
}

void CGMFilesOBB::RenderTableObjectBB()
{
	ImGui::BeginChild("ViewOBB", ImVec2(0, -150), ImGuiChildFlags_Border);

	ImGui::Text("Table Object: %d", g_iTotalObj);

	ImGui::Separator();

	if (g_iTotalObj > 0)
	{
		static const char* TablaObb[] = {"Index", "Position X", "Position Y", "Position Z",
			"Angle X" , "Angle Y" , "Angle Z", "Scale", "Light", "FullLight",
			"PrimaryLight", "Light R", "Light G", "Light B",
		};
		int num_colum = 8;

		if (currentVersion == 1 || currentVersion == 2 || currentVersion == 3)
		{
			num_colum += 2;
			if (currentVersion == 2 || currentVersion == 3)
			{
				num_colum += 1;
				if (currentVersion == 3)
				{
					num_colum += 3;
				}
			}
		}

		ImGui::BeginChild("View_TablaOBB", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
		// Comienza la tabla con 3 columnas
		if (ImGui::BeginTable("MiTablaOBB", num_colum, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			// Encabezados de las columnas
			for (int i = 0; i < num_colum; i++)
			{
				ImGui::TableSetupColumn(TablaObb[i]);

			}
			ImGui::TableHeadersRow();

			int i = 0;
			for (auto& obb : obj_bb)
			{
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				//ImGui::InputScalar("##index_obb", ImGuiDataType_S16, &obb.Index);
				//ImGui::InputNumber("##index_obb", i, &obb.Index, "%d", ImGuiDataType_S16);
				ImGui::Text("%d", obb.Index);
				ImGui::TableNextColumn();
				//ImGui::InputNumber("##Positionx_obb", i, &obb.Position[0], "%.3f", ImGuiDataType_Float);
				ImGui::Text("%.4f", obb.Position[0]);
				ImGui::TableNextColumn();
				ImGui::Text("%.4f", obb.Position[1]);
				ImGui::TableNextColumn();
				ImGui::Text("%.4f", obb.Position[2]);
				ImGui::TableNextColumn();
				ImGui::Text("%.4f", obb.Angle[0]);
				ImGui::TableNextColumn();
				ImGui::Text("%.4f", obb.Angle[1]);
				ImGui::TableNextColumn();
				ImGui::Text("%.4f", obb.Angle[2]);
				ImGui::TableNextColumn();
				ImGui::Text("%.4f", obb.Scale);
				if (currentVersion == 1 || currentVersion == 2 || currentVersion == 3)
				{
					ImGui::TableNextColumn();
					ImGui::Text("%d", obb.LightEnable);
					ImGui::TableNextColumn();
					ImGui::Text("%d", obb.EnableFullLight);
					if (currentVersion == 2 || currentVersion == 3)
					{
						ImGui::TableNextColumn();
						ImGui::Text("%d", obb.EnablePrimaryLight);
						if (currentVersion == 3)
						{
							ImGui::TableNextColumn();
							ImGui::Text("%.4f", obb.Light[0]);
							ImGui::TableNextColumn();
							ImGui::Text("%.4f", obb.Light[1]);
							ImGui::TableNextColumn();
							ImGui::Text("%.4f", obb.Light[2]);
						}
					}
				}
				i++;
			}
			// Finaliza la tabla
			ImGui::EndTable();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

void CGMFilesOBB::RenderInformationOBB()
{
	ImGui::Text("File Information:");

	ImGui::Separator();

	if (g_iTotalObj > 0)
	{
		ImGui::Text("Objects: %d", g_iTotalObj);

		//ImGui::Separator();

		ImGui::Text("Version Enc: %d", currentVersion);

		//ImGui::Separator();

		ImGui::Text("Map: %d", currentMap);

		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("Version 0: Season 6+");
		ImGui::Text("Version 1: Season 13+");
		ImGui::Text("Version 2: Season 14+");
		ImGui::Text("Version 3: Season 16+");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("ATTENTION!!!");
		ImGui::Text("Exported files will \nbe converted to version 0");
		ImGui::Spacing();
		ImGui::Separator();

		if (ImGui::Button("Export"))
		{
			ExportFileObject();
			// Código a ejecutar cuando se hace clic en el botón
			// Este bloque se ejecutará cuando el botón sea presionado
			// Puedes colocar aquí la lógica relacionada con el botón.
		}
	}
	else
	{
		ImGui::Spacing();
		ImGui::Text("Version 0: Season 6+");
		ImGui::Text("Version 1: Season 13+");
		ImGui::Text("Version 2: Season 14+");
		ImGui::Text("Version 3: Season 16+");
	}
}

void CGMFilesOBB::OpenFileMap()
{
	std::string filename;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.map" };

	if (GMFileSystem->CreateFileDialog(filename, L"Select a File", CLSID_FileOpenDialog, fileType))
	{
		ReadDataMap(filename.c_str());
	}
}

void CGMFilesOBB::ReadDataMap(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL)
	{
		return;
	}

	fseek(fp, 0, SEEK_END);

	unsigned int FileSize = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	BYTE Enc[4];

	fread(&Enc, 4u, 1u, fp);

	std::vector<BYTE> FileBuffer;

	if (Enc[0] != 'M' || Enc[1] != 'A' || Enc[2] != 'P' || Enc[3] != 1)
	{
		BYTE* EncData = new BYTE[FileSize];

		fseek(fp, 0, SEEK_SET);

		fread(EncData, 1u, FileSize, fp);

		fclose(fp);

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		MapFileDecrypt(FileBuffer.data(), EncData, FileSize);

		delete[] (EncData);
	}
	else
	{
		FileSize -= 4;

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		fread(FileBuffer.data(), 1, FileSize, fp);

		fclose(fp);

		MuFileDecrypt2(FileBuffer);
	}

	FileBuffer[0] = 0;

	if(FileBuffer[1] > 99)
		FileBuffer[1] = 99;

	std::string FileName;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.map" };

	if (GMFileSystem->CreateFileDialog(FileName, L"Save File", CLSID_FileSaveDialog, fileType, true, FOS_OVERWRITEPROMPT))
	{
		WriteDataMap(FileName.c_str(), FileBuffer.data(), FileBuffer.size());
	}
}

void CGMFilesOBB::WriteDataMap(const char* filename, unsigned char* EncData, int EncBytes)
{
	FILE* fp = fopen(filename, "wb");

	if (fp != NULL)
	{
		int DataBytes = MapFileEncrypt(NULL, EncData, EncBytes);
		unsigned char* Data = new unsigned char[DataBytes];
		MapFileEncrypt(Data, EncData, EncBytes);

		fwrite(Data, DataBytes, 1, fp);
		fclose(fp);
		delete[] Data;
	}
}

void CGMFilesOBB::OpenFileATT()
{
	std::string filename;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.att" };

	if (GMFileSystem->CreateFileDialog(filename, L"Select a File", CLSID_FileOpenDialog, fileType))
	{
		ReadDataATT(filename.c_str());
	}
}

void CGMFilesOBB::ReadDataATT(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL)
	{
		return;
	}

	fseek(fp, 0, SEEK_END);

	unsigned int FileSize = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	BYTE Enc[4];

	fread(&Enc, 4u, 1u, fp);

	std::vector<BYTE> FileBuffer;

	if (Enc[0] != 'A' || Enc[1] != 'T' || Enc[2] != 'T' || Enc[3] != 1)
	{
		BYTE* EncData = new BYTE[FileSize];

		fseek(fp, 0, SEEK_SET);

		fread(EncData, 1u, FileSize, fp);

		fclose(fp);

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		MapFileDecrypt(FileBuffer.data(), EncData, FileSize);

		delete[](EncData);
	}
	else
	{
		FileSize -= 4;

		FileBuffer = std::vector<BYTE>(FileSize, 0);

		fread(FileBuffer.data(), 1, FileSize, fp);

		fclose(fp);

		MuFileDecrypt2(FileBuffer);
	}

	BYTE* blockchain = FileBuffer.data();

	BuxConvert(blockchain, FileSize);

	blockchain[0] = 0;
	if (blockchain[1] > 99)
		blockchain[1] = 99;

	std::string FileName;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.att" };

	if (GMFileSystem->CreateFileDialog(FileName, L"Save File", CLSID_FileSaveDialog, fileType, true, FOS_OVERWRITEPROMPT))
	{
		WriteDataATT(FileName.c_str(), FileBuffer.data(), FileBuffer.size());
	}
}

void CGMFilesOBB::WriteDataATT(const char* filename, unsigned char* EncData, int EncBytes)
{
	FILE* fp = fopen(filename, "wb");

	if (fp != NULL)
	{
		BuxConvert(EncData, EncBytes);

		int DataBytes = MapFileEncrypt(NULL, EncData, EncBytes);
		unsigned char* Data = new unsigned char[DataBytes];
		MapFileEncrypt(Data, EncData, EncBytes);

		fwrite(Data, DataBytes, 1, fp);
		fclose(fp);
		delete[] Data;
	}
}



unsigned char BMPHeader[1080];
float BackTerrainHeight[TERRAIN_SIZE * TERRAIN_SIZE];

void CGMFilesOBB::OpenTerrainHeight()
{
	std::string filename;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.ozb" };

	if (GMFileSystem->CreateFileDialog(filename, L"Terrain Height", CLSID_FileOpenDialog, fileType))
	{
		ReadDataOZB(filename.c_str());
	}
}

void CGMFilesOBB::ReadDataOZB(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL)
	{
		return;
	}

	fseek(fp, 0, SEEK_END);
	int iBytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	BYTE* pbyData = new BYTE[iBytes];
	fread(pbyData, 1, iBytes, fp);
	fclose(fp);

	DWORD dwCurPos = 4;
	BITMAPFILEHEADER header;
	memcpy(&header, &pbyData[dwCurPos], sizeof(BITMAPFILEHEADER));
	dwCurPos += sizeof(BITMAPFILEHEADER);

	BITMAPINFOHEADER bmiHeader;
	memcpy(&bmiHeader, &pbyData[dwCurPos], sizeof(BITMAPINFOHEADER));
	dwCurPos += sizeof(BITMAPINFOHEADER);

	for (int i = 0; i < (TERRAIN_SIZE * TERRAIN_SIZE); ++i)
	{
		DWORD dwHeight = 0;
		BYTE* pbysrc = &pbyData[dwCurPos + i * 3];
		BYTE* pbyHeight = (BYTE*)&dwHeight;

		pbyHeight[0] = pbysrc[2];
		pbyHeight[1] = pbysrc[1];
		pbyHeight[2] = pbysrc[0];

		BackTerrainHeight[i] = (float)dwHeight;
		BackTerrainHeight[i] += (-500.f);
	}
	delete[] pbyData;

	dwCurPos = 0;
	memcpy(&BMPHeader[dwCurPos], &header, sizeof(BITMAPFILEHEADER));
	dwCurPos += sizeof(BITMAPFILEHEADER);
	memcpy(&BMPHeader[dwCurPos], &bmiHeader, sizeof(BITMAPINFOHEADER));
	dwCurPos += sizeof(BITMAPINFOHEADER);

	for (int i = 0; i < 256; i++)
	{
		BMPHeader[dwCurPos + i * 4 + 0] = i; // Blue
		BMPHeader[dwCurPos + i * 4 + 1] = i; // Green
		BMPHeader[dwCurPos + i * 4 + 2] = i; // Red
		BMPHeader[dwCurPos + i * 4 + 3] = 0; // Reserved
	}

	std::string FileName;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.ozb" };

	if (GMFileSystem->CreateFileDialog(FileName, L"Save File", CLSID_FileSaveDialog, fileType, true, FOS_OVERWRITEPROMPT))
	{
		WriteDataOZB(FileName.c_str(), 1);
	}
}

void CGMFilesOBB::WriteDataOZB(const char* filename, int EncBytes)
{
	FILE* fp = fopen(filename, "wb");

	if (fp)
	{
		unsigned char* Buffer = new unsigned char[256 * 256];

		for (int i = 0; i < 256; i++)
		{
			float* src = &BackTerrainHeight[i * 256];
			unsigned char* dst = &Buffer[(255 - i) * 256];

			for (int j = 0; j < 256; j++)
			{
				float Height = *src / 1.5f;

				if (Height < 0.f)
					Height = 0.f;
				else if (Height > 255.f)
					Height = 255.f;

				*dst = (unsigned char)(Height);

				src++;
				dst++;
			}
		}

		char EncData[] = {'O', 'Z', 'B', 1};
		fwrite(&EncData, 4, 1, fp);
		fwrite(BMPHeader, 1080, 1, fp);

		for (int i = 0; i < 256; i++)
			fwrite(Buffer + (255 - i) * 256, 256, 1, fp);

		fclose(fp);
	}
}

void RenderInterfaceMaps()
{
	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));

	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	ImGui::Begin("Maps Object's BB", &WIN_OBJECT, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	if (WIN_OBJECT == false)
	{
		SettingInterface = 0;
	}

	ImGui::BeginChild("SettingOBB", ImVec2(200, 0), ImGuiChildFlags_Border);

	GMFilesMap->RenderInformationOBB();

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("LayoutOBB", ImVec2(0, 0), ImGuiChildFlags_None);

	GMFilesMap->RenderTableObjectBB();

	GMActionLog->RenderChildLog();

	ImGui::EndChild();

	ImGui::End();
}