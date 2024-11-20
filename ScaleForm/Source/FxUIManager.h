#pragma once
#include "GSysFile.h"
#include "GFxPlayer.h"
#include "GRendererOGL.h"
#include "FxUIBase.h"
#include "FontConfigParser.h"

#include "InterfaceNotice.h"

static struct
{
	WPARAM winKey;
	GFxKey::Code appKey;
}

KeyCodeMap[] =
{
	{VK_BACK,       GFxKey::Backspace},
	{VK_TAB,        GFxKey::Tab},
	{VK_CLEAR,      GFxKey::Clear},
	{VK_RETURN,     GFxKey::Return},
	{VK_SHIFT,      GFxKey::Shift},
	{VK_CONTROL,    GFxKey::Control},
	{VK_MENU,       GFxKey::Alt},
	{VK_PAUSE,      GFxKey::Pause},
	{VK_CAPITAL,    GFxKey::CapsLock},
	{VK_ESCAPE,     GFxKey::Escape},
	{VK_SPACE,      GFxKey::Space},
	{VK_PRIOR,      GFxKey::PageUp},
	{VK_NEXT,       GFxKey::PageDown},
	{VK_END,        GFxKey::End},
	{VK_HOME,       GFxKey::Home},
	{VK_LEFT,       GFxKey::Left},
	{VK_UP,         GFxKey::Up},
	{VK_RIGHT,      GFxKey::Right},
	{VK_DOWN,       GFxKey::Down},
	{VK_INSERT,     GFxKey::Insert},
	{VK_DELETE,     GFxKey::Delete},
	{VK_HELP,       GFxKey::Help},
	{VK_NUMLOCK,    GFxKey::NumLock},
	{VK_SCROLL,     GFxKey::ScrollLock},
	{VK_OEM_1,      GFxKey::Semicolon},
	{VK_OEM_PLUS,   GFxKey::Equal},
	{VK_OEM_COMMA,  GFxKey::Comma},
	{VK_OEM_MINUS,  GFxKey::Minus},
	{VK_OEM_PERIOD, GFxKey::Period},
	{VK_OEM_2,      GFxKey::Slash},
	{VK_OEM_3,      GFxKey::Bar},
	{VK_OEM_4,      GFxKey::BracketLeft},
	{VK_OEM_5,      GFxKey::Backslash},
	{VK_OEM_6,      GFxKey::BracketRight},
	{VK_OEM_7,      GFxKey::Quote}
};

enum KeyModifiers
{
	KM_Control = 0x1,
	KM_Alt     = 0x2,
	KM_Shift   = 0x4,
	KM_Num     = 0x8,
	KM_Caps    = 0x10,
	KM_Scroll  = 0x20
};

BoostSmart_Ptr(CFxUIObj);
typedef std::map<int, CFxUIObjPtr > type_map_uibase;
typedef std::vector<CFxUIObjPtr> type_vector_uibase;

class CFxUIManager
{
public:
	CFxUIManager();
	virtual ~CFxUIManager();
	static CFxUIManager* Instance();

	SInt SizeWidth;
	SInt SizeHeight;
private:
	CInterfaceNotice mcHudNotice;
	CInterfacePlayer mcHudPlayer;
	CInterfaceItemDrop mcHudItemDrop;
public:
	void Create();
	void Release();

	bool Initialize();
	bool OpenGLWin32App();
	bool AddObj(DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, GFxMovieView::AlignType AlignType);
	void AddObj(DWORD index, CFxUIObjPtr pUIObj);
	void RemoveUIObj(DWORD index);
	void RemoveAllUIObjs();
	void ReleaseAllUIObj();

	CFxUIObjPtr FindUIObj(DWORD key);

	bool Update();
	bool Render();
	bool Render(DWORD dwKey);
	bool Update(DWORD dwKey);
	bool UpdateMouseEvent(DWORD dwKey, int Event);

	void HideAll();
	void Show(DWORD dwKey);
	void Hide(DWORD dwKey);
	void Toggle(DWORD dwKey);
	bool IsVisible(DWORD dwKey);
	bool Invoke(DWORD index, const char* pmethodName, const char* pargFmt, ...);
	bool Invoke(DWORD index, const char* pmethodName, const char* pargFmt, va_list args);
	bool InvokeArg(DWORD index, const char* pmethodName, CEICommandContainer& args);
	bool InvokeArray(DWORD index, const char* pmethodName, CEICommandContainer* Array, int Size);

	void OnCreateDevice(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0);
	void OnResetDevice(HWND hWnd);
	void OnLostDevice();
	void OnDestroyDevice();

	bool CallBack(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *pbNoFurtherProcessing);
	//uiSelection
	void SetUISelect(int _iUISelection) { m_iUISelection = _iUISelection; }
	int GetUISelect() { return m_iUISelection; }



