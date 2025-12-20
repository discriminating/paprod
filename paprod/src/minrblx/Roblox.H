#pragma once

#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4005)
#include <ntstatus.h>
#pragma warning(pop)

#include <rtti/RunTimeTypeInformation.H>

/*
    Arbitrary heap pointer check... TODO: Improve this?
*/
#define IS_HEAP_PTR(ptr) ( (DWORD64)ptr > 0x1000000000 && (DWORD64)ptr < 0x4FFFFFFFFFFF )

#define ARBITRARY_HEAP_BARRIER	0x4FFFFFFFFFFF
#define PAGE_SIZE               0x1000

typedef struct RBX_ViewMatrix
{
    FLOAT fMatrix[4][4];
} ViewMatrix, *PViewMatrix;

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