///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <locale.h>
#include <zmouse.h>
#include "UIWindows.h"
#include "UIManager.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzOpenData.h"
#include "ZzzScene.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "zzzLodTerrain.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"
#include "Resource.h"
#include <imm.h>
#include "zzzpath.h"
#include "Nprotect.h"
#include "Local.h"
#include "PersonalShopTitleImp.h"
#include "./Utilities/Log/ErrorReport.h"
#include "UIMapName.h"		// rozy
#include "./ExternalObject/leaf/ExceptionHandler.h"
#include "./Utilities/Dump/CrashReporter.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include "ProtocolSend.h"
#include "ProtectSysKey.h"

#include "CBTMessageBox.h"
#include "./ExternalObject/leaf/regkey.h"

#include "CSChaosCastle.h"
#include "GMHellas.h"
#include <io.h>
#include "Input.h"
#include "./Time/Timer.h"
#include "UIMng.h"

#ifdef MOVIE_DIRECTSHOW
#include <dshow.h>
#include "MovieScene.h"
#endif // MOVIE_DIRECTSHOW
#include "GameCensorship.h"
#include "w_MapHeaders.h"
#include "w_PetProcess.h"
#include <ThemidaInclude.h>
#include "MultiLanguage.h"
#include "CGMProtect.h"
#include "CAIController.h"



#include <dbghelp.h>
#pragma comment(lib,"dbghelp.lib")

#include <wzAudio.h>
#pragma comment(lib, "wzAudio.lib")

#ifdef IMPLEMENT_IMGUI_WIN32
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"
#endif // IMPLEMENT_IMGUI_WIN32



CUIMercenaryInputBox* g_pMercenaryInputBox = NULL;
CUITextInputBox* g_pSingleTextInputBox = NULL;
CUITextInputBox* g_pSinglePasswdInputBox = NULL;
int g_iChatInputType = 1;
extern BOOL g_bIMEBlock;

CChatRoomSocketList* g_pChatRoomSocketList = NULL;

CMultiLanguage* pMultiLanguage = NULL;

extern DWORD g_dwTopWindow;

#ifdef MOVIE_DIRECTSHOW
CMovieScene* g_pMovieScene = NULL;
#endif // MOVIE_DIRECTSHOW

CUIManager* g_pUIManager = NULL;
CUIMapName* g_pUIMapName = NULL;		// rozy

int Time_Effect = 0;
bool ashies = false;
int weather = rand() % 3;

HWND      g_hWnd = NULL;
HINSTANCE g_hInst = NULL;
HDC       g_hDC = NULL;
HGLRC     g_hRC = NULL;
HFONT     g_hFont = NULL;
HFONT     g_hFontBold = NULL;
HFONT     g_hFontBig = NULL;
HFONT     g_hFixFont = NULL;

CTimer* g_pTimer = NULL;	// performance counter.
bool      Destroy = false;
bool      ActiveIME = false;

BYTE* RendomMemoryDump;
//CHARACTER* CharacterMemoryDump;

int       RandomTable[100];

char TextMu[] = "mu.exe";

CErrorReport g_ErrorReport;

BOOL g_bMinimizedEnabled = FALSE;
int g_iScreenSaverOldValue = 60 * 15;

extern float g_fScreenRate_x;	// ��
extern float g_fScreenRate_y;



int g_iInactiveTime = 0;
int g_iNoMouseTime = 0;
int g_iInactiveWarning = 0;
bool g_bWndActive = false;
bool HangulDelete = false;
int Hangul = 0;
bool g_bEnterPressed = false;

float g_iMousePopPosition_x = 0;
float g_iMousePopPosition_y = 0;

extern int TimeRemain;
extern bool EnableFastInput;
void MainScene(HDC hDC);

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
BOOL g_bUseWindowMode = TRUE;
#endif

char Mp3FileName[256];

void StopMp3(char* Name, BOOL bEnforce)
{
	if (!m_MusicOnOff && !bEnforce) return;

	if (Mp3FileName[0] != NULL)
	{
		if (strcmp(Name, Mp3FileName) == 0) {
			wzAudioStop();
			Mp3FileName[0] = NULL;
		}
	}
}

void PlayMp3(char* Name, BOOL bEnforce)
{
	if (Destroy) return;
	if (!m_MusicOnOff && !bEnforce) return;

	if (strcmp(Name, Mp3FileName) == 0)
	{
		return;
	}
	else
	{
		wzAudioPlay(Name, 1);
		strcpy(Mp3FileName, Name);
	}
}

bool IsEndMp3()
{
	if (100 == wzAudioGetStreamOffsetRange())
		return true;
	return false;
}

int GetMp3PlayPosition()
{
	return wzAudioGetStreamOffsetRange();
}

extern int  LogIn;
extern char LogInID[];

void CheckHack(void)
{
	SendCheck();
}

GLvoid KillGLWindow(GLvoid)
{
	if (g_hRC)
	{
		if (!wglMakeCurrent(NULL, NULL))
		{
			g_ErrorReport.Write("GL - Release Of DC And RC Failed\r\n");
			MessageBox(NULL, "Release Of DC And RC Failed.", "Error", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(g_hRC))
		{
			g_ErrorReport.Write("GL - Release Rendering Context Failed\r\n");
			MessageBox(NULL, "Release Rendering Context Failed.", "Error", MB_OK | MB_ICONINFORMATION);
		}

		g_hRC = NULL;
	}

	if (g_hDC && !ReleaseDC(g_hWnd, g_hDC))
	{
		g_ErrorReport.Write("GL - OpenGL Release Error\r\n");
		MessageBox(NULL, "OpenGL Release Error.", "Error", MB_OK | MB_ICONINFORMATION);
		g_hDC = NULL;
	}

#if (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
	{
		ChangeDisplaySettings(NULL, 0);
		ShowCursor(TRUE);
	}
#else
#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		ChangeDisplaySettings(NULL, 0);
		ShowCursor(TRUE);
	}
#endif //ENABLE_FULLSCREEN
#endif	//WINDOWMODE(#else)
}


BOOL GetFileNameOfFilePath(char* lpszFile, char* lpszPath)
{
	if (lpszPath != NULL)
	{
		int iFind = (int)'\\';
		char* lpFound = lpszPath;
		char* lpOld = lpFound;
		while (lpFound)
		{
			lpOld = lpFound;
			lpFound = strchr(lpFound + 1, iFind);
		}

		if (strchr(lpszPath, iFind))
		{
			strcpy(lpszFile, lpOld + 1);
		}
		else
		{
			strcpy(lpszFile, lpOld);
		}

		BOOL bCheck = TRUE;
		for (char* lpTemp = lpszFile; bCheck; ++lpTemp)
		{
			switch (*lpTemp)
			{
			case '\"':
			case '\\':
			case '/':
			case ' ':
				*lpTemp = '\0';
			case '\0':
				bCheck = FALSE;
				break;
			}
		}
	}
	return (TRUE);
}

