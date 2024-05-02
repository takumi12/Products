#include "framework.h"
#include "Util.h"
#include "CGMActionLog.h"
#include "CGMFileSystem.h"
#include "CGMFilesMap.h"

bool WIN_OBJECT = false;

CGMFilesMap::CGMFilesMap()
{
	g_iTotalObj = 0;
	currentVersion = 0;
}

CGMFilesMap::~CGMFilesMap()
{
}

CGMFilesMap* CGMFilesMap::Instance()
{
	static CGMFilesMap tc;
	return &tc;
}

void CGMFilesMap::OpenFileObject()
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

void CGMFilesMap::ExportFileObject()
{
	std::string FileName;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.obj" };

	if (GMFileSystem->CreateFileDialog(FileName, L"Save File", CLSID_FileSaveDialog, fileType, true, FOS_OVERWRITEPROMPT))
	{
		FILE* fp = fopen(FileName.c_str(), "wb");

		BYTE Version = 0;
		fwrite(&Version, sizeof(BYTE), 1, fp);
		fwrite(&currentMap, 1, 1, fp);
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

void CGMFilesMap::CreateTableObj(const char* filename)
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

void CGMFilesMap::RenderTableObjectBB()
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

void CGMFilesMap::RenderInformationOBB()
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