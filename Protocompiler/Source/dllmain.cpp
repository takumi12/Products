// dllmain.cpp : Define el punto de entrada de la aplicación DLL.
#include "stdafx.h"
#include "Util.h"


//APIEXPORT void SynchronizeInit()
//{
/*
0065FD66   > 8B85 E0EFFFFF  MOV EAX,DWORD PTR SS:[EBP-1020]
0065FD6C   . 50             PUSH EAX                                 ; /Arg4
0065FD6D   . 8B4D F0        MOV ECX,DWORD PTR SS:[EBP-10]            ; |
0065FD66   > 8B85 E0EFFFFF  MOV EAX,DWORD PTR SS:[EBP-1020]
0065FD6C   . 50             PUSH EAX                                 ; /Arg4
0065FD6D   . 8B4D F0        MOV ECX,DWORD PTR SS:[EBP-10]            ; |
0065FD70   . 51             PUSH ECX                                 ; |Arg3
0065FD71   . 8B55 E8        MOV EDX,DWORD PTR SS:[EBP-18]            ; |
0065FD74   . 52             PUSH EDX                                 ; |Arg2
0065FD75   . 8B45 EC        MOV EAX,DWORD PTR SS:[EBP-14]            ; |
0065FD78   . 50             PUSH EAX                                 ; |Arg1
0065FD79   . E8 A23D0000    CALL Main.00663B20                       ; \Main.00663B20
*/

void TranslateProtocol(BYTE HeadCode, BYTE* ReceiveBuffer, int Size, int bEncrypted) // OK
{
	//pLog->LogMessage(0, "[HeadCode:%0X][SubCode:%0X][Size: %d]", HeadCode, ((ReceiveBuffer[0] == 0xC1) ? ReceiveBuffer[3] : ReceiveBuffer[4]), Size);
}

Naked_void Protocompiler()
{
	static DWORD AddrJmp = 0x0065FD66;
	static DWORD AddrNext = 0x0065FD45;

	_asm
	{
		Mov     Eax, Dword Ptr ss:[Ebp-0x1020]
		Push    Eax
		Mov     Ecx, Dword Ptr ss:[Ebp-0x10]
		Push    Ecx
		Mov     Edx, Dword Ptr ss:[Ebp-0x18]
		Push    Edx
		Mov     Eax, Dword Ptr ss:[Ebp-0x14]
		Push    Eax
		Call    [TranslateProtocol]
		Cmp     Dword Ptr ss: [Ebp-0x1848], 1
		Je      NEXT
		Jmp     [AddrNext]
NEXT:
		Jmp     [AddrJmp]
	}
}

APIEXPORT void EntryProc() // OK
{
	SetHooking(0xE9, 0x0065FD3A, &TranslateProtocol);
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		pLog;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}