HANDLE g_hMainExe = INVALID_HANDLE_VALUE;

BOOL OpenMainExe(void)
{
#ifdef _DEBUG
	return (TRUE);
#endif
	char lpszFile[MAX_PATH];
	char* lpszCommandLine = GetCommandLine();
	GetFileNameOfFilePath(lpszFile, lpszCommandLine);

	g_hMainExe = CreateFile((char*)lpszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	return (INVALID_HANDLE_VALUE != g_hMainExe);
}

void CloseMainExe(void)
{
	CloseHandle(g_hMainExe);
}

WORD DecryptCheckSumKey(WORD wSource)
{
	WORD wAcc = wSource ^ 0xB479;
	return ((wAcc >> 10) << 4) | (wAcc & 0xF);
}

DWORD GenerateCheckSum(BYTE* pbyBuffer, DWORD dwSize, WORD wKey)
{
	DWORD dwKey = (DWORD)wKey;
	DWORD dwResult = dwKey << 9;
	for (DWORD dwChecked = 0; dwChecked <= dwSize - 4; dwChecked += 4)
	{
		DWORD dwTemp;
		memcpy(&dwTemp, pbyBuffer + dwChecked, sizeof(DWORD));

		switch ((dwChecked / 4 + wKey) % 3)
		{
		case 0:
			dwResult ^= dwTemp;
			break;
		case 1:
			dwResult += dwTemp;
			break;
		case 2:
			dwResult <<= (dwTemp % 11);
			dwResult ^= dwTemp;
			break;
		}

		if (0 == (dwChecked % 4))
		{
			dwResult ^= ((dwKey + dwResult) >> ((dwChecked / 4) % 16 + 3));
		}
	}

	return (dwResult);
}

DWORD GetCheckSum(WORD wKey)
{
	wKey = DecryptCheckSumKey(wKey);

	char lpszFile[MAX_PATH];

	strcpy(lpszFile, "data\\local\\Gameguard.csr");

	HANDLE hFile = CreateFile((char*)lpszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return (0);
	}

	DWORD dwSize = GetFileSize(hFile, NULL);
	BYTE* pbyBuffer = new BYTE[dwSize];
	DWORD dwNumber;
	if (ReadFile(hFile, pbyBuffer, dwSize, &dwNumber, 0))
	{
		CloseHandle(hFile);
	}

	DWORD dwCheckSum = GenerateCheckSum(pbyBuffer, dwSize, wKey);
	delete[] pbyBuffer;

	return (dwCheckSum);
}

BOOL GetFileVersion(char* lpszFileName, WORD* pwVersion)
{
	DWORD dwHandle = 1;
	DWORD dwLen = GetFileVersionInfoSize(lpszFileName, &dwHandle);

	if (dwLen == 0 || dwHandle == 0)
	{
		return (FALSE);
	}

	BYTE* pbyData = new BYTE[dwLen];
	if (GetFileVersionInfo(lpszFileName, dwHandle, dwLen, pbyData) == false)
	{
		delete[] pbyData;
		return (FALSE);
	}

	VS_FIXEDFILEINFO* pffi;
	UINT uLen;
	if (!VerQueryValue(pbyData, "\\", (LPVOID*)&pffi, &uLen))
	{
		delete[] pbyData;
		return (FALSE);
	}

	pwVersion[0] = HIWORD(pffi->dwFileVersionMS);
	pwVersion[1] = LOWORD(pffi->dwFileVersionMS);
	pwVersion[2] = HIWORD(pffi->dwFileVersionLS);
	pwVersion[3] = LOWORD(pffi->dwFileVersionLS);

	delete[] pbyData;
	return (TRUE);
}

extern PATH* path;

void DestroyWindow()
{
	//. save volume level
	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");
	regkey.WriteDword("VolumeLevel", g_pOption->GetVolumeLevel());

	CUIMng::Instance().Release();

#ifdef MOVIE_DIRECTSHOW
	if (g_pMovieScene)
	{
		g_pMovieScene->Destroy();
	}
#endif // MOVIE_DIRECTSHOW

	//. release font handle
	if (g_hFont)
		DeleteObject((HGDIOBJ)g_hFont);

	if (g_hFontBold)
		DeleteObject((HGDIOBJ)g_hFontBold);

	if (g_hFontBig)
		DeleteObject((HGDIOBJ)g_hFontBig);

	ReleaseCharacters();

	if (g_hFixFont)
		DeleteObject((HGDIOBJ)g_hFixFont);

	SAFE_DELETE_ARRAY(GateAttribute);

	SAFE_DELETE_ARRAY(SkillAttribute);

	SAFE_DELETE(CharacterMachine);

	DeleteWaterTerrain();

#ifdef MOVIE_DIRECTSHOW
	if (SceneFlag != MOVIE_SCENE)
#endif // MOVIE_DIRECTSHOW
	{
		gMapManager.DeleteObjects();

		// Object.
		for (int i = MODEL_LOGO; i < MAX_MODELS; i++)
		{
			gmClientModels->GetModel(i)->Release();
		}
		// Bitmap
		Bitmaps.UnloadAllImages();
	}

	SAFE_DELETE_ARRAY(RendomMemoryDump);

#ifdef DYNAMIC_FRUSTRUM
	DeleteAllFrustrum();
#endif //DYNAMIC_FRUSTRUM

	SAFE_DELETE(g_pMercenaryInputBox);
	SAFE_DELETE(g_pSingleTextInputBox);
	SAFE_DELETE(g_pSinglePasswdInputBox);

	SAFE_DELETE(g_pChatRoomSocketList);
	SAFE_DELETE(g_pUIMapName);	// rozy
	SAFE_DELETE(g_pTimer);
	SAFE_DELETE(g_pUIManager);

#ifdef MOVIE_DIRECTSHOW
	SAFE_DELETE(g_pMovieScene);
#endif // MOVIE_DIRECTSHOW

	SAFE_DELETE(pMultiLanguage);
	BoostRest(g_BuffSystem);
	BoostRest(g_MapProcess);
	BoostRest(g_petProcess);

	g_ErrorReport.Write("Destroy");

	HWND shWnd = FindWindow(NULL, "MuPlayer");
	if (shWnd)
		SendMessage(shWnd, WM_DESTROY, 0, 0);
}

void DestroySound()
{
	for (int i = 0; i < MAX_BUFFER; i++)
		ReleaseBuffer(i);

	FreeDirectSound();
	wzAudioDestroy();
}

#ifdef IMPLEMENT_IMGUI_WIN32
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // IMPLEMENT_IMGUI_WIN32

#include "ConnectVersionHex.h"

#include <shellapi.h>
#include <ShlObj.h>

bool CreateFileDialog(std::string& FileLink, LPCWSTR pszTitle, REFCLSID rclsid, COMDLG_FILTERSPEC fileType, bool option, FILEOPENDIALOGOPTIONS fos)
{
	bool success = false;

	IFileDialog* pFileDialog;

	if (SUCCEEDED(CoCreateInstance(rclsid/*CLSID_FileOpenDialog*/, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog))))
	{
		pFileDialog->SetTitle(pszTitle);
		pFileDialog->SetFileTypes(1, &fileType);

		if (option)
			pFileDialog->SetOptions(fos);

		if (SUCCEEDED(pFileDialog->Show(NULL)))
		{
			IShellItem* pItem;
			if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
			{
				PWSTR pszFolderPath;
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath)))
				{
					char ansiPath[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, pszFolderPath, -1, ansiPath, MAX_PATH, NULL, NULL);

					FileLink = ansiPath;

					success = true;

					CoTaskMemFree(pszFolderPath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}

	return success;
}

void OpenFileBMDREAD()
{
	std::string filename;
	COMDLG_FILTERSPEC fileType = { L"Allowed Files", L"*.BMD" };

	if (CreateFileDialog(filename, L"Select a File", CLSID_FileOpenDialog, fileType, false, FOS_OVERWRITEPROMPT))
	{
		//CreateTableObj(filename.c_str());
	}
}

LONG FAR PASCAL WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef IMPLEMENT_IMGUI_WIN32
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
#endif // IMPLEMENT_IMGUI_WIN32

	int wmId = LOWORD(wParam);

	switch (msg)
	{
	case WM_COMMAND:
	{
		switch (wmId)
		{
		case ID_FILES_ABRIR:
			OpenFileBMDREAD();
			break;
		}
	}
	case WM_SYSKEYDOWN:
	{
		return 0;
	}
	break;
#if defined PROTECT_SYSTEMKEY && defined NDEBUG
#ifndef FOR_WORK
	case WM_SYSCOMMAND:
	{
		if (wParam == SC_KEYMENU || wParam == SC_SCREENSAVE)
		{
			return 0;
		}
	}
	break;
#endif // !FOR_WORK
#endif // PROTECT_SYSTEMKEY && NDEBUG
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			g_bWndActive = false;

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
			if (g_bUseWindowMode == TRUE)
			{
				MouseLButton = false;
				MouseLButtonPop = false;
				MouseRButton = false;
				MouseRButtonPop = false;
				MouseRButtonPush = false;
				MouseLButtonDBClick = false;
				MouseMButton = false;
				MouseMButtonPop = false;
				MouseMButtonPush = false;
				MouseWheel = false;
			}
#endif
		}
		else
		{
			g_bWndActive = true;
		}
		break;
	case WM_TIMER:
		//MessageBox(NULL,GlobalText[16],"Error",MB_OK);
		switch (wParam)
		{
		case HACK_TIMER:
			// PKD_ADD_BINARY_PROTECTION
			VM_START
				CheckHack();
			VM_END
				break;
		case WINDOWMINIMIZED_TIMER:
			PostMessage(g_hWnd, WM_CLOSE, 0, 0);
			break;
		case CHATCONNECT_TIMER:
			g_pFriendMenu->SendChatRoomConnectCheck();
			break;
		case SLIDEHELP_TIMER:
			if (g_bWndActive)
			{
				if (g_pSlideHelpMgr)
					g_pSlideHelpMgr->CreateSlideText();
			}
			break;
		}
		break;
	case WM_USER_MEMORYHACK:
		//SetTimer( g_hWnd, WINDOWMINIMIZED_TIMER, 1*1000, NULL);
		KillGLWindow();
		break;
	case WM_NPROTECT_EXIT_TWO:
		SendHackingChecked(0x04, 0);
		SetTimer(g_hWnd, WINDOWMINIMIZED_TIMER, 1 * 1000, NULL);
		MessageBox(NULL, GlobalText[16], "Error", MB_OK);
		break;
	case WM_ASYNCSELECTMSG:
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_CONNECT:
			break;
		case FD_READ:
			SocketClient.nRecv();
			break;
		case FD_WRITE:
			SocketClient.FDWriteSend();
			break;
		case FD_CLOSE:
			g_pChatListBox->AddText("", GlobalText[3], SEASON3B::TYPE_SYSTEM_MESSAGE);
#ifdef CONSOLE_DEBUG
			switch (WSAGETSELECTERROR(lParam))
			{
			case WSAECONNRESET:
				g_ConsoleDebug->Write(MCD_ERROR, "The connection was reset by the remote side.");
				g_ErrorReport.Write("The connection was reset by the remote side.\r\n");
				g_ErrorReport.WriteCurrentTime();
				break;
			case WSAECONNABORTED:
				g_ConsoleDebug->Write(MCD_ERROR, "The connection was terminated due to a time-out or other failure.");
				g_ErrorReport.Write("The connection was terminated due to a time-out or other failure.\r\n");
				g_ErrorReport.WriteCurrentTime();
				break;
			}
#endif // CONSOLE_DEBUG
			SocketClient.Close();
			CUIMng::Instance().PopUpMsgWin(MESSAGE_SERVER_LOST);
			break;
		}
		break;
	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(BLACK_BRUSH);
		break;
	case WM_ERASEBKGND:
		return TRUE;
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
	}
	return 0;
	break;
	case WM_DESTROY:
	{
		Destroy = true;
		SocketClient.Close();
		DestroySound();
		KillGLWindow();
		CloseMainExe();
		PostQuitMessage(0);
	}
	break;
	case WM_SETCURSOR:
		ShowCursor(TRUE);
		//ShowCursor(false);
		break;
