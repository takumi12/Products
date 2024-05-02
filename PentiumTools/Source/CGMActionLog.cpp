#include "framework.h"
#include "CGMActionLog.h"

void CGMActionLog::Push(const std::string& entry)
{
	logDeque.push_back(entry);

	if (logDeque.size() > maxSize)
	{
		logDeque.pop_front();
	}
}

void CGMActionLog::RenderLog()
{
	ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Console", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	ImGui::TextWrapped("[Event console]");
	ImGui::Separator();

	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

	if (ImGui::BeginChild("ConsoleScrolling", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

		for (const auto& entry : logDeque)
		{
			ImVec4 color;
			bool has_color = false;
			if (strstr(entry.c_str(), "[error]"))
			{
				color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				has_color = true;
			}
			else if (strncmp(entry.c_str(), "# ", 2) == 0)
			{
				color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true;
			}

			if (has_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color);

			ImGui::TextUnformatted(entry.c_str());

			if (has_color)
				ImGui::PopStyleColor();
		}

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line
	/*bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
	if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
	{
		char* s = InputBuf;
		Strtrim(s);
		if (s[0])
			ExecCommand(s);
		strcpy(s, "");
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget*/
	
	ImGui::End();
}

void CGMActionLog::RenderChildLog()
{
	ImGui::BeginChild("Console View", ImVec2(0, 0), ImGuiChildFlags_Border);

	ImGui::TextWrapped("[Event console]");

	ImGui::Separator();

	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

	if (ImGui::BeginChild("ConsoleScrolling", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

		for (const auto& entry : logDeque)
		{
			ImVec4 color;
			bool has_color = false;
			if (strstr(entry.c_str(), "[error]"))
			{
				color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				has_color = true;
			}
			else if (strncmp(entry.c_str(), "# ", 2) == 0)
			{
				color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true;
			}

			if (has_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color);

			ImGui::TextUnformatted(entry.c_str());

			if (has_color)
				ImGui::PopStyleColor();
		}

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();

	ImGui::EndChild();
}
