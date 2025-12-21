#pragma once

#include <dumper/LinearSearch.H>

#define ARBITRARY_SEARCH_DEPTH                  50
#define PROJECTION_VIEW_MATRIX_SEARCH_DEPTH     170

typedef struct _ROBLOX_OFFSETS
{
    /*
        Obtainable via LuaApp.
    */
    
    DWORD   dwParent;
    DWORD   dwClassDescriptor;
    DWORD   dwClassDescriptorName;
    DWORD   dwInstanceName;
    DWORD   dwChildren;
    DWORD   dwViewMatrix;
    /*DWORD   dwViewportSize;*/
    
    /*
        Need to be in-game.
    */

    /*DWORD   dwModelInstance;*/
    /*DWORD   dwPrimitive;*/
    /*DWORD   dwCFrame;*/
    /*DWORD   dwHumanoid;*/
    /*DWORD   dwHealth;*/
    /*DWORD   dwMaxHealth;*/
} ROBLOX_OFFSETS, *PROBLOX_OFFSETS;

_Success_( return != 0 )
BOOL
DumpOffsets(
    _In_    HANDLE              hRoblox,
    _In_    BOOL                bIsUsingClient,
    _In_    BOOL                bFastRenderViewScan,
    _Out_   PROBLOX_OFFSETS     psRobloxOffsets
);