#if (defined WINDOWMODE)
	case WM_SIZE:
		if (SIZE_MINIMIZED == wParam && g_bUseWindowMode == FALSE && !(g_bMinimizedEnabled))
		{
			DWORD dwMess[SIZE_ENCRYPTION_KEY];
			for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
			{
				dwMess[i] = GetTickCount();
			}
			g_SimpleModulusCS.LoadKeyFromBuffer((BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
		}
		break;
#else
#ifdef NDEBUG
#ifndef FOR_WORK
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	case WM_SIZE:
		if (SIZE_MINIMIZED == wParam
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
			&& g_bUseWindowMode == FALSE
#endif
			)
		{
			if (!(g_bMinimizedEnabled))
			{
				SendHackingChecked(0x05, 0);
				DWORD dwMess[SIZE_ENCRYPTION_KEY];
				for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
				{
					dwMess[i] = GetTickCount();
				}
				g_SimpleModulusCS.LoadKeyFromBuffer((BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
			}
		}
		break;
#endif
#endif
#endif
#endif	//WINDOWMODE(#else)
		//-----------------------------
	default:
		if (msg >= WM_CHATROOMMSG_BEGIN && msg < WM_CHATROOMMSG_END)
			g_pChatRoomSocketList->ProcessSocketMessage(msg - WM_CHATROOMMSG_BEGIN, WSAGETSELECTEVENT(lParam));
		break;
	}

	MouseLButtonDBClick = false;
	if (MouseLButtonPop == true && (g_iMousePopPosition_x != MouseX || g_iMousePopPosition_y != MouseY))
		MouseLButtonPop = false;
	switch (msg)
	{
	case WM_MOUSEMOVE:
	{
		MouseX = (float)LOWORD(lParam) / g_fScreenRate_x;
		MouseY = (float)HIWORD(lParam) / g_fScreenRate_y;
		if (MouseX < 0)
			MouseX = 0;
		if (MouseX > GetWindowsX)
			MouseX = GetWindowsX;
		if (MouseY < 0)
			MouseY = 0;
		if (MouseY > GetWindowsY)
			MouseY = GetWindowsY;
	}
	break;
	case WM_LBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseLButtonPop = false;
		if (!MouseLButton)
			MouseLButtonPush = true;
		MouseLButton = true;
		SetCapture(g_hWnd);
		break;
	case WM_LBUTTONUP:
		g_iNoMouseTime = 0;
		MouseLButtonPush = false;
		MouseLButtonPop = true;
		MouseLButton = false;
		g_iMousePopPosition_x = MouseX;
		g_iMousePopPosition_y = MouseY;
		ReleaseCapture();
		break;
	case WM_RBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseRButtonPop = false;
		if (!MouseRButton) MouseRButtonPush = true;
		MouseRButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_RBUTTONUP:
		g_iNoMouseTime = 0;
		MouseRButtonPush = false;
		if (MouseRButton) MouseRButtonPop = true;
		MouseRButton = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
	case WM_LBUTTONDBLCLK:
		g_iNoMouseTime = 0;
		MouseLButtonDBClick = true;
		break;
	case WM_MBUTTONDOWN:
		g_iNoMouseTime = 0;
		MouseMButtonPop = false;
		if (!MouseMButton) MouseMButtonPush = true;
		MouseMButton = true;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		SetCapture(g_hWnd);
#endif
		break;
	case WM_MBUTTONUP:
		g_iNoMouseTime = 0;
		MouseMButtonPush = false;
		if (MouseMButton) MouseMButtonPop = true;
		MouseRButton = false;
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		ReleaseCapture();
#endif
		break;
	case WM_MOUSEWHEEL:
	{
		MouseWheel = (short)HIWORD(wParam) / WHEEL_DELTA;
	}
	break;
	case WM_IME_NOTIFY:
	{
		if (g_iChatInputType == 1)
		{
			switch (wParam)
			{
			case IMN_SETCONVERSIONMODE:
				if (GetFocus() == g_hWnd)
				{
					CheckTextInputBoxIME(IME_CONVERSIONMODE);
				}
				break;
			case IMN_SETSENTENCEMODE:
				if (GetFocus() == g_hWnd)
				{
					CheckTextInputBoxIME(IME_SENTENCEMODE);
				}
				break;
			default:
				break;
			}
		}
	}
	break;
	case WM_CHAR:
	{
		switch (wParam)
		{
		case VK_RETURN:
		{
			SetEnterPressed(true);
		}
		break;
		}
	}
	break;
	}

	if (g_BuffSystem) {
		LRESULT result;
		TheBuffStateSystem().HandleWindowMessage(msg, wParam, lParam, result);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool CreateOpenglWindow()
{
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 16;

	if (!(g_hDC = GetDC(g_hWnd)))
	{
		g_ErrorReport.Write("OpenGL Get DC Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL Get DC Error.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	GLuint PixelFormat;

	if (!(PixelFormat = ChoosePixelFormat(g_hDC, &pfd)))
	{
		g_ErrorReport.Write("OpenGL Choose Pixel Format Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL Choose Pixel Format Error.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!DescribePixelFormat(g_hDC, PixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
	{
		g_ErrorReport.Write("OpenGL Describe Pixel Format Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL Describe Pixel Format Error.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!SetPixelFormat(g_hDC, PixelFormat, &pfd))
	{
		g_ErrorReport.Write("OpenGL Set Pixel Format Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL Set Pixel Format Error.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(g_hRC = wglCreateContext(g_hDC)))
	{
		g_ErrorReport.Write("OpenGL Create Context Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL Create Context Error.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!wglMakeCurrent(g_hDC, g_hRC))
	{
		g_ErrorReport.Write("OpenGL Make Current Error - ErrorCode : %d\r\n", GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], "OpenGL Make Current Error.", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	//if (wglSwapIntervalEXT)
	//{
	//	wglSwapIntervalEXT(1); // 1 habilita V-Sync, 0 deshabilita V-Sync
	//}

	// Habilitar antialiasing
	glEnable(GL_MULTISAMPLE);

	ShowWindow(g_hWnd, SW_SHOW);
	SetForegroundWindow(g_hWnd);
	SetFocus(g_hWnd);

	return true;
}

#ifdef IMPLEMENT_IMGUI_WIN32
void CreateImGuiWindow()
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	//io.Fonts->AddFontFromFileTTF("segoeui.ttf", 16, NULL, io.Fonts->GetGlyphRangesCyrillic());

	ImGui::StyleColorsDark();

	// Inicializaci�n de los backends de ImGui
	ImGui_ImplWin32_Init(g_hWnd);

	ImGui_ImplOpenGL2_Init();
}

void DestroyImGuiWindow()
{
	ImGui_ImplOpenGL2_Shutdown();

	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();
}
#endif // IMPLEMENT_IMGUI_WIN32

char m_ID[11];
char m_Version[11];
char m_ExeVersion[11];
int  m_SoundOnOff;
int  m_iFontSize;
int  m_MusicOnOff;
int  m_Resolution;
int	m_nColorDepth;
int	g_iRenderTextType = 0;

char g_aszMLSelection[MAX_LANGUAGE_NAME_LENGTH] = { '\0' };
std::string g_strSelectedML = "";

HWND StartWindow(HINSTANCE hCurrentInst, int nCmdShow)
{
	char* windowName = "MU";

	WNDCLASS wndClass;
	HWND hWnd;

	wndClass.style = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hCurrentInst;
	wndClass.hIcon = LoadIcon(hCurrentInst, (LPCTSTR)IDI_ICON1);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wndClass.lpszClassName = GMProtect->GetWinName();
	RegisterClass(&wndClass);

	RECT rc = { 0, 0, WindowWidth, WindowHeight };

	if (g_bUseWindowMode == TRUE)
	{
		AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN, NULL);
	}
	else
	{
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, NULL);
	}
	rc.right -= rc.left;
	rc.bottom -= rc.top;

	if (g_bUseWindowMode == TRUE)
	{
		hWnd = CreateWindow(
			windowName, windowName,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right,
			rc.bottom,
			NULL, NULL, hCurrentInst, NULL);
	}
	else
	{
		hWnd = CreateWindow(
			windowName,                    // Nombre de la clase
			windowName,         // T�tulo de la ventana
			WS_POPUP | WS_VISIBLE,        // Estilo de la ventana (sin bordes, ni t�tulo)
			0, 0,                         // Posici�n de la ventana (top-left corner)
			WindowWidth,
			WindowHeight,    // Tama�o de la ventana (ancho y alto)
			NULL,                         // Handle de la ventana padre
			NULL,                         // Handle del men�
			hCurrentInst,                    // Handle de la instancia
			NULL                          // Par�metros de creaci�n
		);

		/*hWnd = CreateWindow(
			windowName, windowName,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right, rc.bottom, NULL, NULL, hCurrentInst, NULL);*/
	}
	return hWnd;
}

void SetResolution()
{
	if (g_bUseWindowMode == TRUE)
	{
		switch (m_Resolution)
		{
		case 1:
			WindowWidth = 800;
			WindowHeight = 600;
			break;
		case 2:
			WindowWidth = 1024;
			WindowHeight = 768;
			break;
		case 3:
			WindowWidth = 1280;
			WindowHeight = 1024;
			break;
		case 4:
			WindowWidth = 1360;
			WindowHeight = 768;
			break;
		case 5:
			WindowWidth = 1440;
			WindowHeight = 900;
			break;
		case 6:
			WindowWidth = 1600;
			WindowHeight = 900;
			break;
		case 7:
			WindowWidth = 1680;
			WindowHeight = 1050;
			break;
		case 8:
			WindowWidth = 1920;
			WindowHeight = 1080;
			break;
		case 9:
			WindowWidth = 3840;
			WindowHeight = 2160;
			break;
		default:
			WindowWidth = 800;
			WindowHeight = 600;
			break;
		}
	}
	else
	{
		WindowWidth = GetSystemMetrics(SM_CXSCREEN);
		WindowHeight = GetSystemMetrics(SM_CYSCREEN);
	}


	if (gmProtect->ScreenType == 0)
	{
		g_fScreenRate_x = (float)WindowWidth / 640;
		g_fScreenRate_y = (float)WindowHeight / 480;
	}
	else if (gmProtect->ScreenType == 1)
	{
		g_fScreenRate_x = (float)WindowHeight / 480;
		g_fScreenRate_y = (float)WindowHeight / 480;
	}
	else
	{
		if (WindowWidth >= 1920)
		{
			g_fScreenRate_x = 1.6f;
			g_fScreenRate_y = 1.6f;
		}
		else
		{
			g_fScreenRate_x = 1.25f;
			g_fScreenRate_y = 1.25f;
		}
	}

	WinFrameX = (WindowWidth / g_fScreenRate_x);
	WinFrameY = (WindowHeight / g_fScreenRate_y);
}

BOOL OpenInitFile()
{
	char szIniFilePath[256 + 20] = "";
	char szCurrentDir[256];

	GetCurrentDirectory(256, szCurrentDir);

	strcpy(szIniFilePath, szCurrentDir);
	if (szCurrentDir[strlen(szCurrentDir) - 1] == '\\')
		strcat(szIniFilePath, "config.ini");
	else strcat(szIniFilePath, "\\config.ini");

	GetPrivateProfileString("LOGIN", "Version", "", m_Version, 11, szIniFilePath);

	GMProtect->HookComplete();

	char* lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	if (GetFileNameOfFilePath(lpszFile, lpszCommandLine))
	{
		WORD wVersion[4];
		if (GetFileVersion(lpszFile, wVersion))
		{
			sprintf(m_ExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if (wVersion[2] > 0)
			{
				char lpszMinorVersion[3] = "a";
				if (wVersion[2] > 26)
				{
					lpszMinorVersion[0] = 'A';
					lpszMinorVersion[0] += (wVersion[2] - 27);
					lpszMinorVersion[1] = '+';
				}
				else
				{
					lpszMinorVersion[0] += (wVersion[2] - 1);
				}
				strcat(m_ExeVersion, lpszMinorVersion);
			}
		}
		else
		{
			strcpy(m_ExeVersion, m_Version);
		}
	}
	else
	{
		strcpy(m_ExeVersion, m_Version);
	}

	//#ifdef _DEBUG

	m_ID[0] = '\0';
	m_SoundOnOff = 1;
	m_MusicOnOff = 1;
	m_Resolution = 0;
	m_nColorDepth = 0;
	m_iFontSize = 13;

	HKEY hKey;
	DWORD dwDisp;
	DWORD dwSize;
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp))
	{
		dwSize = 11;
		if (RegQueryValueEx(hKey, "ID", 0, NULL, (LPBYTE)m_ID, &dwSize) != ERROR_SUCCESS)
		{
		}
		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "SoundOnOff", 0, NULL, (LPBYTE)&m_SoundOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_SoundOnOff = true;
		}
		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "MusicOnOff", 0, NULL, (LPBYTE)&m_MusicOnOff, &dwSize) != ERROR_SUCCESS)
		{
			m_MusicOnOff = false;
		}
		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "Resolution", 0, NULL, (LPBYTE)&m_Resolution, &dwSize) != ERROR_SUCCESS)
			m_Resolution = 1;

		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "FontSize", 0, NULL, (LPBYTE)&m_iFontSize, &dwSize) != ERROR_SUCCESS)
			m_iFontSize = 13;

		if (RegQueryValueEx(hKey, "ColorDepth", 0, NULL, (LPBYTE)&m_nColorDepth, &dwSize) != ERROR_SUCCESS)
		{
			m_nColorDepth = 0;
		}
		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "TextOut", 0, NULL, (LPBYTE)&g_iRenderTextType, &dwSize) != ERROR_SUCCESS)
		{
			g_iRenderTextType = 0;
		}

		g_iChatInputType = 1;

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
		dwSize = sizeof(int);
		if (RegQueryValueEx(hKey, "WindowMode", 0, NULL, (LPBYTE)&g_bUseWindowMode, &dwSize) != ERROR_SUCCESS)
		{
			g_bUseWindowMode = FALSE;
		}
#endif // USER_WINDOW_MODE

		dwSize = MAX_LANGUAGE_NAME_LENGTH;
		if (RegQueryValueEx(hKey, "LangSelection", 0, NULL, (LPBYTE)g_aszMLSelection, &dwSize) != ERROR_SUCCESS)
		{
			strcpy(g_aszMLSelection, "Eng");
		}
		g_strSelectedML = g_aszMLSelection;
	}
	RegCloseKey(hKey);

	if (0 == m_Resolution)
		m_Resolution = 1;

	SetResolution();

	return TRUE;
}

BOOL Util_CheckOption(char* lpszCommandLine, unsigned char cOption, char* lpszString)
{
	unsigned char cComp[2];
	cComp[0] = cOption; cComp[1] = cOption;
	if (islower((int)cOption))
	{
		cComp[1] = toupper((int)cOption);
	}
	else if (isupper((int)cOption))
	{
		cComp[1] = tolower((int)cOption);
	}

	int nFind = (int)'/';
	unsigned char* lpFound = (unsigned char*)lpszCommandLine;
	while (lpFound)
	{
		lpFound = (unsigned char*)strchr((char*)(lpFound + 1), nFind);
		if (lpFound && (*(lpFound + 1) == cComp[0] || *(lpFound + 1) == cComp[1]))
		{	// �߰�
			if (lpszString)
			{
				int nCount = 0;
				for (unsigned char* lpSeek = lpFound + 2; *lpSeek != ' ' && *lpSeek != '\0'; lpSeek++)
				{
					nCount++;
				}

				memcpy(lpszString, lpFound + 2, nCount);
				lpszString[nCount] = '\0';
			}
			return (TRUE);
		}
	}

	return (FALSE);
}

BOOL UpdateFile(char* lpszOld, char* lpszNew)
{
	SetFileAttributes(lpszOld, FILE_ATTRIBUTE_NORMAL);
	SetFileAttributes(lpszNew, FILE_ATTRIBUTE_NORMAL);

	DWORD dwStartTickCount = ::GetTickCount();
	while (::GetTickCount() - dwStartTickCount < 5000) {
		if (CopyFile(lpszOld, lpszNew, FALSE))
		{	// ����
			DeleteFile(lpszOld);
			return (TRUE);
		}
	}
	g_ErrorReport.Write("%s to %s CopyFile Error : %d\r\n", lpszNew, lpszOld, GetLastError());
	return (FALSE);
}

#include <tlhelp32.h>

BOOL KillExeProcess(char* lpszExe)
{
	HANDLE hProcessSnap = NULL;
	BOOL bRet = FALSE;
	PROCESSENTRY32 pe32 = { 0 };

	//  Take a snapshot of all processes in the system. 

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return (FALSE);

	//  Fill in the size of the structure before using it. 

	pe32.dwSize = sizeof(PROCESSENTRY32);

	//  Walk the snapshot of the processes, and for each process, 
	//  display information. 

	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (stricmp(pe32.szExeFile, lpszExe) == 0)
			{
				HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

				if (process)
				{
					TerminateProcess(process, 0);
				}
			}
		} while (Process32Next(hProcessSnap, &pe32));
		bRet = TRUE;
	}
	else
		bRet = FALSE;    // could not walk the list of processes 

	// Do not forget to clean up the snapshot object. 

	CloseHandle(hProcessSnap);

	return bRet;
}

