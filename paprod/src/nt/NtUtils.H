#pragma once

#pragma warning(push)
#pragma warning(disable:4005)
#pragma warning(disable:4820)
#pragma warning(disable:4668)
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#pragma warning(pop)

#pragma warning(disable:5045)
#pragma warning(disable:4711)

#pragma comment(lib, "ntdll.lib")

#define MAX_MEMORY_ADDR     0x7FFFFFFFFFFF
#define PAGE_SIZE           0x1000

_Success_( return != 0 )
BOOL
GetProcessIdFromName(
    _In_    LPWSTR      lpProcessName,
    _Out_   LPDWORD     lpProcessId
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
IsValidPointer(
    _In_    HANDLE      hRoblox,
    _In_    PVOID       pVirtualAddress
);

#define IS_VALID_POINTER( hProcess, pPtr ) \
    ( IsValidPointer( hProcess, (PVOID)( pPtr ) ) == STATUS_SUCCESS )

#define IS_VALID_POINTER_DWORD64( hProcess, pPtr ) \
    ( IsValidPointer( hProcess, (PVOID)(DWORD64)( pPtr ) ) == STATUS_SUCCESS )
