#include "stdafx.h"
#include "Util.h"

void SetHooking(BYTE head, DWORD offset, ...) // OK
{
	DWORD OldProtect;

	VirtualProtect((void*)offset, 5, PAGE_EXECUTE_READWRITE, &OldProtect);

	if (head != 0xFF)
	{
		*(BYTE*)(offset) = head;
	}

	DWORD* function = &offset + 1;

	*(DWORD*)(offset + 1) = (*function) - (offset + 5);

	VirtualProtect((void*)offset, 5, OldProtect, &OldProtect);
}