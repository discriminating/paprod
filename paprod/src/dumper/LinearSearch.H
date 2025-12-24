#pragma once

#include <math.h>
#include <minrblx/Roblox.H>
#include <ShellScalingApi.h>

#pragma comment( lib, "Shcore.lib" )

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForClass(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_z_      CHAR*       szClassName,
    _In_        DWORD       dwMaxSearch,
    _In_        DWORD       dwAlignment,
    _Out_       DWORD*      pdwOffsetOut
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForWorkspace(
    _In_                            HANDLE  hRoblox,
    _In_                            PVOID   pvDataModel,
    _Outptr_result_nullonfailure_   PVOID*  pvOutWorkspace
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForString(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_z_      CHAR*       szString,
    _In_        DWORD       dwMaxSearch,
    _In_        DWORD       dwAlignment,
    _Out_       DWORD*      pdwOffsetOut
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForChildren(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwOffsetOut
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForProjectionViewMatrix(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvVisualEngine,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwViewMatrixOffset
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForViewportSize(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvVisualEngine,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwViewportSizeOffset
);
