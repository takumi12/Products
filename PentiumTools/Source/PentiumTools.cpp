// PentiumTools.cpp : Define el punto de entrada de la aplicación.
//

#include "framework.h"
#include "PentiumTools.h"
#include "ImageConvert.h"
#include "CGMLibrayMng.h"
#include "CGMFileSystem.h"
#include "CGMFilesOBB.h"
#include "CGMActionLog.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

#include "CGFxEIHandler.h"

#define MAX_LOADSTRING 100

// Variables globales:
HWND g_hWnd;
HINSTANCE hInst;                                // instancia actual
CHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
CHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal

// Declaraciones de funciones adelantadas incluidas en este módulo de código:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

struct WGL_WindowData { HDC hDC; };
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int WindowWidth;
static int WindowHeight;
int SettingInterface = 0;

bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Colocar código aquí.

	// Inicializar cadenas globales
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PENTIUMTOOLS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Realizar la inicialización de la aplicación:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PENTIUMTOOLS));

	MSG msg;

	//GFxEIHandler->DinamicLibrary();
	// Bucle principal de mensajes:
	GMLibrayMng->Initialize();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		WindowWidth = (int)ImGui::GetIO().DisplaySize.x;

		WindowHeight = (int)ImGui::GetIO().DisplaySize.y;

		//GFxEIHandler->OnLoad(WindowWidth, WindowHeight);

		ImGui_ImplOpenGL3_NewFrame();

		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		switch (SettingInterface)
		{
		case 1:
			RenderImageConvert();
			break;
		case 2:
			RenderInterfaceMaps();
			break;
		default:
			GMActionLog->RenderLog();
			break;
		}
		//ImGui::ShowDemoWindow();

		ImGui::Render();

		glViewport(0, 0, WindowWidth, WindowHeight);

		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		//GFxEIHandler->OnRender();

		::SwapBuffers(g_MainWindow.hDC);
	}

	ImGui_ImplOpenGL3_Shutdown();

	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	CleanupDeviceWGL(g_hWnd, &g_MainWindow);

	wglDeleteContext(g_hRC);

	return (int)msg.wParam;
}

//
//  FUNCIÓN: MyRegisterClass()
//
//  PROPÓSITO: Registra la clase de ventana.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXA wcex;

	wcex.cbSize = sizeof(WNDCLASSEXA);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PENTIUMTOOLS));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_PENTIUMTOOLS);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

// FUNCIÓN: InitInstance(HINSTANCE, int)
//
// PROPÓSITO: Guarda el identificador de instancia y crea la ventana principal
//
// COMENTARIOS:
//
// En esta función, se guarda el identificador de instancia en una variable común y
// se crea y muestra la ventana principal del programa.
//

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Almacenar identificador de instancia en una variable global

	g_hWnd = CreateWindow(szWindowClass, "PentiumTools v2.8 ImGui", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd)
	{
		return FALSE;
	}

	if (!CreateDeviceWGL(g_hWnd, &g_MainWindow))
	{
		CleanupDeviceWGL(g_hWnd, &g_MainWindow);
		::DestroyWindow(g_hWnd);
		//::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return FALSE;
	}

	wglMakeCurrent(g_MainWindow.hDC, g_hRC);

	ShowWindow(g_hWnd, nCmdShow);

	UpdateWindow(g_hWnd);

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsLight();

	ImGui_ImplWin32_InitForOpenGL(g_hWnd);

	ImGui_ImplOpenGL3_Init();

	HMENU hMainMenu = GetMenu(g_hWnd);

	if (hMainMenu != NULL)
	{
		EnableMenuItem(hMainMenu, ID_ARCHIVO_ABRIR, MF_BYCOMMAND | MF_GRAYED);

		//EnableMenuItem(hMainMenu, ID_MAPSFILES_32775, MF_BYCOMMAND | MF_GRAYED);

		//EnableMenuItem(hMainMenu, ID_MAPSFILES_32776, MF_BYCOMMAND | MF_GRAYED);

		EnableMenuItem(hMainMenu, ID_GRAPHICFILES_3DBMD, MF_BYCOMMAND | MF_GRAYED);
	}

	return TRUE;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//  FUNCIÓN: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PROPÓSITO: Procesa mensajes de la ventana principal.
//
//  WM_COMMAND  - procesar el menú de aplicaciones
//  WM_PAINT    - Pintar la ventana principal
//  WM_DESTROY  - publicar un mensaje de salida y volver
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Analizar las selecciones de menú:
		switch (wmId)
		{
		case IDM_IMAGENCONVERT:
			{
				if(SettingInterface == 0)
				{
					SettingInterface = 1;
					GraphicImage = true;
					HMENU hMainMenu = GetMenu(g_hWnd);
					if (hMainMenu != NULL)
					{
						EnableMenuItem(hMainMenu, ID_ARCHIVO_ABRIR, MF_BYCOMMAND | MF_ENABLED);
					}
				}
			}
			break;
		case ID_ARCHIVO_ABRIR:
			if (SettingInterface == 1)
			{
				GMFileSystem->OpenNewDirectory();
			}
			else if (SettingInterface == 2)
			{
				GMFilesMap->OpenFileObject();
			}
			break;
		case ID_MAPSFILES_32775:
			GMFilesMap->OpenFileATT();
			break;
		case ID_MAPSFILES_32776:
			GMFilesMap->OpenFileMap();
			break;
		case ID_MAPSFILES_32778:
			GMFilesMap->OpenTerrainHeight();
			break;
		case ID_MAPSFILES_32777:
			{
				if(SettingInterface == 0)
				{
					SettingInterface = 2;
					WIN_OBJECT = true;
					HMENU hMainMenu = GetMenu(g_hWnd);
					if (hMainMenu != NULL)
					{
						EnableMenuItem(hMainMenu, ID_ARCHIVO_ABRIR, MF_BYCOMMAND | MF_ENABLED);
					}
				}
			}
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Agregar cualquier código de dibujo que use hDC aquí...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Controlador de mensajes del cuadro Acerca de.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	HDC hDc = ::GetDC(hWnd);
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	const int pf = ::ChoosePixelFormat(hDc, &pfd);
	if (pf == 0)
		return false;
	if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
		return false;
	::ReleaseDC(hWnd, hDc);

	data->hDC = ::GetDC(hWnd);
	if (!g_hRC)
		g_hRC = wglCreateContext(data->hDC);
	return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	wglMakeCurrent(nullptr, nullptr);
	::ReleaseDC(hWnd, data->hDC);
}
