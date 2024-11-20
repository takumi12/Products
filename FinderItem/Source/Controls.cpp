#include "framework.h"
#include "Util.h"
#include "Controls.h"
#include "QueryManager.h"


void RenderCheckBox(float RenderFrameX, float RenderFrameY, const char* label, bool* value)
{
	ImGui::SetCursorPos(ImVec2(RenderFrameX, RenderFrameY));
	ImGui::Checkbox(label, value);
}

void RenderLabel(float RenderFrameX, float RenderFrameY, ImU32 col_black, const char* fmt, ...)
{
	ImGui::SetCursorPos(ImVec2(RenderFrameX, RenderFrameY));

	if (col_black != 0)
	{
		char buffer[1024];  // Asegúrate de que el tamaño sea lo suficientemente grande para tu texto
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);  // Formatear el texto en el buffer
		va_end(args);

		// Obtener la posición y calcular el tamaño del texto
		ImVec2 pos = ImGui::GetCursorScreenPos();  // Posición en pantalla del texto
		ImVec2 textSize = ImGui::CalcTextSize(buffer);  // Tamaño del texto generado

		// Dibuja el fondo con el tamaño correcto
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(pos.x-2, pos.y-2),  // Esquina superior izquierda
			ImVec2(pos.x+textSize.x+2, pos.y+textSize.y+2),  // Esquina inferior derecha
			col_black  // Color del fondo (azul en este caso, RGBA)
		);
		ImGui::Text(buffer);
	}
	else
	{
		va_list args;
		va_start(args, fmt);
		ImGui::TextV(fmt, args);  // Usa TextV para manejo de argumentos variables
		va_end(args);
	}
}

bool RenderButton(float RenderFrameX, float RenderFrameY, const char* label, const ImVec2& size)
{
	ImGui::SetCursorPos(ImVec2(RenderFrameX, RenderFrameY));
	return ImGui::Button(label, size);  // Devuelve true si el botón fue presionado
}

void RenderInputNumber(float RenderFrameX, float RenderFrameY, const char* label, DWORD* value, int width, ImGuiInputTextFlags flags)
{
	ImGui::SetCursorPos(ImVec2(RenderFrameX, RenderFrameY));
	if (width != 0)
	{
		ImGui::PushItemWidth(width);
	}

	ImGui::InputInt64(label, value, 0, 0, flags);

	if (width != 0)
	{
		ImGui::PopItemWidth();
	}
}

bool runtime_check_inventory(DWORD SerialCheck, char* AccountID, char* CharacterName, int& item_index, int& type, int Size01, int Size02)
{
	BYTE Inventory[INVENTORY_FULL_SIZE][16];

	if (gQueryManager.ExecQuery("SELECT AccountID,Name,Inventory FROM Character") != 0)
	{
		while (gQueryManager.Fetch() != SQL_NO_DATA)
		{
			memset(Inventory, 0xFF, sizeof(Inventory));

			gQueryManager.GetAsBinary("Inventory", Inventory[0], sizeof(Inventory));

			for (int i = 0; i < INVENTORY_FULL_SIZE; i++)
			{
				DWORD SerialItem = GetSerialItem(Inventory[i]);

				if (SerialItem != 0xFFFFFFFF)
				{
					if (SerialCheck == SerialItem)
					{
						item_index = GetIndexItem(Inventory[i]);

						gQueryManager.GetAsString("Name", CharacterName, Size02);

						gQueryManager.GetAsString("AccountID", AccountID, Size01);

						type = i;
						return true;
					}
				}
			}
		}
	}

	item_index = -1;

	sprintf(AccountID, "---");

	sprintf(CharacterName, "---");

	gQueryManager.Close();

	return false;
}

bool runtime_check_warehouse(DWORD SerialCheck, char* AccountID, char* CharacterName, int& item_index, int& type, int Size01, int Size02)
{
	BYTE Inventory[WAREHOUSE_SIZE][16];

	if (gQueryManager.ExecQuery("SELECT AccountID,Items FROM Warehouse") != 0)
	{
		while (gQueryManager.Fetch() != SQL_NO_DATA)
		{
			memset(Inventory, 0xFF, sizeof(Inventory));

			gQueryManager.GetAsBinary("Items", Inventory[0], sizeof(Inventory));

			for (int i = 0; i < WAREHOUSE_SIZE; i++)
			{
				DWORD SerialItem = GetSerialItem(Inventory[i]);

				if (SerialItem != 0xFFFFFFFF)
				{
					if (SerialCheck == SerialItem)
					{
						item_index = GetIndexItem(Inventory[i]);

						gQueryManager.GetAsString("AccountID", AccountID, Size01);

						sprintf(CharacterName, "---");

						type = i;
						return true;
					}
				}
			}
		}
	}

	item_index = -1;

	sprintf(AccountID, "---");

	sprintf(CharacterName, "---");

	gQueryManager.Close();

	return false;
}
