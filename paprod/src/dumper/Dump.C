/*
File:       Dump.C
Purpose:    Functions to dump Roblox offsets
Author:     @discriminating
Date:       20 December 2025
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
    _In_    HANDLE  hRoblox,
    _In_    BOOL    bFastRenderViewScan
)
{
    if ( !hRoblox || hRoblox == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    NTSTATUS            lStatus                 = STATUS_UNSUCCESSFUL;
    BOOL                bRet                    = FALSE;

    PVOID               pvRenderView            = 0x0;
    PVOID               pvDataModel             = 0x0;
    PVOID               pvWorkspace             = 0x0;
    PVOID               pvClassDescriptor       = 0x0;
    PVOID               pvVisualEngine          = 0x0;

    ROBLOX_OFFSETS      sRobloxOffsets          = { 0 };

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
        50,
        sizeof( PVOID ),
        &sRobloxOffsets.dwParent
    );

    if ( !NT_SUCCESS( lStatus ) || !sRobloxOffsets.dwParent )
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
        50,
        sizeof( PVOID ),
        &sRobloxOffsets.dwClassDescriptor
    );

    if ( !NT_SUCCESS( lStatus ) || !sRobloxOffsets.dwClassDescriptor )
    {
        OutputFormat(
            L"Warning: Failed to find Instance->ClassDescriptor offset (0x%08X).\n",
            lStatus
        );
    }

    /*
        Instance->ClassDescriptor->Name
    */

    if ( !sRobloxOffsets.dwClassDescriptor )
    {
        OutputFormat(
            L"Warning: Skipping ClassDescriptor->Name offset search due to missing ClassDescriptor offset.\n"
        );

        goto lblSkipClassDescriptorName;
    }

    bRet = ReadProcessMemory(
        hRoblox,
        (LPCVOID) ( (DWORD64) pvWorkspace + sRobloxOffsets.dwClassDescriptor ),
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
        50,
        sizeof( PVOID ),
        &sRobloxOffsets.dwClassDescriptorName
    );

    if ( !NT_SUCCESS( lStatus ) || !sRobloxOffsets.dwClassDescriptorName )
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
        50,
        sizeof( PVOID ),
        &sRobloxOffsets.dwInstanceName
    );

    if ( !NT_SUCCESS( lStatus ) || !sRobloxOffsets.dwInstanceName )
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
        50,
        &sRobloxOffsets.dwChildren
    );

    if ( !NT_SUCCESS( lStatus ) || !sRobloxOffsets.dwChildren )
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
            170,
            &sRobloxOffsets.dwViewMatrix
        );

        if ( !NT_SUCCESS( lStatus ) || !sRobloxOffsets.dwViewMatrix )
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

    OutputFormat(
        L"\n============================== OFFSETS ==============================\n\n"
    );

    OutputFormat(
        L"#define INSTANCE_PARENT_PTR_OFFSET             0x%lx\n",
        sRobloxOffsets.dwParent
    );

    OutputFormat(
        L"#define INSTANCE_CLASS_DESCRIPTOR_PTR_OFFSET   0x%lx\n",
        sRobloxOffsets.dwClassDescriptor
    );

    OutputFormat(
        L"#define CLASS_DESCRIPTOR_NAME_PTR_OFFSET       0x%lx\n",
        sRobloxOffsets.dwClassDescriptorName
    );

    OutputFormat(
        L"#define INSTANCE_NAME_PTR_OFFSET               0x%lx\n",
        sRobloxOffsets.dwInstanceName
    );

    OutputFormat(
        L"#define INSTANCE_CHILDREN_PTR_OFFSET           0x%lx\n",
        sRobloxOffsets.dwChildren
    );

    OutputFormat(
        L"#define VISUALENGNE_VIEW_MATRIX_OFFSET         0x%lx\n",
        sRobloxOffsets.dwViewMatrix
    );

    OutputFormat(
        L"\n=====================================================================\n"
    );

    bRet = TRUE;

    goto lblExit;

lblFail:

    bRet = FALSE;

lblExit:

    return bRet;
}