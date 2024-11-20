// PentiumTools.cpp : Define el punto de entrada de la aplicación.
//

#include "framework.h"
#include "FinderItem.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "Util.h"
#include "Controls.h"
#include "QueryManager.h"
#include "ItemManager.h"


#define MAX_LOADSTRING 100

// Variables globales:
HWND g_hWnd;
HINSTANCE hInst;                                // instancia actual
CHAR szTitle[MAX_LOADSTRING];                  // Texto de la barra de título
CHAR szWindowClass[MAX_LOADSTRING];            // nombre de clase de la ventana principal

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

void SetupImGuiStyle();
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Colocar código aquí.

	SetProcessDPIAware();

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

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	SetupImGuiStyle();


	wchar_t wDataServerODBC[MAX_PATH];

	GetPrivateProfileStringW(L"CONNECT", L"DataServerODBC", L"", wDataServerODBC, sizeof(wDataServerODBC) / sizeof(wchar_t), L".\\Connect.ini");

	std::wstring wServerODBC(wDataServerODBC);

	bool runtime_connect_base;

	if (gQueryManager.Connect(WStringToUTF8(wServerODBC).c_str(), "", "") == 0)
	{
		runtime_connect_base = false;
		CreateMessageBox(MB_OK, "Error", "Could not connect to database");
		//LogAdd(LOG_RED, "Could not connect to database");
	}
	else
	{
		runtime_connect_base = true;
	}

	gItemManager->Load("Data\\Item_info.xml");

	char AccountID[12];
	char CharacterName[12];
	int inventory_type = -1;
	bool InventoryCharacter = true;
	bool WarehouseCharacter = false;

	sprintf(AccountID, "---");

	sprintf(CharacterName, "---");

	static DWORD i2 = 0;
	int item_index = -1;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		WindowWidth = (int)ImGui::GetIO().DisplaySize.x;

		WindowHeight = (int)ImGui::GetIO().DisplaySize.y;

		ImGui_ImplOpenGL3_NewFrame();

		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		//RenderImageConvert();

		static bool window = true;
		ImGui::SetNextWindowSize(ImVec2(736, 425));
		//!! You might want to use these ^^ values in the OS window instead, and add the ImGuiWindowFlags_NoTitleBar flag in the ImGui window !!

		if (ImGui::Begin("Finder Item Serial", &window))
		{
			ImGui::SetCursorPos(ImVec2(54, 55));
			ImGui::BeginChild(1, ImVec2(630, 104), true);

			static bool c11 = false;
			RenderCheckBox(160, 20, "Hexadecimal", &c11);

			if (c11)
			{
				RenderInputNumber(160, 55, "##ItemWord", &i2, 374, ImGuiInputTextFlags_CharsHexadecimal);
			}
			else
			{
				RenderInputNumber(160, 55, "##ItemWord", &i2, 374, ImGuiInputTextFlags_CharsDecimal);
			}

			if (RenderButton(75, 55, "Find Item"))
			{
				InventoryCharacter = runtime_check_inventory(i2, AccountID, CharacterName, item_index, inventory_type, sizeof(AccountID), sizeof(CharacterName));

				if (InventoryCharacter == false)
				{
					WarehouseCharacter = runtime_check_warehouse(i2, AccountID, CharacterName, item_index, inventory_type, sizeof(AccountID), sizeof(CharacterName));
				}
			}
			ImGui::EndChild();

			ImGui::SetCursorPos(ImVec2(54, 190));
			ImGui::BeginChild(3, ImVec2(630, 190), true);

			RenderLabel(25, 25, 0, "Connect DB:");

			if (runtime_connect_base)
				RenderLabel(120, 25, IM_COL32(42, 194, 9, 255), "Online");
			else
				RenderLabel(120, 25, IM_COL32(194, 9, 9, 255), "Offline");

			RenderLabel(25, 50, 0, "AccountID:");

			RenderLabel(120, 50, 0, AccountID);

			RenderLabel(25, 75, 0, "Character:");

			RenderLabel(120, 75, 0, CharacterName);

			RenderLabel(25, 100, 0, "Location:");

			if (InventoryCharacter)
			{
				if (inventory_type >= 0 && inventory_type < INVENTORY_WEAR_SIZE)
				{
					RenderLabel(120, 100, 0, "Equipment");
				}
				else if (inventory_type >= INVENTORY_WEAR_SIZE && inventory_type < INVENTORY_MAIN_SIZE)
				{
					RenderLabel(120, 100, 0, "Inventory Normal");
				}
				else if (inventory_type >= INVENTORY_MAIN_SIZE && inventory_type < INVENTORY_EXT1_SIZE)
				{
					RenderLabel(120, 100, 0, "Inventory Extension 1");
				}
				else if (inventory_type >= INVENTORY_EXT1_SIZE && inventory_type < INVENTORY_EXT2_SIZE)
				{
					RenderLabel(120, 100, 0, "Inventory Extension 2");
				}
				else if (inventory_type >= INVENTORY_EXT2_SIZE && inventory_type < INVENTORY_EXT3_SIZE)
				{
					RenderLabel(120, 100, 0, "Inventory Extension 2");
				}
				else if (inventory_type >= INVENTORY_EXT3_SIZE && inventory_type < INVENTORY_EXT4_SIZE)
				{
					RenderLabel(120, 100, 0, "Inventory Extension 4");
				}
				else if (inventory_type >= INVENTORY_EXT4_SIZE && inventory_type < INVENTORY_FULL_SIZE)
				{
					RenderLabel(120, 100, 0, "MyShopStore");
				}
				else
				{
					RenderLabel(120, 100, 0, "no found");
				}
			}
			else if (WarehouseCharacter)
			{
				if (inventory_type >= 0 && inventory_type < 120)
				{
					RenderLabel(120, 100, 0, "Warehouse");
				}
				else if (inventory_type >= 120 && inventory_type < WAREHOUSE_SIZE)
				{
					RenderLabel(120, 100, 0, "Warehouse Extension");
				}
				else
				{
					RenderLabel(120, 100, 0, "no found");
				}
			}
			else
			{
				RenderLabel(100, 100, 0, "no found");
			}

			RenderLabel(290, 100, 0, "Name:");

			RenderLabel(340, 100, 0, gItemManager->GetName(item_index));

			ImGui::EndChild();
		}
		ImGui::End();

		ImGui::Render();

		glViewport(0, 0, WindowWidth, WindowHeight);

		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		::SwapBuffers(g_MainWindow.hDC);
	}

	ImGui_ImplOpenGL3_Shutdown();

	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	CleanupDeviceWGL(g_hWnd, &g_MainWindow);

	wglDeleteContext(g_hRC);

	return (int)msg.wParam;
}

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

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Almacenar identificador de instancia en una variable global

	g_hWnd = CreateWindow(szWindowClass, "PentiumTools v2.9 ImGui", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

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
	{
		g_hRC = wglCreateContext(data->hDC);
	}

	return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
	wglMakeCurrent(nullptr, nullptr);
	::ReleaseDC(hWnd, data->hDC);
}

void SetupImGuiStyle()
{
	// login style from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 12.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(6.0f, 6.0f);
	style.FrameRounding = 4.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 5.199999809265137f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2832589447498322f, 0.2832600772380829f, 0.283261775970459f, 0.5400000214576721f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3218331336975098f, 0.339589387178421f, 0.3605149984359741f, 0.4000000059604645f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3012396395206451f, 0.3087479472160339f, 0.3175965547561646f, 0.6700000166893005f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1568627506494522f, 0.2862745225429535f, 0.47843137383461f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568f, 0.5899999737739563f, 0.9800000190734863f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239999994635582f, 0.5199999809265137f, 0.8799999952316284f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2599999904632568f, 0.5899999737739563f, 0.9800000190734863f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3100000023841858f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}