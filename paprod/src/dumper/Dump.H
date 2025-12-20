#pragma once

#include <windows.h>
#include <winternl.h>
#include <math.h>
#include <minrblx/Roblox.H>

_Success_( return != 0 )
BOOL
DumpOffsets(
    _In_    HANDLE  hRoblox,
    _In_    BOOL    bFastRenderViewScan
);