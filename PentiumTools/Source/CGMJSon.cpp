#include "framework.h"
#include "CGMJSon.h"

bool CGMJSon::OpenFile()
{
	std::ifstream f(filename);

	if (!f.is_open()) {
		printf("Error al abrir el archivo JSON");
		JsonTemplate = json();
		//MessageBox(g_hWnd, "Error al abrir el archivo JSON", "IMGConvertor", MB_OK);
		return false;
	}
	else
	{
		std::stringstream buffer;
		buffer << f.rdbuf();
		std::string fileContent = buffer.str();
		f.close();
		JsonTemplate = json::parse(fileContent);

		index = 0;
		JsonElement = JsonTemplate[index];
		return true;
	}
}

json CGMJSon::GetElement()
{
	return JsonElement;
}

json CGMJSon::OpenTemplate()
{
	std::ifstream f(filename);

	if (!f.is_open())
	{
		printf("Error al abrir el archivo JSON");
		JsonTemplate = json();
	}
	else
	{
		std::stringstream buffer;
		buffer << f.rdbuf();
		std::string fileContent = buffer.str();
		f.close();
		JsonTemplate = json::parse(fileContent);

		JsonElement = JsonTemplate[index];
	}
	return JsonTemplate;
}
