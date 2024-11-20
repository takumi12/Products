#include "stdafx.h"
#include "CGMFrame.h"

char TextList[60][100];
int  TextListColor[60];
int  TextBold[60];

float WinFrameX = (1024 / 1.25);
float WinFrameY = ( 768 / 1.25);

float AddMiddleX(float x)
{
	return ((WinFrameX - x) / 2.f);
}

float AddMiddleY(float x)
{
	return ((WinFrameY - x) / 2.f);
}

float AddPositionX(float x)
{
	return ((WinFrameX - 640.0) + x);
}

float AddPositionY(float x)
{
	return ((WinFrameY - 480.0) + x);
}