char g_lpszCmdURL[50];
BOOL GetConnectServerInfo(PSTR szCmdLine, char* lpszURL, WORD* pwPort)
{
	char lpszTemp[256] = { 0, };
	if (Util_CheckOption(szCmdLine, 'y', lpszTemp))
	{
		BYTE bySuffle[] = { 0x0C, 0x07, 0x03, 0x13 };

		for (int i = 0; i < (int)strlen(lpszTemp); i++)
			lpszTemp[i] -= bySuffle[i % 4];
		strcpy(lpszURL, lpszTemp);

		if (Util_CheckOption(szCmdLine, 'z', lpszTemp))
		{
			for (int j = 0; j < (int)strlen(lpszTemp); j++)
				lpszTemp[j] -= bySuffle[j % 4];
			*pwPort = atoi(lpszTemp);
		}
		g_ErrorReport.Write("[Virtual Connection] Connect IP : %s, Port : %d\r\n", lpszURL, *pwPort);
		return (TRUE);
	}
	if (!Util_CheckOption(szCmdLine, 'u', lpszTemp))
	{
		return (FALSE);
	}
	strcpy(lpszURL, lpszTemp);
	if (!Util_CheckOption(szCmdLine, 'p', lpszTemp))
	{
		return (FALSE);
	}
	*pwPort = atoi(lpszTemp);

	return (TRUE);
}


