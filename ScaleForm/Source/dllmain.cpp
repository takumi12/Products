// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"
#include "Util.h"
#include "FxUIManager.h"

BoostSmart_Ptr(CFxUIManager);

class CSynchronize
{
public:
	// Punteros a funciones de CFxUIManager
	bool (*AddObj)(DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, int aligntype);
	void (*CreateDevice)(DWORD, DWORD);
	void (*ReleaseDevice)();

	bool (*Update)();
	bool (*Render)();
	bool (*RenderWithKey)(DWORD);
	bool (*UpdateWithKey)(DWORD);
	bool (*UpdateMouseEvent)(DWORD, int);

	void (*HideAll)();
	void (*Show)(DWORD);
	void (*Hide)(DWORD);
	void (*Toggle)(DWORD);
	bool (*IsVisible)(DWORD);
	bool (*Invoke)(DWORD, const char*, const char*, va_list);
	bool (*InvokeArg)(DWORD, const char*, CEICommandContainer&);
	bool (*InvokeArray)(DWORD, const char*, CEICommandContainer*, int);
	void (*CallBack)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

extern "C" _declspec(dllexport) void SynchronizeInit(CSynchronize*& ma)
{
	GFxSystem::Init();

	ma = new CSynchronize;
	ma->AddObj = &CFxUIManager::AddObjWrapper;
	ma->CreateDevice = &CFxUIManager::CreateDevice;
	ma->ReleaseDevice = &CFxUIManager::ReleaseDevice;
	ma->Update = &CFxUIManager::UpdateWrapper;
	ma->Render = &CFxUIManager::RenderWrapper;
	ma->RenderWithKey = &CFxUIManager::RenderWithKeyWrapper;
	ma->UpdateWithKey = &CFxUIManager::UpdateWithKeyWrapper;
	ma->UpdateMouseEvent = &CFxUIManager::UpdateMouseEventWrapper;
	ma->HideAll = &CFxUIManager::HideAllWrapper;
	ma->Show = &CFxUIManager::ShowWrapper;
	ma->Hide = &CFxUIManager::HideWrapper;
	ma->Toggle = &CFxUIManager::ToggleWrapper;
	ma->IsVisible = &CFxUIManager::IsVisibleWrapper;
	ma->Invoke = &CFxUIManager::InvokeWrapper;
	ma->InvokeArg = &CFxUIManager::InvokeArgWrapper;
	ma->InvokeArray = &CFxUIManager::InvokeArrayWrapper;
	ma->CallBack = &CFxUIManager::CallBackWrapper;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		pLog;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}