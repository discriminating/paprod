#pragma once

#include <nt/NtUtils.H>
#include <rtti/RunTimeTypeInformation.H>
#include <strsafe.h>

/*
    Arbitrary search depths. TODO: Make these configurable?
*/

#define DATAMODEL_SEARCH_DEPTH      0x400
#define ROBLOX_STRING_MAX_LEN       100

typedef struct RBX_ViewMatrix
{
    FLOAT fMatrix[4][4];
} ViewMatrix, *PViewMatrix;

typedef struct RBX_ViewportSize
{
    FLOAT   fWidth;
    FLOAT   fHeight;
} ViewportSize, * PViewportSize;

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
    _In_                                        HANDLE      hRoblox,
    _In_                                        BOOL        bFastScan,
    _Outptr_result_nullonfailure_   __restrict  PVOID*      ppvRenderViewOut
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxGetDataModel(
    _In_                                        HANDLE      hRoblox,
    _In_                                        PVOID       pvRenderView,
    _Outptr_result_nullonfailure_   __restrict  PVOID*      ppvDataModelOut
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
RobloxReadString(
    _In_                            HANDLE          hRoblox,
    _In_                            PVOID           pvInstance,
    _In_                            DWORD           dwStringOffset,
    _In_                            DWORD           dwBufferSize,
    _Out_writes_z_( dwBufferSize )  PCHAR           pszOutBuffer
);
