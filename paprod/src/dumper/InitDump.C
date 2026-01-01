/*
File:       InitDump.C
Purpose:    Initialization of dump process
Author:     @discriminating
Date:       31 December 2025
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

VOID
PrintOffsets(
    _In_    PROBLOX_OFFSETS  psRobloxOffsets
)
{
    OutputFormat(
        L"\n============================== OFFSETS ==============================\n\n"
    );

    OutputFormat(
        L"#define INSTANCE_PARENT_PTR_OFFSET                0x%lx\n",
        psRobloxOffsets->dwParent
    );

    OutputFormat(
        L"#define INSTANCE_CLASS_DESCRIPTOR_PTR_OFFSET      0x%lx\n",
        psRobloxOffsets->dwClassDescriptor
    );

    OutputFormat(
        L"#define CLASS_DESCRIPTOR_NAME_PTR_OFFSET          0x%lx\n",
        psRobloxOffsets->dwClassDescriptorName
    );

    OutputFormat(
        L"#define INSTANCE_NAME_PTR_OFFSET                  0x%lx\n",
        psRobloxOffsets->dwInstanceName
    );

    OutputFormat(
        L"#define INSTANCE_CHILDREN_PTR_OFFSET              0x%lx\n",
        psRobloxOffsets->dwChildren
    );

    OutputFormat(
        L"#define VISUALENGNE_VIEW_MATRIX_OFFSET            0x%lx\n",
        psRobloxOffsets->dwViewMatrix
    );

    OutputFormat(
        L"#define VISUALENGINE_VIEWPORT_SIZE_OFFSET         0x%lx\n",
        psRobloxOffsets->dwViewportSize
    );

    OutputFormat(
        L"#define PLAYER_MODEL_INSTANCE_OFFSET              0x%lx\n",
        psRobloxOffsets->dwModelInstance
    );

    OutputFormat(
        L"#define INSTANCE_PRIMITIVE_PTR_OFFSET             0x%lx\n",
        psRobloxOffsets->dwPrimitive
    );

    OutputFormat(
        L"#define PRIMITIVE_CFRAME_OFFSET                   0x%lx\n",
        psRobloxOffsets->dwCFrame
    );

    OutputFormat(
        L"#define HUMANOID_HEALTH_OFFSET                    0x%lx\n",
        psRobloxOffsets->dwHealth
    );

    OutputFormat(
        L"#define HUMANOID_MAX_HEALTH_OFFSET                0x%lx\n",
        psRobloxOffsets->dwMaxHealth
    );

    OutputFormat(
        L"#define HUMANOID_JUMP_POWER_OFFSET                0x%lx\n",
        psRobloxOffsets->dwJumpPower
    );

    OutputFormat(
        L"#define HUMANOID_JUMP_HEIGHT_OFFSET               0x%lx\n",
        psRobloxOffsets->dwJumpHeight
    );

    OutputFormat(
        L"#define HUMANOID_HIP_HEIGHT_OFFSET                0x%lx\n",
        psRobloxOffsets->dwHipHeight
    );

    OutputFormat(
        L"#define HUMANOID_MAX_SLOPE_ANGLE_OFFSET           0x%lx\n",
        psRobloxOffsets->dwMaxSlopeAngle
    );

    OutputFormat(
        L"#define HUMANOID_WALK_SPEED_OFFSET                0x%lx\n",
        psRobloxOffsets->dwWalkSpeed
    );

    OutputFormat(
        L"\n=====================================================================\n\n"
    );
}

_Success_( return != 0 )
BOOL
InitDump(
    _In_    BOOL    bUseClient,
    _In_    BOOL	bSuspend,
    _In_    BOOL    bFastRenderViewScan
)
{
    HANDLE              hRoblox             = NULL;
    BOOL                bRet                = FALSE;
    NTSTATUS            lStatus             = 0;
    INT                 nMsgBoxRet          = 0;
    ROBLOX_OFFSETS      sRobloxOffsets      = { 0 };

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
        bUseClient,
        bFastRenderViewScan,
        &sRobloxOffsets
    );

    if ( !bRet )
    {
        OutputFormat(
            L"Error: Failed to dump offsets.\n"
        );

        nMsgBoxRet = MessageBoxW(
            NULL,
            L"Failed to dump offsets. Please check the output for more information.\n\nWould you like to see the output anyway?",
            L"Roblox Offset Dumper",
            MB_ICONERROR | MB_YESNO
        );

        if ( nMsgBoxRet != IDYES )
        {
            goto lblIDResumeAndExit;
        }
    }
    else
    {
        OutputFormat(
            L"Ok: Successfully dumped offsets.\n"
        );
    }

    PrintOffsets( &sRobloxOffsets );

lblIDResumeAndExit:
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