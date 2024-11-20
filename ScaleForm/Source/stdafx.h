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
#pragma warning( disable : 4786)

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Excluir material rara vez utilizado de encabezados de Windows
#define _WIN32_WINNT					_WIN32_WINNT_WIN7

#include <windows.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <GL/gl.h>
#include <glprocs.h>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm> 
#include <time.h>
#include <Mmsystem.h>
#include <assert.h>
#include "ConsoleDebug.h"

#define IMAGEN_TITLE							"ScaleForm"
#define SAFE_DELETE(p)							{ if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p)					{ if(p) { delete [] (p);     (p)=NULL; } }
#define SAFE_RELEASE(p)							{ if(p) { (p)->Release(); (p)=NULL; } }
#define	NON_SCENE								0
#define WEBZEN_SCENE							1
#define LOG_IN_SCENE							2
#define LOADING_SCENE							3
#define CHARACTER_SCENE							4
#define MAIN_SCENE								5

#define LANGUAGE_KOREAN							( 0 )
#define LANGUAGE_ENGLISH						( 1 )
#define LANGUAGE_TAIWANESE						( 2 )
#define LANGUAGE_CHINESE						( 3 )
#define LANGUAGE_JAPANESE						( 4 )
#define LANGUAGE_THAILAND						( 5 )
#define LANGUAGE_PHILIPPINES					( 6 )
#define LANGUAGE_VIETNAMESE						( 7 )
#define NUM_LANGUAGE							( 8 )


#define APIEXPORT								extern "C" _declspec(dllexport)



#define BoostSmart_Ptr(classname) \
	class classname; \
	typedef classname* classname##Ptr