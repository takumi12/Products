// stdafx.h: archivo de inclusión de los archivos de inclusión estándar del sistema
// o archivos de inclusión específicos de un proyecto utilizados frecuentemente,
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

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Excluir material rara vez utilizado de encabezados de Windows
// Archivos de encabezado de Windows:
#include <windows.h>
#include <string>
#include <vector>
#include <gl\GL.h>
#include <SOIL/SOIL.h>

#define IMAGEN_TITLE			"ImagenConvert"
#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete [] (p);(p)=NULL; } }

//extern "C" _declspec(dllexport) bool SaveImage(std::string Filename, bool Encrypt);
// TODO: mencionar aquí los encabezados adicionales que el programa necesita
