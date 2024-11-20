// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"
#include "Util.h"
#include "CGFxManager.h"

BoostSmart_Ptr(CGFxManager);

class CSynchronize
{
public:
	void* inheritance;
	// Punteros a funciones de CGFxManager
	bool (*AddObj)(void*& obj, DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, int aligntype);
	void (*CreateDevice)(void*&, DWORD, DWORD);

	bool (*Update)(void*&);
	bool (*Render)(void*&);
	bool (*RenderWithKey)(void*&, DWORD);
	bool (*UpdateWithKey)(void*&, DWORD);
	bool (*UpdateMouseEvent)(void*&, DWORD, int);

	void (*HideAll)(void*&);
	void (*Show)(void*&, DWORD);
	void (*Hide)(void*&, DWORD);
	void (*Toggle)(void*&, DWORD);
	bool (*IsVisible)(void*&, DWORD);
	bool (*Invoke)(void*&, DWORD, const char*, const char*, va_list);
	bool (*InvokeArg)(void*&, DWORD, const char*, CEICommandContainer&);
	bool (*InvokeArray)(void*&, DWORD, const char*, CEICommandContainer*, int);
	void (*CallBack)(void*& obj, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

APIEXPORT void SynchronizeInit(CSynchronize*& ma)
{
	ma = new CSynchronize;
	ma->inheritance = new CGFxManager;
	ma->AddObj = &CGFxManager::AddObjWrapper;
	ma->CreateDevice = &CGFxManager::CreateDevice;
	ma->Update = &CGFxManager::UpdateWrapper;
	ma->Render = &CGFxManager::RenderWrapper;
	ma->RenderWithKey = &CGFxManager::RenderWithKeyWrapper;
	ma->UpdateWithKey = &CGFxManager::UpdateWithKeyWrapper;
	ma->UpdateMouseEvent = &CGFxManager::UpdateMouseEventWrapper;
	ma->HideAll = &CGFxManager::HideAllWrapper;
	ma->Show = &CGFxManager::ShowWrapper;
	ma->Hide = &CGFxManager::HideWrapper;
	ma->Toggle = &CGFxManager::ToggleWrapper;
	ma->IsVisible = &CGFxManager::IsVisibleWrapper;
	ma->Invoke = &CGFxManager::InvokeWrapper;
	ma->InvokeArg = &CGFxManager::InvokeArgWrapper;
	ma->InvokeArray = &CGFxManager::InvokeArrayWrapper;
	ma->CallBack = &CGFxManager::CallBackWrapper;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		GFxSystem::Init();
	}
	if(ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		GFxSystem::Destroy();
	}
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}