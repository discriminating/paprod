#pragma once

#pragma warning(push)
#pragma warning(disable:4668)
#include <windows.h>
#pragma warning(pop)

#pragma warning(disable:5045)

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

_Success_( return != 0 )
BOOL
IsClass(
    _In_        HANDLE          hRoblox,
    _In_        VOID*           pVirtualAddress,
    _In_z_      CONST CHAR*     szClassName
);

_Success_( return != 0 )
BOOL
IsRobloxClass(
    _In_    HANDLE          hRoblox,
    _In_    VOID*           pVirtualAddress
);