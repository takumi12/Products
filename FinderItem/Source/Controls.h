#pragma once
#define INVENTORY_SIZE			254

extern void RenderCheckBox(float RenderFrameX, float RenderFrameY, const char* label, bool* value);
extern void RenderLabel(float RenderFrameX, float RenderFrameY, ImU32 col_black, const char* fmt, ...);
extern bool RenderButton(float RenderFrameX, float RenderFrameY, const char* label, const ImVec2& size = ImVec2(0, 0));
extern void RenderInputNumber(float RenderFrameX, float RenderFrameY, const char* label, DWORD* value, int width = 0, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
extern bool runtime_check_inventory(DWORD SerialCheck, char* AccountID, char* CharacterName, int& item_index, int& type, int Size01, int Size02);
extern bool runtime_check_warehouse(DWORD SerialCheck, char* AccountID, char* CharacterName, int& item_index, int& type, int Size01, int Size02);
