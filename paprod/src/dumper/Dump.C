/*
File:       Dump.C
Purpose:    Functions to dump Roblox offsets
Author:     @discriminating
Date:       31 December 2025
*/

#include <dumper/Dump.H>

extern
VOID
OutputFormat(
    _In_    LPCWSTR lpFormat,
    ...
);

_Success_( return != 0 )
BOOL
GetAddresses(
    _In_    HANDLE  hRoblox,
    _In_    BOOL    bFastRenderViewScan,
    _Out_   PVOID*  ppvRenderView,
    _Out_   PVOID*  ppvDataModel,
    _Out_   PVOID*  ppvWorkspace
)
{
    NTSTATUS    lStatus         = STATUS_UNSUCCESSFUL;

    PVOID       pvRenderView    = 0x0;
    PVOID       pvDataModel     = 0x0;
    PVOID       pvWorkspace     = 0x0;

    lStatus = RobloxGetRenderView(
        hRoblox,
        bFastRenderViewScan,
        &pvRenderView
    );

    if ( !NT_SUCCESS( lStatus ) || !pvRenderView )
    {
        OutputFormat(
            L"Error: Failed to get RenderView address (0x%08X).\n",
            lStatus
        );

        return FALSE;
    }

    OutputFormat(
        L"Ok: Found RenderView at address 0x%p.\n",
        pvRenderView
    );

    lStatus = RobloxGetDataModel(
        hRoblox,
        pvRenderView,
        &pvDataModel
    );

    if ( !NT_SUCCESS( lStatus ) || !pvDataModel )
    {
        OutputFormat(
            L"Error: Failed to get DataModel address (0x%08X).\n",
            lStatus
        );

        if ( lStatus == STATUS_PENDING )
        {
            OutputFormat(
                L"Info: DataModel is NULL. Please dump while in a game...\n"
            );
        }

        return FALSE;
    }

    OutputFormat(
        L"Ok: Found DataModel at address 0x%p.\n",
        pvDataModel
    );

    lStatus = LinearSearchForWorkspace(
        hRoblox,
        pvDataModel,
        &pvWorkspace
    );

    if ( !NT_SUCCESS( lStatus ) || !pvWorkspace )
    {
        OutputFormat(
            L"Error: Failed to get Workspace address (0x%08X).\n",
            lStatus
        );

        return FALSE;
    }

    OutputFormat(
        L"Ok: Found Workspace at address 0x%p.\n",
        pvWorkspace
    );

    *ppvRenderView  = pvRenderView;
    *ppvDataModel   = pvDataModel;
    *ppvWorkspace   = pvWorkspace;

    return TRUE;
}

