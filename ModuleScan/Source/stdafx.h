// stdafx.h: archivo de inclusi�n de los archivos de inclusi�n est�ndar del sistema
// o archivos de inclusi�n espec�ficos de un proyecto utilizados frecuentemente,
// pero rara vez modificados
//

#pragma once
#pragma warning( disable : 4005 )
#pragma warning( disable : 4018 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4482 )
#pragma warning( disable : 4800 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4091 )
#pragma warning( disable : 4094 )
#pragma warning( disable : 4172 )
#pragma warning( disable : 4101 )
#pragma warning( disable : 4102 )
#pragma warning( disable : 4309 )
#pragma warning( disable : 4067 )
#pragma warning( disable : 4805 )
#pragma warning( disable : 4154 )
#pragma warning( disable : 4733 )
#pragma warning( disable : 4620 )
#pragma warning( disable : 4996 )

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#include "targetver.h"
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <tchar.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <shellapi.h> // Para IFileDialog

#include <shobjidl.h> // Para IFileDialog
#include <comdef.h> // Para CLSID y otros
#include "Util.h"

// TODO: mencionar aqu� los encabezados adicionales que el programa necesita
