#pragma once

#include <dumper/LinearSearch.H>

#define ARBITRARY_SEARCH_DEPTH                  50
#define PROJECTION_VIEW_MATRIX_SEARCH_DEPTH     170
#define VIEWPORT_SIZE_SEARCH_DEPTH              270
#define MODELINSTANCE_SEARCH_DEPTH              170
#define PRIMITIVE_SEARCH_DEPTH                  170
#define CFRAME_SEARCH_DEPTH                     50

#define HUMANOID_SEARCH_DEPTH                   200

#define MAX_HEALTH_VALUE                        102.0f
#define HEALTH_VALUE                            101.0f
#define JUMP_POWER_VALUE                        50.0f
#define JUMP_HEIGHT_VALUE                       7.2f
#define HIP_HEIGHT_VALUE                        2.225f
#define MAX_SLOPE_ANGLE_VALUE                   89.0f
#define WALK_SPEED_VALUE                         16.0f

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
    DWORD   dwViewportSize;
    
    /*
        Need to be in-game.
    */

    DWORD   dwModelInstance;
    DWORD   dwPrimitive;
    DWORD   dwCFrame;
    
    /*DWORD   dwGameID;*/
    DWORD   dwHealth;
    DWORD   dwMaxHealth;
    DWORD   dwJumpPower;
    DWORD   dwJumpHeight;
    DWORD   dwHipHeight;
    DWORD   dwMaxSlopeAngle;
    DWORD   dwWalkSpeed;
} ROBLOX_OFFSETS, *PROBLOX_OFFSETS;

_Success_( return != 0 )
BOOL
DumpOffsets(
    _In_    HANDLE              hRoblox,
    _In_    BOOL                bIsUsingClient,
    _In_    BOOL                bFastRenderViewScan,
    _Out_   PROBLOX_OFFSETS     psRobloxOffsets
);
