// header.h: archivo de inclusión para archivos de inclusión estándar del sistema,
// o archivos de inclusión específicos de un proyecto
//

#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Excluir material rara vez utilizado de encabezados de Windows
// Archivos de encabezado de Windows
#include <windows.h>
// Archivos de encabezado en tiempo de ejecución de C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <nlohmann/json.hpp>

#include <time.h>
#include <Mmsystem.h>
#include <locale>
#include <codecvt>

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

#define GLEW_BUILD
#include <glew.h>
#include <GL/gl.h>


#pragma comment(lib,"Winmm.lib")

#include "imgui.h"

#define CONSOLE

#include "ConsoleDebug.h"

using json = nlohmann::json;
extern HWND g_hWnd;
extern int SettingInterface;