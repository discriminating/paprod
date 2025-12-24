#pragma once

#include <nt/NtUtils.H>
#include <rtti/RunTimeTypeInformation.H>

/*
    Arbitrary search depths. TODO: Make these configurable?
*/

#define DATAMODEL_SEARCH_DEPTH          0x400
#define WORKSPACE_SEARCH_DEPTH          0x70
#define ROBLOX_STRING_MAX_LEN           100

#define ROBLOX_MINIMUM_WINDOW_WIDTH     801.0f
#define ROBLOX_MINIMUM_WINDOW_HEIGHT    596.0f

typedef struct RBX_ViewMatrix
{
    FLOAT fMatrix[4][4];
} ViewMatrix, *PViewMatrix;

typedef struct RBX_ViewportSize
{
    FLOAT fWidth;
    FLOAT fHeight;
} ViewportSize, *PViewportSize;

typedef struct RBX_Vector3
{
    FLOAT X;
    FLOAT Y;
    FLOAT Z;
} Vector3, * PVector3;

typedef struct RBX_CFrame
{
    FLOAT m00, m10, m20;
    FLOAT m01, m11, m21;
    FLOAT m02, m12, m22;

    Vector3 v3Position;
} CFrame, *PCFrame;

/*
    Modified from std string SSO? Maybe...
*/

#pragma pack(push, 1)
typedef struct _ROBLOX_STRING
{
    union
    {
        PCHAR   pszLongString;
        CHAR    szShortString[16];
    } szString;
    DWORD32 dwStringLen;
} ROBLOX_STRING, *PROBLOX_STRING;
#pragma pack(pop)

_Success_( return == STATUS_SUCCESS )
NTSTATUS
WINAPI
RobloxGetRenderView(
    _In_                            HANDLE      hRoblox,
    _In_                            BOOL        bFastScan,
    _Outptr_result_nullonfailure_   PVOID*      ppvRenderViewOut
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxGetDataModel(
    _In_                            HANDLE      hRoblox,
    _In_                            PVOID       pvRenderView,
    _Outptr_result_nullonfailure_   PVOID*      ppvDataModelOut
);

_Success_( return == STATUS_SUCCESS )
_Must_inspect_result_
NTSTATUS
RobloxBatchGetChildren(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pChildrenStart,
    _Outptr_result_bytebuffer_maybenull_( dwChildrenAmount * ( sizeof( PVOID ) * 2 ) )
            PVOID*  ppvChildrenOutBuff,
    _In_    DWORD   dwChildrenAmount
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxReadName(
    _In_                            HANDLE      hRoblox,
    _In_                            PVOID       pvInstance,
    _In_                            DWORD       dwNameOffset,
    _In_                            DWORD       dwBufferSize,
    _Out_writes_z_( dwBufferSize )  PCHAR       pszOutBuffer
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxFindFirstChildOfRTTIClass(
    _In_                            HANDLE      hRoblox,
    _In_                            PVOID       pvInstance,
    _In_                            DWORD       dwChildrenOffset,
    _In_z_                          PCSTR       pszRTTIClassName,
    _Outptr_result_nullonfailure_   PVOID*      ppvChildOut
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxFindFirstChildOfName(
    _In_                            HANDLE      hRoblox,
    _In_                            PVOID       pvInstance,
    _In_                            DWORD       dwChildrenOffset,
    _In_                            DWORD       dwNameOffset,
    _In_z_                          PCSTR       pszName,
    _Outptr_result_nullonfailure_   PVOID*      ppvChildOut
);
