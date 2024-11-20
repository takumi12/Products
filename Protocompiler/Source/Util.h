#pragma once
#define Naked_void										__declspec(naked) void

extern void SetHooking(BYTE head,DWORD offset,...);