	// Funciones estáticas envolventes
	static bool AddObjWrapper(DWORD index, CInternalmethod* pcommand, CExternalmethod* method, const char* FileName, int aligntype);
	static void CreateDevice(DWORD iWindowWidth, DWORD iWindowHeight);
	static void ReleaseDevice();
	static bool UpdateWrapper();
	static bool RenderWrapper();
	static bool RenderWithKeyWrapper(DWORD dwKey);
	static bool UpdateWithKeyWrapper(DWORD dwKey);
	static bool UpdateMouseEventWrapper(DWORD dwKey, int Event);
	static void HideAllWrapper();
	static void ShowWrapper(DWORD dwKey);
	static void HideWrapper(DWORD dwKey);
	static void ToggleWrapper(DWORD dwKey);
	static bool IsVisibleWrapper(DWORD dwKey);
	static bool InvokeWrapper(DWORD index, const char* pmethodName, const char* pargFmt, va_list args);
	static bool InvokeArgWrapper(DWORD index, const char* pmethodName, CEICommandContainer& args);
	static bool InvokeArrayWrapper(DWORD index, const char* pmethodName, CEICommandContainer* args, int Size);
	static void CallBackWrapper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	FontConfig* GetCurrentFontConfig();
	bool GetLanguage(int LanguageId, char* LanguageName);
	bool LoadFontConfig(const GString& fontConfigFilePath);
	int GetFontConfigIndexByName(const char* pname);
	void LoadFontConfigs(ConfigParser* conf);
	bool LoadDefaultFontConfigFromPath( const GString& path );
	int            m_iNowSceneFlag;
	bool           m_isCompletedLoad;
	int            m_iUISelection;
	bool           m_bNoFontConfig;
	FontConfigSet  FontConfigs;
	SInt           FontConfigIndex;

	int m_BackResolutionX;
	int m_BackResolutionY;

	GString FontConfigFilePath;


	type_vector_uibase m_vecUI;
	type_map_uibase    m_mapUI;
	char               m_LanguageName[100];
	FontConfig*        pConfig;
public:
	std::string m_AbsolutePath;
};

static int GetModifiers()
{
	int new_mods = 0;
	if (::GetKeyState(VK_SHIFT) & 0x8000)
		new_mods |= KM_Shift;
	if (::GetKeyState(VK_CONTROL) & 0x8000)
		new_mods |= KM_Control;
	if (::GetKeyState(VK_MENU) & 0x8000)
		new_mods |= KM_Alt;
	if (::GetKeyState(VK_NUMLOCK) & 0x1)
		new_mods |= KM_Num;
	if (::GetKeyState(VK_CAPITAL) & 0x1)
		new_mods |= KM_Caps;
	if (::GetKeyState(VK_SCROLL) & 0x1)
		new_mods |= KM_Scroll;
	return new_mods;
}

inline void KeyEvent(GFxMovieView *pContainerMovie, GFxKey::Code keyCode, unsigned char asciiCode, unsigned int wcharCode, unsigned int mods, bool down)
{
	if(pContainerMovie == NULL) return;

	if (keyCode != GFxKey::VoidSymbol)
	{
		GFxKeyEvent keyEvent(down ? GFxEvent::KeyDown : GFxKeyEvent::KeyUp, keyCode, asciiCode, wcharCode);
		keyEvent.SpecialKeysState.SetShiftPressed(mods & KM_Shift ? 1 : 0);
		keyEvent.SpecialKeysState.SetCtrlPressed(mods & KM_Control ? 1 : 0);
		keyEvent.SpecialKeysState.SetAltPressed(mods & KM_Alt ? 1 : 0);
		keyEvent.SpecialKeysState.SetNumToggled(mods & KM_Num ? 1 : 0);
		keyEvent.SpecialKeysState.SetCapsToggled(mods & KM_Caps ? 1 : 0);
		keyEvent.SpecialKeysState.SetScrollToggled(mods & KM_Scroll ? 1 : 0);
		pContainerMovie->HandleEvent(keyEvent);
	}
}

inline void OnKey(GFxMovieView *pContainerMovie, GFxKey::Code keyCode, unsigned char asciiCode, unsigned int wcharCode, unsigned int mods, bool downFlag)
{
	if(pContainerMovie == NULL) return;

	KeyEvent(pContainerMovie, (GFxKey::Code)keyCode, asciiCode, wcharCode, mods, downFlag);
}

inline void ProcessKeyEvent(GFxMovieView *pContainerMovie, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(pContainerMovie == NULL) return;

	if(uMsg == WM_CHAR)
	{
		UInt32 wcharCode = (UInt32)wParam;
		GFxCharEvent charEvent(wcharCode);
		pContainerMovie->HandleEvent(charEvent);
		return;
	}

	GFxKey::Code kc = GFxKey::VoidSymbol;
	if (wParam >= 'A' && wParam <= 'Z')
	{
		kc = (GFxKey::Code) ((wParam - 'A') + GFxKey::A);
	}
	else if (wParam >= VK_F1 && wParam <= VK_F15)
	{
		kc = (GFxKey::Code) ((wParam - VK_F1) + GFxKey::F1);
	}
	else if (wParam >= '0' && wParam <= '9')
	{
		kc = (GFxKey::Code) ((wParam - '0') + GFxKey::Num0);
	}
	else if (wParam >= VK_NUMPAD0 && wParam <= VK_DIVIDE)
	{
		kc = (GFxKey::Code) ((wParam - VK_NUMPAD0) + GFxKey::KP_0);
	}
	else
	{
		for (int i = 0; KeyCodeMap[i].winKey != 0; i++)
		{
			if (wParam == (UInt)KeyCodeMap[i].winKey)
			{
				kc = KeyCodeMap[i].appKey;
				break;
			}
		}
	}
	unsigned char asciiCode = 0;
	if (kc != GFxKey::VoidSymbol)
	{
		// get the ASCII code, if possible.
		UINT uScanCode = ((UINT)lParam >> 16) & 0xFF; // fetch the scancode
		BYTE ks[256];
		WORD charCode;

		// Get the current keyboard state
		::GetKeyboardState(ks);

		if (::ToAscii((UINT)wParam, uScanCode, ks, &charCode, 0) > 0)
		{
			asciiCode = LOBYTE (charCode);
		}
	}
	bool downFlag = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) ? 1 : 0;
	OnKey(pContainerMovie, kc, asciiCode, 0, GetModifiers(), downFlag);
}

#define GMScaleForm			(CFxUIManager::Instance())