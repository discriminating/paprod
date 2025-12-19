#pragma once


#include <windows.h>
#include <winternl.h>
#pragma warning(push)
#pragma warning(disable:4005)
#include <ntstatus.h>
#pragma warning(pop)

#pragma comment(lib, "ntdll.lib")

_Success_( return != 0 )
BOOL
GetProcessIdFromName(
    _In_    LPWSTR      lpProcessName,
    _Out_   LPDWORD     lpProcessId
);