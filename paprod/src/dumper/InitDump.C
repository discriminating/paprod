/*
File:       InitDump.C
Purpose:    Initialization of dump process
Author:     @discriminating
Date:       18 December 2025
*/

#include <dumper/InitDump.H>

extern
VOID
OutputFormat(
    _In_    LPCWSTR lpFormat,
    ...
);

extern
LONG
NTAPI
NtSuspendProcess(
    _In_    HANDLE  ProcessHandle
);

extern
LONG
NTAPI
NtResumeProcess(
    _In_    HANDLE  ProcessHandle
);

HANDLE
GetRobloxHandle(
    _In_    BOOL    bUseClient,
    _In_    BOOL    bSuspend
)
{
    HANDLE      hRoblox         = NULL;
    BOOL        bRet            = FALSE;
    DWORD32     dwProcessId     = 0;

    if ( bUseClient )
    {
        bRet = GetProcessIdFromName(
            L"RobloxPlayerBeta.exe",
            (LPDWORD)&dwProcessId
        );
    }
    else
    {
        bRet = GetProcessIdFromName(
            L"RobloxStudioBeta.exe",
            (LPDWORD)&dwProcessId
        );
    }

    if ( !bRet || !dwProcessId )
    {
        OutputFormat(
            L"Error: Failed to find Roblox %s process.\n",
            ( bUseClient ? L"Player" : L"Studio" )
        );

        return INVALID_HANDLE_VALUE;
    }

    if ( bSuspend )
    {
        hRoblox = OpenProcess(
            PROCESS_SUSPEND_RESUME |
            PROCESS_VM_READ |
            PROCESS_QUERY_INFORMATION,
            FALSE,
            dwProcessId
        );
    }
    else
    {
        hRoblox = OpenProcess(
            PROCESS_VM_READ |
            PROCESS_QUERY_INFORMATION,
            FALSE,
            dwProcessId
        );
    }

    if ( !hRoblox || hRoblox == INVALID_HANDLE_VALUE )
    {
        OutputFormat(
            L"Error: Failed to open Roblox %s process (PID: %lu).\n",
            ( bUseClient ? L"Player" : L"Studio" ),
            dwProcessId
        );

        return INVALID_HANDLE_VALUE;
    }

    OutputFormat(
        L"Ok: Opened Roblox %s process (PID: %lu).\n",
        ( bUseClient ? L"Player" : L"Studio" ),
        dwProcessId
    );

    return hRoblox;
}

_Success_( return != 0 )
BOOL
InitDump(
    _In_    BOOL    bUseClient,
    _In_    BOOL	bSuspend,
    _In_    BOOL    bFastRenderViewScan
)
{
    HANDLE      hRoblox     = NULL;
    BOOL        bRet        = FALSE;
    NTSTATUS    lStatus     = 0;

    hRoblox = GetRobloxHandle(
        bUseClient,
        bSuspend
    );

    if ( !hRoblox || hRoblox == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    if ( bSuspend )
    {
        lStatus = NtSuspendProcess( hRoblox );
        
        if ( !NT_SUCCESS( lStatus ) )
        {
            OutputFormat(
                L"Error: Failed to suspend Roblox process (0x%08X).\n",
                lStatus
            );
            
            CloseHandle( hRoblox );
            
            return FALSE;
        }

        OutputFormat(
            L"Ok: Suspended Roblox process.\n"
        );
    }

    OutputFormat(
        L"Info: Dumping offsets...\n"
    );

    bRet = DumpOffsets(
        hRoblox,
        bFastRenderViewScan
    );

    if ( !bRet )
    {
        OutputFormat(
            L"Error: Failed to dump offsets.\n"
        );
    }
    else
    {
        OutputFormat(
            L"Ok: Successfully dumped offsets.\n"
        );
    }

    if ( bSuspend )
    {
        lStatus = NtResumeProcess( hRoblox );
        
        if ( !NT_SUCCESS( lStatus ) )
        {
            OutputFormat(
                L"Error: Failed to resume Roblox process (0x%08X).\n",
                lStatus
            );
        }
        else
        {
            OutputFormat(
                L"Ok: Resumed Roblox process.\n"
            );
        }
    }

    CloseHandle( hRoblox );

    return bRet;
}