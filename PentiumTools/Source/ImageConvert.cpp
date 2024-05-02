#include "framework.h"
#include "CGMFileSystem.h"
#include "CGlobalBitmap.h"
#include "CGMActionLog.h"
#include "ImageConvert.h"

bool GraphicImage = false;

void RenderImageConvert()
{
	GMFileSystem->LoadDirectory();

	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));

	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	if (ImGui::Begin("Graphic Image", &GraphicImage, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::BeginChild("FileImage", ImVec2(300, 0), ImGuiChildFlags_Border);

		GMFileSystem->RenderTreeNode();

		ImGui::EndChild();

		if (ImGui::BeginPopupContextItem("MenuNodo"))
		{
			if (GMFileSystem->IsNodoSelect())
			{
				if (ImGui::MenuItem("Save Image"))
				{
					GMFileSystem->SaveImage();
				}
			}
			if (ImGui::MenuItem("Update"))
			{
				GMFileSystem->ReloadDirectory();
			}
			ImGui::EndPopup();
		}

		ImGui::SameLine();

		ImGui::BeginChild("LayoutImage", ImVec2(0, 0), ImGuiChildFlags_None);

		GlobalBitmap->RenderImage();

		GMActionLog->RenderChildLog();

		ImGui::EndChild();

		ImGui::End();
	}

	if (GraphicImage == false)
	{
		SettingInterface = 0;
	}
}
