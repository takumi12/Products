#include "framework.h"
#include "ImageConvert.h"

bool GraphicImage = false;

void SetupImGuiStyle()
{
	// prope style from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 0.800000011920929f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(13.89999961853027f, 11.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 2.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(11.89999961853027f, 7.699999809265137f);
	style.FrameRounding = 5.199999809265137f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 8.199999809265137f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1848440915346146f, 0.1931330561637878f, 0.1893489360809326f, 0.7596566677093506f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3004261553287506f, 0.3004275858402252f, 0.3004291653633118f, 0.4000000059604645f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4255005717277527f, 0.4271916151046753f, 0.4291845560073853f, 0.6700000166893005f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2965609729290009f, 0.2981522977352142f, 0.3004291653633118f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568f, 0.5899999737739563f, 0.9800000190734863f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239999994635582f, 0.5199999809265137f, 0.8799999952316284f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2599999904632568f, 0.5899999737739563f, 0.9800000190734863f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.3106154203414917f, 0.31184983253479f, 0.3133047223091125f, 0.4000000059604645f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3100000023841858f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.196078434586525f, 0.3921568691730499f, 0.2235294133424759f, 0.8627451062202454f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.196078434586525f, 0.6784313917160034f, 0.2235294133424759f, 0.800000011920929f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.7843137383460999f, 0.2235294133424759f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.196078434586525f, 0.3529411852359772f, 0.2235294133424759f, 0.9725490212440491f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.7872986793518066f, 0.9399141669273376f, 0.1129510551691055f, 0.7639485001564026f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

void RenderImageConvert()
{
	//GMFileSystem->LoadDirectory();

	SetupImGuiStyle();

	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));

	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	if (ImGui::Begin("Graphic Image", &GraphicImage, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::BeginChild("FileImage", ImVec2(300, 0), ImGuiChildFlags_Border);

		if (ImGui::TreeNode("File data"))
		{
			bool Selected = (!ImGui::IsWindowFocused());

			if (ImGui::SelectableHovered("Item", Selected, ImGuiSelectableFlags_AllowDoubleClick))
			{
			}
			if (ImGui::SelectableHovered("Skill", Selected, ImGuiSelectableFlags_AllowDoubleClick))
			{
			}
			if (ImGui::SelectableHovered("Itemtooltip", Selected, ImGuiSelectableFlags_AllowDoubleClick))
			{
			}
			if (ImGui::SelectableHovered("MasterSkill", Selected, ImGuiSelectableFlags_AllowDoubleClick))
			{
			}
			if (ImGui::SelectableHovered("MasterSkillTooltip", Selected, ImGuiSelectableFlags_AllowDoubleClick))
			{
			}
			ImGui::TreePop();
		}
		else
		{
			ImGui::TreeNodeOpen("File data");
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("LayoutImage", ImVec2(0, 0), ImGuiChildFlags_Border);

		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

		// Crear la tabla con 3 columnas
		if (ImGui::BeginTable("ExampleTable", 3, flags))
		{
			// Definir los encabezados de las columnas
			ImGui::TableSetupColumn("ID");
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Age");
			ImGui::TableHeadersRow();

			// Agregar filas a la tabla
			for (int row = 0; row < 5; ++row)
			{
				ImGui::TableNextRow();

				// Primera columna: ID
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%d", row + 1);

				// Segunda columna: Name
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Person %d", row + 1);

				// Tercera columna: Age
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%d", 20 + row);
			}

			// Finalizar la tabla
			ImGui::EndTable();
		}

		ImGui::EndChild();

		ImGui::End();
	}
	/*ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));


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
	}*/
}
