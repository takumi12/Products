#pragma once
extern int uMouse;
extern int MouseX;
extern int MouseY;
extern int MouseWheel;
extern bool MouseMButton;
extern bool MouseLButton;
extern bool MouseRButton;
extern bool MouseMButtonPop;
extern bool MouseLButtonPop;
extern bool MouseRButtonPop;
extern bool MouseMButtonPush;
extern bool MouseLButtonPush;
extern bool MouseRButtonPush;
extern int g_iMousePopPosition_x;
extern int g_iMousePopPosition_y;

extern std::string GetModulePath();
extern void CreateMessageBox(UINT uType, const char* title, char* message,...);