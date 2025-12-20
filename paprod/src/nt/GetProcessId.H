#pragma once

#include <nt/Nt.H>

_Success_( return != 0 )
BOOL
GetProcessIdFromName(
    _In_    LPWSTR      lpProcessName,
    _Out_   LPDWORD     lpProcessId
);
