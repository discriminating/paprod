/*
File:       NtUtils.C
Purpose:    Memory functions, NT functions
Author:     @discriminating
Date:       19 December 2025
*/

#include <nt/NtUtils.H>

_Success_( return != 0 )
BOOL
GetProcessIdFromName(
    _In_    LPWSTR      lpProcessName,
    _Out_   LPDWORD     lpProcessId
)
{
    DWORD                           dwBufferSize = 0;
    ULONG                           dwSize = 0;
    NTSTATUS                        lStatus = 0;
    PVOID                           pBuffer = NULL;
    BOOL                            bStatus = FALSE;
    SYSTEM_PROCESS_INFORMATION* psProcessInfo = { 0 };

    if ( lpProcessId == NULL ||
        lpProcessName == NULL ||
        lpProcessName[0] == L'\0' )
    {
        return FALSE;
    }

    /*
        Query the size of the buffer needed...
    */

    (VOID) NtQuerySystemInformation(
        SystemProcessInformation,
        NULL,
        0,
        &dwBufferSize
    );

    do
    {
        if ( pBuffer )
        {
            (VOID) VirtualFree(
                pBuffer,
                0,
                MEM_RELEASE
            );
        }

        pBuffer = VirtualAlloc(
            NULL,
            dwBufferSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );

        if ( !pBuffer )
        {
            return FALSE;
        }

        lStatus = NtQuerySystemInformation(
            SystemProcessInformation,
            pBuffer,
            dwBufferSize,
            &dwSize
        );

        /*
            Increase buffer in case another process
            spawns after the first query...
        */

        if ( lStatus == STATUS_INFO_LENGTH_MISMATCH )
        {
            dwBufferSize *= 2;
        }
    }
    while ( lStatus == STATUS_INFO_LENGTH_MISMATCH );

    if ( !NT_SUCCESS( lStatus ) )
    {
        (VOID) VirtualFree(
            pBuffer,
            0,
            MEM_RELEASE
        );

        return FALSE;
    }

    psProcessInfo = (SYSTEM_PROCESS_INFORMATION*) pBuffer;

    while ( psProcessInfo )
    {
        if ( !psProcessInfo->ImageName.Buffer ||
            psProcessInfo->ImageName.Length < 0 )
        {
            goto lblNext;
        }

        if ( _wcsicmp(
            psProcessInfo->ImageName.Buffer,
            lpProcessName
        ) != 0 )
        {
            goto lblNext;
        }

        *lpProcessId = (DWORD) (DWORD64) psProcessInfo->UniqueProcessId;

        bStatus = TRUE;

        break;

lblNext:
        if ( psProcessInfo->NextEntryOffset == 0 )
        {
            break;
        }

        psProcessInfo =
            (SYSTEM_PROCESS_INFORMATION*) ( (BYTE*) psProcessInfo + psProcessInfo->NextEntryOffset );
    }

    (VOID) VirtualFree(
        pBuffer,
        0,
        MEM_RELEASE
    );

    return bStatus;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
IsValidPointer(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pVirtualAddress
)
{
    MEMORY_BASIC_INFORMATION    mbiInfo = { 0 };
    SIZE_T                      szBytesReturned = 0;

    if ( !hRoblox || !pVirtualAddress )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( hRoblox == INVALID_HANDLE_VALUE )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( (DWORD64) pVirtualAddress < PAGE_SIZE ||
        (DWORD64) pVirtualAddress > MAX_MEMORY_ADDR )
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

    if ( ( mbiInfo.Protect & PAGE_GUARD ) ||
        ( mbiInfo.Protect & PAGE_NOACCESS ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