extern int TimeRemain;
BOOL g_bInactiveTimeChecked = FALSE;

bool ExceptionCallback(_EXCEPTION_POINTERS* pExceptionInfo)
{
	char path[MAX_PATH];
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	wsprintf(path, "%d-%d-%d_%dh%dm%ds.dmp", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	HANDLE file = CreateFile(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pExceptionInfo;
		mdei.ClientPointers = FALSE; // Set to TRUE if the callback routines require full-memory minidump.

		if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithIndirectlyReferencedMemory, &mdei, 0, 0))
		{
			CloseHandle(file);
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}

	// Ensure to close the file handle even if the minidump write fails
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void StartAddress(LPVOID lpThreadParameter)
{
	HANDLE v1;
	HANDLE v2;

	while (TRUE)
	{
		Sleep(5000);

		v1 = GetCurrentProcess();
		SetProcessWorkingSetSize(v1, 0xFFFFFFFF, 0xFFFFFFFF);

		v2 = GetCurrentProcess();
		SetThreadPriority(v2, -2);
	}
}

//Nvidia Update
extern "C"
{
	_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

// Force NVidia Optimus to use NVidia GPU on drivers 302 and later.
extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

void EnablePermanentDep()
{
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");

	if (hNtdll == nullptr)
	{
		return;
	}

	lpNtSetInformationProcess NtSetInformationProcess = (lpNtSetInformationProcess)GetProcAddress(hNtdll, "NtSetInformationProcess");
	if (NtSetInformationProcess == nullptr)
	{
		return;
	}

	ULONG ExecuteFlags = 0x02;
	NTSTATUS Status = NtSetInformationProcess(NtCurrentProcess, 0x22, &ExecuteFlags, sizeof(ExecuteFlags));

	if (Status != 0)
	{
		return;
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
	if (GMProtect->ReadMainFile("Data\\av-code45.pak") == false)
	{
		MessageBox(0, "Data\\av-code45.pak not found or invalid!", "Error", MB_OK | MB_ICONERROR);
		//ExitProcess(0);
		return 0;
	}

	SetProcessDPIAware();

	EnablePermanentDep();

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)StartAddress, 0, 0, 0);

	MSG msg = {};

	leaf::AttachExceptionHandler(ExceptionCallback);

	char lpszExeVersion[256] = "unknown";

	char* lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	WORD wVersion[4] = { 0, };
	if (GetFileNameOfFilePath(lpszFile, lpszCommandLine))
	{
		if (GetFileVersion(lpszFile, wVersion))
		{
			sprintf(lpszExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if (wVersion[2] > 0)
			{
				char lpszMinorVersion[2] = "a";
				lpszMinorVersion[0] += (wVersion[2] - 1);
				strcat(lpszExeVersion, lpszMinorVersion);
			}
		}
	}

	g_ErrorReport.Write("\r\n");
	g_ErrorReport.WriteLogBegin();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.Write("Mu online %s (%s) executed. (%d.%d.%d.%d)\r\n", lpszExeVersion, "Eng", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ConsoleDebug->Write(MCD_NORMAL, "Mu Online (Version: %d.%d.%d.%d)", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ErrorReport.WriteCurrentTime();

	ER_SystemInfo si;

	ZeroMemory(&si, sizeof(ER_SystemInfo));
	GetSystemInfo(&si);

	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSystemInfo(&si);
	g_ErrorReport.AddSeparator();

	VM_START
	WORD wPortNumber;
	if (GetConnectServerInfo(szCmdLine, g_lpszCmdURL, &wPortNumber))
	{
		szServerIpAddress = g_lpszCmdURL;
		g_ServerPort = wPortNumber;
	}
	VM_END

	if (!OpenMainExe())
	{
		return false;
	}

	VM_START
	g_SimpleModulusCS.LoadEncryptionKey("Data\\Enc1.dat");
	g_SimpleModulusSC.LoadDecryptionKey("Data\\Dec2.dat");
	VM_END

	g_ErrorReport.Write("> To read config.ini.\r\n");

	if (OpenInitFile() == FALSE)
	{
		g_ErrorReport.Write("config.ini read error\r\n");
		return false;
	}

	pMultiLanguage = new CMultiLanguage(g_strSelectedML);

	//if (g_iChatInputType == 1)
	//	ShowCursor(FALSE);

	g_ErrorReport.Write("> Enum display settings.\r\n");

	DEVMODE DevMode = {};
	DEVMODE* pDevmodes;
	int nModes = 0;

	while (EnumDisplaySettings(NULL, nModes, &DevMode))
		nModes++;

	pDevmodes = new DEVMODE[nModes + 1];
	nModes = 0;

	while (EnumDisplaySettings(NULL, nModes, &pDevmodes[nModes])) nModes++;

	DWORD dwBitsPerPel = 16;
	for (int n1 = 0; n1 < nModes; n1++)
	{
		if (pDevmodes[n1].dmBitsPerPel == 16 && m_nColorDepth == 0)
		{
			dwBitsPerPel = 16; break;
		}
		if (pDevmodes[n1].dmBitsPerPel == 24 && m_nColorDepth == 1)
		{
			dwBitsPerPel = 24; break;
		}
		if (pDevmodes[n1].dmBitsPerPel == 32 && m_nColorDepth == 1)
		{
			dwBitsPerPel = 32; break;
		}
	}

#ifdef ENABLE_FULLSCREEN
#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
#endif	// USER_WINDOW_MODE
	{
		for (int n2 = 0; n2 < nModes; n2++)
		{
			if (pDevmodes[n2].dmPelsWidth == WindowWidth && pDevmodes[n2].dmPelsHeight == WindowHeight && pDevmodes[n2].dmBitsPerPel == dwBitsPerPel)
			{
				g_ErrorReport.Write("> Change display setting %dx%d.\r\n", pDevmodes[n2].dmPelsWidth, pDevmodes[n2].dmPelsHeight);
				ChangeDisplaySettings(&pDevmodes[n2], 0);
				break;
			}
		}
	}
#endif //ENABLE_FULLSCREEN

	delete[] pDevmodes;

	g_ErrorReport.Write("> Screen size = %d x %d.\r\n", WindowWidth, WindowHeight);

	g_hInst = hInstance;
	g_hWnd = StartWindow(hInstance, nCmdShow);
	g_ErrorReport.Write("> Start window success.\r\n");

	if (!CreateOpenglWindow())
	{
		return FALSE;
	}

#ifdef IMPLEMENT_IMGUI_WIN32
	CreateImGuiWindow();
#endif // IMPLEMENT_IMGUI_WIN32

	g_ErrorReport.Write("> OpenGL init success.\r\n");
	g_ErrorReport.AddSeparator();
	//g_ErrorReport.WriteOpenGLInfo();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSoundCardInfo();

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	//g_ErrorReport.WriteImeInfo( g_hWnd);
	g_ErrorReport.AddSeparator();

	int fontheight = m_iFontSize;

	switch (WindowHeight)
	{
	case 480:
		FontHeight = (fontheight - 1);
		break;
	case 600:
		FontHeight = fontheight;
		break;
	case 768:
		FontHeight = fontheight + 1;
		break;
	case 900:
	case 1024:
	case 1050:
		FontHeight = fontheight + 2;
		break;
	default:
		FontHeight = fontheight + 3;
		break;
	}

	int iFontSize = FontHeight - 1;

	g_hFont = CreateFont(iFontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, GlobalText[0][0] ? GlobalText[0] : NULL);

	g_hFontBold = CreateFont(iFontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, GlobalText[0][0] ? GlobalText[0] : NULL);

	g_hFontBig = CreateFont(iFontSize * 2, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, GlobalText[0][0] ? GlobalText[0] : NULL);

	g_hFixFont = CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, GlobalText[18][0] ? GlobalText[18] : NULL);

#ifdef FREETYPE_ENABLE
	gRenderText->CreateFontFamily(iFontSize - 1, 0);
#endif // FREETYPE_ENABLE

	setlocale(LC_ALL, "english");

	CInput::Instance().Create(g_hWnd, WindowWidth, WindowHeight);

	g_pNewUISystem->Create();

	if (m_MusicOnOff)
	{
		wzAudioCreate(g_hWnd);
		wzAudioOption(WZAOPT_STOPBEFOREPLAY, 1);
	}

	if (m_SoundOnOff)
	{
		DWORD value;
		leaf::CRegKey regkey;

		InitDirectSound(g_hWnd);

		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu\\Config");

		if (!regkey.ReadDword("VolumeLevel", value))
		{
			value = 5;	//. default setting
			regkey.WriteDword("VolumeLevel", value);
		}
		if (value < 0 || value >= 10)
			value = 5;

		g_pOption->SetVolumeLevel(int(value));
		SetEffectVolumeLevel(g_pOption->GetVolumeLevel());
	}

	SetTimer(g_hWnd, HACK_TIMER, 20 * 1000, NULL);

	srand((unsigned)time(NULL));
	for (int i = 0; i < 100; i++)
	{
		RandomTable[i] = rand() % 360;
	}

	RendomMemoryDump = new BYTE[rand() % 100 + 1];
	GateAttribute = new GATE_ATTRIBUTE[MAX_GATES];
	SkillAttribute = new SKILL_ATTRIBUTE[MAX_SKILLS];


	static CGMCharacter gcCharacter;
	static CGMModelManager gcModel;

	CharacterMachine = new CHARACTER_MACHINE;

	memset(GateAttribute, 0, sizeof(GATE_ATTRIBUTE) * (MAX_GATES));
	memset(SkillAttribute, 0, sizeof(SKILL_ATTRIBUTE) * (MAX_SKILLS));
	memset(CharacterMachine, 0, sizeof(CHARACTER_MACHINE));

	CharacterAttribute = &CharacterMachine->Character;
	CharacterMachine->Init();

	Hero = gmCharacters->GetCharacter(0);

	if (g_iChatInputType == 1)
	{
		g_pMercenaryInputBox = new CUIMercenaryInputBox;
		g_pSingleTextInputBox = new CUITextInputBox;
		g_pSinglePasswdInputBox = new CUITextInputBox;
	}

	g_pChatRoomSocketList = new CChatRoomSocketList;
	g_pUIManager = new CUIManager;
	g_pUIMapName = new CUIMapName;	// rozy
	g_pTimer = new CTimer();

#ifdef MOVIE_DIRECTSHOW
	g_pMovieScene = new CMovieScene;
#endif // MOVIE_DIRECTSHOW


	static CAIController pAIController(Hero);

	g_BuffSystem = BuffStateSystem::Make();

	g_MapProcess = MapProcess::Make();

	g_petProcess = PetProcess::Make();

	CUIMng::Instance().Create();

	if (g_iChatInputType == 1)
	{
		g_pMercenaryInputBox->Init(g_hWnd);
		g_pSingleTextInputBox->Init(g_hWnd, 200, 20);
		g_pSinglePasswdInputBox->Init(g_hWnd, 200, 20, 9, TRUE);
		g_pSingleTextInputBox->SetState(UISTATE_HIDE);
		g_pSinglePasswdInputBox->SetState(UISTATE_HIDE);

		g_pMercenaryInputBox->SetFont(g_hFont);
		g_pSingleTextInputBox->SetFont(g_hFont);
		g_pSinglePasswdInputBox->SetFont(g_hFont);

		g_bIMEBlock = FALSE;
		HIMC  hIMC = ImmGetContext(g_hWnd);
		ImmSetConversionStatus(hIMC, IME_CMODE_ALPHANUMERIC, IME_SMODE_NONE);
		ImmReleaseContext(g_hWnd, hIMC);
		SaveIMEStatus();
		g_bIMEBlock = TRUE;
	}
#if (defined WINDOWMODE)
	if (g_bUseWindowMode == FALSE)
	{
		int nOldVal;
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300 * 60, NULL, 0);
	}
#else
#ifdef NDEBUG
#ifndef FOR_WORK
#ifdef ACTIVE_FOCUS_OUT
	if (g_bUseWindowMode == FALSE)
	{
#endif	// ACTIVE_FOCUS_OUT
		int nOldVal; // ���� �� �ʿ䰡 ����
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);  // ����Ű�� ������ ��
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);  // ��ũ�����̹� ����
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300 * 60, NULL, 0);  // ��ũ�����̹� ����
#ifdef ACTIVE_FOCUS_OUT
	}
