#pragma once

#include <nt/Nt.H>

_Success_( return == STATUS_SUCCESS )
NTSTATUS
IsValidPointer(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pVirtualAddress
);

#define IS_VALID_POINTER( hProcess, pPtr ) \
    ( IsValidPointer( hProcess, (PVOID)( pPtr ) ) == STATUS_SUCCESS )

#define IS_VALID_POINTER_DWORD64( hProcess, pPtr ) \
    ( IsValidPointer( hProcess, (PVOID)(DWORD64)( pPtr ) ) == STATUS_SUCCESS )
