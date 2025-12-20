/*
File:       IsValidPointer.C
Purpose:    Check if a pointer is within valid memory bounds
Author:     @discriminating
Date:       19 December 2025
*/

#include <nt/IsValidPointer.H>

_Success_( return == STATUS_SUCCESS )
NTSTATUS
IsValidPointer(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pVirtualAddress
)
{
    MEMORY_BASIC_INFORMATION    mbiInfo     = { 0 };
    SIZE_T                      szBytesReturned     = 0;

    if ( !hRoblox || !pVirtualAddress )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( hRoblox == INVALID_HANDLE_VALUE )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( (DWORD64)pVirtualAddress < PAGE_SIZE ||
         (DWORD64)pVirtualAddress > MAX_MEMORY_ADDR )
    {
        return STATUS_UNSUCCESSFUL;
    }

    szBytesReturned = VirtualQueryEx(
        hRoblox,
        pVirtualAddress,
        &mbiInfo,
        sizeof( mbiInfo )
    );

    if ( szBytesReturned == 0 )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( mbiInfo.State != MEM_COMMIT )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( (mbiInfo.Protect & PAGE_GUARD) ||
         (mbiInfo.Protect & PAGE_NOACCESS) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