#endif	// ACTIVE_FOCUS_OUT
#endif
#endif
#endif	//WINDOWMODE(#else)

#ifdef SAVE_PACKET
	DeleteFile(PACKET_SAVE_FILE);
#endif

#if defined PROTECT_SYSTEMKEY && defined NDEBUG
#ifndef FOR_WORK
	ProtectSysKey::AttachProtectSysKey(g_hInst, g_hWnd);
#endif // !FOR_WORK
#endif // PROTECT_SYSTEMKEY && NDEBUG

	while (msg.message != 18 && msg.message != 16)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//Scene
#if (defined WINDOWMODE)
			if (g_bUseWindowMode == TRUE || g_bWndActive)
			{
				Scene(g_hDC);
			}
			else if (g_bUseWindowMode == FALSE)
			{
				SetForegroundWindow(g_hWnd);
				SetFocus(g_hWnd);

				if (g_iInactiveWarning < 1)
				{
					g_iInactiveWarning++;
					g_bMinimizedEnabled = TRUE;
					ShowWindow(g_hWnd, SW_MINIMIZE);
					g_bMinimizedEnabled = FALSE;
					ShowWindow(g_hWnd, SW_MAXIMIZE);
				}
				else
				{
					SetTimer(g_hWnd, WINDOWMINIMIZED_TIMER, 1 * 1000, NULL);
					PostMessage(g_hWnd, WM_CLOSE, 0, 0);
				}
			}

#else//WINDOWMODE
			if (g_bWndActive)
				Scene(g_hDC);

#endif	//WINDOWMODE(#else)
		}
		ProtocolCompiler();
		g_pChatRoomSocketList->ProtocolCompile();
	} // while( 1 )

#ifdef IMPLEMENT_IMGUI_WIN32
	DestroyImGuiWindow();
#endif // IMPLEMENT_IMGUI_WIN32

	DestroyWindow();

	return msg.wParam;
}

