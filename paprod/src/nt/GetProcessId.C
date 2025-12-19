/*
File:       GetProcessId.C
Purpose:    Get a process' ID from its name
Author:     @discriminating
Date:       18 December 2025
*/

#include <nt/GetProcessId.H>

_Success_( return != 0 )
BOOL
GetProcessIdFromName(
    _In_    LPWSTR      lpProcessName,
    _Out_   LPDWORD     lpProcessId
)
{
    DWORD                           dwBufferSize    = 0;
    ULONG                           dwSize          = 0;
    NTSTATUS                        lStatus         = 0;
    PVOID                           pBuffer         = NULL;
    BOOL                            bStatus         = FALSE;
    SYSTEM_PROCESS_INFORMATION*     psProcessInfo   = { 0 };

    if ( lpProcessId        == NULL     ||
         lpProcessName      == NULL     ||
         lpProcessName[0]   == L'\0' )
    {
        return FALSE;
    }

    /*
        Query the size of the buffer needed...
    */

    (VOID)NtQuerySystemInformation(
        SystemProcessInformation,
        NULL,
        0,
        &dwBufferSize
    );

    do
    {
        if ( pBuffer )
        {
            (VOID)VirtualFree(
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
        (VOID)VirtualFree(
            pBuffer,
            0,
            MEM_RELEASE
        );

        return FALSE;
    }

    psProcessInfo = (SYSTEM_PROCESS_INFORMATION*)pBuffer;

    while ( psProcessInfo )
    {
        if ( !psProcessInfo->ImageName.Buffer       ||
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

        *lpProcessId = (DWORD)(DWORD64)psProcessInfo->UniqueProcessId;

        bStatus = TRUE;

        break;

lblNext:
        if ( psProcessInfo->NextEntryOffset == 0 )
        {
            break;
        }

        psProcessInfo =
            (SYSTEM_PROCESS_INFORMATION*)( (BYTE*)psProcessInfo + psProcessInfo->NextEntryOffset );
    }

    (VOID)VirtualFree(
        pBuffer,
        0,
        MEM_RELEASE
    );

    return bStatus;
}