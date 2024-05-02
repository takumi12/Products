#include "framework.h"
#include "CGFxEIHandler.h"


GFxEIHandler__OnLoad EIHandler__OnLoad;
GFxEIHandler__OnRender EIHandler__OnRender;



CGFxEIHandler::CGFxEIHandler()
{
	onLoad = false;
}

CGFxEIHandler::~CGFxEIHandler()
{
}


CGFxEIHandler* CGFxEIHandler::Instance()
{
	static CGFxEIHandler s_instance;
	return &s_instance;
}

void CGFxEIHandler::OnLoad(int width, int height)
{
	if (onLoad == false)
	{
		onLoad = true;
		EIHandler__OnLoad(width, height);
	}
}

void CGFxEIHandler::OnRender()
{
	EIHandler__OnRender();
}

void CGFxEIHandler::OnUpdate()
{
}

void CGFxEIHandler::DinamicLibrary()
{
	HMODULE module = LoadLibrary("gfx.dll");
	if (module != nullptr)
	{
		void (*GFxInitializer)() = (void(*)())GetProcAddress(module, "GFxInitializer");
		EIHandler__OnLoad = (GFxEIHandler__OnLoad)GetProcAddress(module, "GFxOnLoad");
		EIHandler__OnRender = (GFxEIHandler__OnRender)GetProcAddress(module, "GFxRenderFrame");

		if (GFxInitializer != nullptr)
		{
			GFxInitializer();

			return;
		}
	}
}

