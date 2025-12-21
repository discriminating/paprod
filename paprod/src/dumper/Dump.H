#pragma once

#include <dumper/LinearSearch.H>

typedef struct _ROBLOX_OFFSETS
{
    DWORD   dwParent;
    DWORD   dwClassDescriptor;
    DWORD   dwClassDescriptorName;
    DWORD   dwInstanceName;
    DWORD   dwChildren;
    DWORD   dwViewMatrix;
    /*DWORD   dwPrimitive;*/
    /*DWORD   dwCFrame;*/

    /*DWORD   dwModelInstance;*/
    /*DWORD   dwHumanoid;*/
    /*DWORD   dwHealth;*/
    /*DWORD   dwMaxHealth;*/
    /*DWORD   dwViewportSize;*/
} ROBLOX_OFFSETS, *PROBLOX_OFFSETS;

_Success_( return != 0 )
BOOL
DumpOffsets(
    _In_    HANDLE  hRoblox,
    _In_    BOOL    bFastRenderViewScan
);