_Success_( return != 0 )
BOOL
DumpOffsets(
    _In_    HANDLE              hRoblox,
    _In_    BOOL                bIsUsingClient,
    _In_    BOOL                bFastRenderViewScan,
    _Out_   PROBLOX_OFFSETS     psRobloxOffsets
)
{
    if ( !hRoblox || hRoblox == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    NTSTATUS        lStatus                 = STATUS_UNSUCCESSFUL;

    BOOL            bRet                    = FALSE;
    BOOL            bIsInGame               = FALSE;

    PVOID           pvRenderView            = 0x0;
    PVOID           pvDataModel             = 0x0;
    PVOID           pvWorkspace             = 0x0;
    PVOID           pvVisualEngine          = 0x0;
    PVOID           pvPlayers               = 0x0;

    PVOID           pvClassDescriptor       = 0x0;

    PVOID           pvFirstPlayer                       = 0x0;
    PVOID           pvFirstPlayerModelInstance          = 0x0;
    PVOID           pvFirstPlayerHead                   = 0x0;

    PVOID           pvHeadPrimitive         = 0x0;

    PVOID           pvHumanoid              = 0x0;

    CHAR    szTempName[ ROBLOX_STRING_MAX_LEN + 1 ]     = { 0 };

    ZeroMemory(
        psRobloxOffsets,
        sizeof( ROBLOX_OFFSETS )
    );

    bRet = GetAddresses(
        hRoblox,
        bFastRenderViewScan,
        &pvRenderView,
        &pvDataModel,
        &pvWorkspace
    );

    if ( !bRet )
    {
        goto lblFail;
    }

    /*
        RenderView->VisualEngine
    */

    bRet = ReadProcessMemory(
        hRoblox,
        (LPCVOID)( (DWORD64) pvRenderView + ( sizeof( PVOID ) * 2 ) ), /* Skip pointer to self, skip pointer to DeviceD3D11... */
        &pvVisualEngine,
        sizeof( PVOID ),
        NULL
    );

    if ( !bRet || !pvVisualEngine )
    {
        OutputFormat(
            L"Warning: Failed to read VisualEngine pointer from RenderView (%lu).\n",
            GetLastError()
        );
    }

    /*
        Instance->Parent
    */

    lStatus = LinearSearchForClass(
        hRoblox,
        pvWorkspace,
        ".?AVDataModel@RBX@@",
        ARBITRARY_SEARCH_DEPTH,
        sizeof( PVOID ),
        &psRobloxOffsets->dwParent
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwParent )
    {
        OutputFormat(
            L"Warning: Failed to find Instance->Parent offset (0x%08X).\n",
            lStatus
        );
    }

    /*
        Instance->ClassDescriptor
    */

    lStatus = LinearSearchForClass(
        hRoblox,
        pvWorkspace,
        ".?AVClassDescriptor@Reflection@RBX@@",
        ARBITRARY_SEARCH_DEPTH,
        sizeof( PVOID ),
        &psRobloxOffsets->dwClassDescriptor
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwClassDescriptor )
    {
        OutputFormat(
            L"Warning: Failed to find Instance->ClassDescriptor offset (0x%08X).\n",
            lStatus
        );
    }

    /*
        Instance->ClassDescriptor->Name
    */

    if ( !psRobloxOffsets->dwClassDescriptor )
    {
        OutputFormat(
            L"Warning: Skipping ClassDescriptor->Name offset search due to missing ClassDescriptor offset.\n"
        );

        goto lblSkipClassDescriptorName;
    }

    bRet = ReadProcessMemory(
        hRoblox,
        (LPCVOID) ( (DWORD64) pvWorkspace + psRobloxOffsets->dwClassDescriptor ),
        &pvClassDescriptor,
        sizeof( PVOID ),
        NULL
    );

    if ( !bRet || !pvClassDescriptor )
    {
        OutputFormat(
            L"Warning: Skipping ClassDescriptor->Name offset search due to failed read of ClassDescriptor pointer (%lu).\n",
            GetLastError()
        );
        
        goto lblSkipClassDescriptorName;
    }

    lStatus = LinearSearchForString(
        hRoblox,
        pvClassDescriptor,
        "Workspace",
        ARBITRARY_SEARCH_DEPTH,
        sizeof( PVOID ),
        &psRobloxOffsets->dwClassDescriptorName
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwClassDescriptorName )
    {
        OutputFormat(
            L"Warning: Failed to find ClassDescriptor->Name offset (0x%08X).\n",
            lStatus
        );
    }

lblSkipClassDescriptorName:
    
    /*
        Instance->Name
    */

    lStatus = LinearSearchForString(
        hRoblox,
        pvWorkspace,
        "Workspace",
        ARBITRARY_SEARCH_DEPTH,
        sizeof( PVOID ),
        &psRobloxOffsets->dwInstanceName
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwInstanceName )
    {
        OutputFormat(
            L"Warning: Failed to find Instance->Name offset (0x%08X).\n",
            lStatus
        );
    }

    /*
        Instance->Children
    */

    lStatus = LinearSearchForChildren(
        hRoblox,
        pvWorkspace,
        ARBITRARY_SEARCH_DEPTH,
        &psRobloxOffsets->dwChildren
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwChildren )
    {
        OutputFormat(
            L"Warning: Failed to find Instance->Children offset (0x%08X).\n",
            lStatus
        );
    }

    /*
        RenderView->VisualEngine->ViewMatrix
    */

    if ( pvVisualEngine )
    {
        lStatus = LinearSearchForProjectionViewMatrix(
            hRoblox,
            pvVisualEngine,
            PROJECTION_VIEW_MATRIX_SEARCH_DEPTH,
            &psRobloxOffsets->dwViewMatrix
        );

        if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwViewMatrix )
        {
            OutputFormat(
                L"Warning: Failed to find ViewMatrix offset (0x%08X).\n",
                lStatus
            );
        }
    }
    else
    {
        OutputFormat(
            L"Warning: Skipping ViewMatrix offset search due to missing VisualEngine pointer.\n"
        );
    }

    /*
        Determine whether it is safe to continue.
    */

    if ( !psRobloxOffsets->dwInstanceName )
    {
        OutputFormat(
            L"Error: Failed to find ctitical offset Instance->Name. Cannot determine whether it is safe to continue.\n"
        );

        return FALSE;
    }

    lStatus = RobloxReadName(
        hRoblox,
        pvDataModel,
        psRobloxOffsets->dwInstanceName,
        sizeof( szTempName ),
        szTempName
    );

    if ( !NT_SUCCESS( lStatus ) && lStatus != STATUS_BUFFER_OVERFLOW )
    {
        OutputFormat(
            L"Error: Failed to read instance name string (0x%08X).\n",
            lStatus
        );

        goto lblFail;
    }

    if ( bIsUsingClient )
    {
        /*
            Datamodel is named 'Ugc' when in-game and
            'LuaApp' when on the home-screen.
        */

        bIsInGame = strcmp(
            szTempName,
            "Ugc"
        ) == 0;
    }
    else
    {
        /*
            Studio is a bit more ambiguous...

            There's no reliable way to determine
            whether a player is inside a game in
            Studio or not. Assume they are.
        */

        bIsInGame = TRUE;
    }

    if ( !bIsInGame )
    {
        MessageBoxA(
            NULL,
            "Partially succeeded.\n\nIf you want more offsets, please "
            "join a game and re-dump.",
            "Roblox Offset Dumper",
            MB_ICONINFORMATION | MB_OK
        );

        OutputFormat(
            L"Info: You are not in-game. Please join a game to get more offsets.\n"
        );

        bRet = TRUE;

        goto lblExit;
    }

    /*
        VisualEngine->ViewportSize
    */

    if ( pvVisualEngine )
    {
        lStatus = LinearSearchForViewportSize(
            hRoblox,
            pvVisualEngine,
            VIEWPORT_SIZE_SEARCH_DEPTH,
            &psRobloxOffsets->dwViewportSize
        );

        if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwViewportSize )
        {
            OutputFormat(
                L"Warning: Failed to find ViewportSize offset (0x%08X).\n",
                lStatus
            );

            if ( !bIsUsingClient )
            {
                OutputFormat(
                    L"Info: You need to use the client for this offset.\n"
                );
            }
        }
    }

    /*
        DataModel->Children[Players]
    */

    if ( !psRobloxOffsets->dwChildren )
    {
        OutputFormat(
            L"Error: Failed to find critical offset Instance->Children.\n"
        );

        goto lblFail;
    }

    lStatus = RobloxFindFirstChildOfRTTIClass(
        hRoblox,
        pvDataModel,
        psRobloxOffsets->dwChildren,
        ".?AVPlayers@RBX@@",
        &pvPlayers
    );

    if ( !NT_SUCCESS( lStatus ) || !pvPlayers )
    {
        OutputFormat(
            L"Error: Could not find DataModel->Children[RBX::Players] (0x%08X). Are you in a game?\n",
            lStatus
        );

        goto lblFail;
    }

    OutputFormat(
        L"Ok: Found Players instance at address 0x%p.\n",
        pvPlayers
    );

    /*
        Get the first player.
    */

    lStatus = RobloxFindFirstChildOfRTTIClass(
        hRoblox,
        pvPlayers,
        psRobloxOffsets->dwChildren,
        ".?AVPlayer@RBX@@",
        &pvFirstPlayer
    );

    if ( !NT_SUCCESS( lStatus ) || !pvFirstPlayer )
    {
        OutputFormat(
            L"Error: Could not find first Player instance (0x%08X).\n",
            lStatus
        );

        if ( !bIsUsingClient )
        {
            MessageBoxA(
                NULL,
                "It seems you're using Studio. Please make sure you are actually "
                "inside a game, and not just in the editor.",
                "Roblox Offset Dumper",
                MB_ICONINFORMATION | MB_OK
            );

            OutputFormat(
                L"Info: It seems you're using Studio. Please make sure you are actually "
                L"inside a game, and not just in the editor.\n"
            );
        }

        goto lblFail;
    }

    OutputFormat(
        L"Ok: Found first Player instance at address 0x%p.\n",
        pvFirstPlayer
    );

    /*
        Player->ModelInstance
    */

    lStatus = LinearSearchForClass(
        hRoblox,
        pvFirstPlayer,
        ".?AVModelInstance@RBX@@",
        MODELINSTANCE_SEARCH_DEPTH,
        sizeof( PVOID ),
        &psRobloxOffsets->dwModelInstance
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwModelInstance )
    {
        OutputFormat(
            L"Error: Failed to find critical offset Player->ModelInstance (0x%08X).\n",
            lStatus
        );

        goto lblFail;
    }

    /*
        Get the first player's model instance.
    */

    if ( !ReadProcessMemory(
        hRoblox,
        (LPCVOID)( (DWORD64)pvFirstPlayer + psRobloxOffsets->dwModelInstance ),
        &pvFirstPlayerModelInstance,
        sizeof( PVOID ),
        NULL
    ) )
    {
        OutputFormat(
            L"Error: Failed to read first player's model instance.\n",
            lStatus
        );

        goto lblFail;
    }

    /*
        Get the first player's head.
    */

    lStatus = RobloxFindFirstChildOfName(
        hRoblox,
        pvFirstPlayerModelInstance,
        psRobloxOffsets->dwChildren,
        psRobloxOffsets->dwInstanceName,
        "Head",
        &pvFirstPlayerHead
    );

    if ( !NT_SUCCESS( lStatus ) || !pvFirstPlayerHead )
    {
        OutputFormat(
            L"Error: Failed to get first player's head. (0x%08X).\n",
            lStatus
        );

        goto lblFail;
    }

    OutputFormat(
        L"Ok: Found first player's head instance at address: 0x%p.\n",
        pvFirstPlayerHead
    );

    /*
        Instance->Primitive
    */

    lStatus = LinearSearchForClass(
        hRoblox,
        pvFirstPlayerHead,
        ".?AVPrimitive@RBX@@",
        PRIMITIVE_SEARCH_DEPTH,
        sizeof( PVOID ),
        &psRobloxOffsets->dwPrimitive
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwPrimitive )
    {
        OutputFormat(
            L"Error: Failed to get critical offset Instance->Primitive. (0x%08X).\n",
            lStatus
        );

        goto lblFail;
    }

    /*
        Get the head's primitive.
    */

    if ( !ReadProcessMemory(
        hRoblox,
        (LPCVOID)( (DWORD64)pvFirstPlayerHead + psRobloxOffsets->dwPrimitive ),
        &pvHeadPrimitive,
        sizeof( PVOID ),
        NULL
    ) )
    {
        OutputFormat(
            L"Error: Failed to read head primitive.\n",
            lStatus
        );

        goto lblFail;
    }

    OutputFormat(
        L"Ok: Found first player's head's primitive at address: 0x%p\n",
        pvHeadPrimitive
    );

    /*
        Primitive->CFrame
    */

    lStatus = LinearSearchForCFrame(
        hRoblox,
        pvHeadPrimitive,
        CFRAME_SEARCH_DEPTH,
        &psRobloxOffsets->dwCFrame
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwCFrame )
    {
        OutputFormat(
            L"Warning: Failed to get offset Primitive->CFrame.\n",
            lStatus
        );
    }

    /*
        Get the player's Humanoid.
    */

    lStatus = RobloxFindFirstChildOfRTTIClass(
        hRoblox,
        pvFirstPlayerModelInstance,
        psRobloxOffsets->dwChildren,
        ".?AVHumanoid@RBX@@",
        &pvHumanoid
    );

    OutputFormat(
        L"Ok: Found first player's humanoid at address: 0x%p\n",
        pvHumanoid
    );

    if ( !NT_SUCCESS( lStatus ) || !pvHumanoid )
    {
        OutputFormat(
            L"Error: Failed to get find player's Humanoid: 0x%08X.\n",
            lStatus
        );

        goto lblFail;
    }

    /*
        Humanoid->Health
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        HEALTH_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwHealth
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwHealth )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->Health: 0x%08X.\n",
            lStatus
        );
    }

    /*
        Humanoid->MaxHealth
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        MAX_HEALTH_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwMaxHealth
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwMaxHealth )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->dwMaxHealth: 0x%08X.\n",
            lStatus
        );
    }

    /*
        Humanoid->JumpPower
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        JUMP_POWER_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwJumpPower
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwJumpPower )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->dwJumpPower: 0x%08X.\n",
            lStatus
        );
    }

    /*
        Humanoid->JumpHeight
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        JUMP_HEIGHT_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwJumpHeight
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwJumpHeight )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->dwJumpHeight: 0x%08X.\n",
            lStatus
        );
    }

    /*
        Humanoid->HipHeight
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        HIP_HEIGHT_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwHipHeight
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwHipHeight )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->dwHipHeight: 0x%08X.\n",
            lStatus
        );
    }

    /*
        Humanoid->MaxSlopeAngle
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        MAX_SLOPE_ANGLE_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwMaxSlopeAngle
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwMaxSlopeAngle )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->dwMaxSlopeAngle: 0x%08X.\n",
            lStatus
        );
    }

    /*
        Humanoid->WalkSpeed
    */

    lStatus = LinearSearchForFloat(
        hRoblox,
        pvHumanoid,
        WALK_SPEED_VALUE,
        HUMANOID_SEARCH_DEPTH,
        &psRobloxOffsets->dwWalkSpeed
    );

    if ( !NT_SUCCESS( lStatus ) || !psRobloxOffsets->dwWalkSpeed )
    {
        OutputFormat(
            L"Warning: Failed to get offset Humanoid->WalkSpeed: 0x%08X.\n",
            lStatus
        );
    }

    bRet = TRUE;

    goto lblExit;

lblFail:

    bRet = FALSE;

lblExit:

    return bRet;
}