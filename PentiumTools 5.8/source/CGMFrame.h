#pragma once
#define GetWindowsX								(WinFrameX)
#define GetWindowsY								(WinFrameY)
#define PositionX_Add(x)						((GetWindowsX - 640.f + x ))
#define PositionY_Add(x)						((GetWindowsY - 480.f + x ))
#define PositionX_The_Mid(x)					(((GetWindowsX - x) / 2.f))
#define PositionY_The_Mid(x)					(((GetWindowsY - x) / 2.f))
#define PositionX_In_The_Mid(x)					(((GetWindowsX - 640.f) / 2.f + x ))
#define PositionY_In_The_Mid(x)					(((GetWindowsY - 480.f) / 2.f + x ))
#define Position_In_The_Left(x)					(((GetWindowsX) - x ))
#define Position_In_The_Down(x)					(((GetWindowsY) - x ))
#define ScaleMA(MA)								((MA * GetWindowsY / 480.0))

#define GetNoWideX(num)							((num / g_fScreenRate_x) * ((float)WindowWidth / 640.0))
#define GetNoWideY(num)							((num / g_fScreenRate_y) * ((float)WindowHeight / 480.0))


extern float WinFrameX;
extern float WinFrameY;
extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern unsigned int WindowWidth;
extern unsigned int WindowHeight;

extern int  TextBold[60];
extern char TextList[60][100];
extern int  TextListColor[60];

extern float AddMiddleX(float x = 640.0);
extern float AddMiddleY(float x = 480.0);
extern float AddPositionX(float x = 0.0);
extern float AddPositionY(float x = 0.0);
