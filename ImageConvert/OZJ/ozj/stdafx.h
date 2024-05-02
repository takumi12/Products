// stdafx.h: archivo de inclusión de los archivos de inclusión estándar del sistema
// o archivos de inclusión específicos de un proyecto utilizados frecuentemente,
// pero rara vez modificados
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Excluir material rara vez utilizado de encabezados de Windows
// Archivos de encabezado de Windows:
#include <windows.h>
#include <string>
#include <gl\GL.h>
#include <jpeglib\Jpeglib.h>
#define IMAGEN_TITLE			"ImagenConvert"
#define SAFE_DELETE_ARRAY(p)	{ if(p) { delete [] (p);(p)=NULL; } }

//extern "C" _declspec(dllexport) bool SaveImage(std::string Filename, bool Encrypt);
// TODO: mencionar aquí los encabezados adicionales que el programa necesita
