#pragma once

#include <windows.h>

struct TypeDescriptor
{
    CONST PVOID     pVFTable;
    PVOID		    pSpare;
    CHAR		    szName[128];
};

struct RTTICompleteObjectLocator
{
    DWORD		dwSignature;
    DWORD		dwOffset;
    DWORD		dwClassDescriptorOffset;
    INT			pTypeDescriptor;
    INT			pClassDescriptor;
    INT			pSelf;
};

BOOL
IsClass(
    _In_    HANDLE          hRoblox,
    _In_    VOID*           pVirtualAddress,
    _In_    CONST CHAR*     szClassName
);

BOOL
IsRobloxClass(
    _In_    HANDLE          hRoblox,
    _In_    VOID*           pVirtualAddress
);