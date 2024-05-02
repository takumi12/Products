#include "stdafx.h"
#include "Util.h"

int uMouse = 0;
int MouseX = 0;
int MouseY = 0;
int MouseWheel = 0;
bool MouseMButton = false;
bool MouseLButton = false;
bool MouseRButton = false;
bool MouseMButtonPop = false;
bool MouseLButtonPop = false;
bool MouseRButtonPop = false;
bool MouseMButtonPush = false;
bool MouseLButtonPush = false;
bool MouseRButtonPush = false;
int g_iMousePopPosition_x = 0;
int g_iMousePopPosition_y = 0;

void CreateMessageBox(UINT uType, const char* title, char* message,...) // OK
{
	char buff[256];
	memset(buff,0,sizeof(buff));
	va_list arg;
	va_start(arg,message);
	vsprintf_s(buff,message,arg);
	va_end(arg);
	MessageBox(NULL, buff, title, uType);
}

std::string GetModulePath()
{
	char buffer[MAX_PATH];
	DWORD size = GetModuleFileName(NULL, buffer, MAX_PATH);

	if (size != 0)
	{
		char dir[_MAX_DIR];
		char drive[_MAX_DRIVE];
		_splitpath_s(buffer, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);

		std::string directoryPath = drive;
		directoryPath += dir;

		return directoryPath;
	}
	else
	{
		return "";
	}
}