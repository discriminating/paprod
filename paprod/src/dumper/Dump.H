#pragma once

#include <dumper/LinearSearch.H>

_Success_( return != 0 )
BOOL
DumpOffsets(
    _In_    HANDLE  hRoblox,
    _In_    BOOL    bFastRenderViewScan
);
