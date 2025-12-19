#pragma once

#include <windows.h>
#include <nt/GetProcessId.H>
#include <dumper/Dump.H>

_Success_( return != 0 )
BOOL
InitDump(
    _In_    BOOL    bUseClient,
    _In_    BOOL	bSuspend,
    _In_    BOOL    bFastRenderViewScan
);