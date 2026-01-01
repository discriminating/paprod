/*
File:       LinearSearch.C
Purpose:    Linear search functions for memory scanning
Author:     @discriminating
Date:       28 December 2025
*/

#include <dumper/LinearSearch.H>

extern
VOID
OutputFormat(
    _In_    LPCWSTR lpFormat,
    ...
);

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForClass(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_z_      CHAR*       szClassName,
    _In_        DWORD       dwMaxSearch,
    _In_        DWORD       dwAlignment,
    _Out_       DWORD*      pdwOffsetOut
)
{
    PVOID   pvAddress   = 0;
    DWORD   dwSearch    = 0;
    BOOL    bRet        = FALSE;

    if ( !hRoblox || !pvStart || !szClassName || !pdwOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( dwAlignment == 0 || dwMaxSearch == 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    for ( ; dwSearch < ( dwAlignment * dwMaxSearch ); dwSearch += dwAlignment )
    {
        bRet = ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvStart + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForClass: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvStart + dwSearch ),
                GetLastError()
            );

            continue;
        }

        if ( IsClass(
            hRoblox,
            pvAddress,
            szClassName
        ) )
        {
            break;
        }
    }

    if ( dwSearch == ( dwAlignment * dwMaxSearch ) )
    {
        return STATUS_NOT_FOUND;
    }

    *pdwOffsetOut = dwSearch;

    return STATUS_SUCCESS;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForWorkspace(
    _In_                            HANDLE  hRoblox,
    _In_                            PVOID   pvDataModel,
    _Outptr_result_nullonfailure_   PVOID*  pvWorkspaceOut
)
{
    PVOID   pvAddress       = 0;
    
    DWORD   dwSearch        = 0;
    DWORD   dwMaxSearch     = sizeof( PVOID ) * WORKSPACE_SEARCH_DEPTH;

    BOOL    bRet            = FALSE;

    if ( !hRoblox || !pvDataModel || !pvWorkspaceOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    for ( ; dwSearch < dwMaxSearch; dwSearch += sizeof( PVOID ) )
    {
        bRet = ReadProcessMemory(
            hRoblox,
            (LPCVOID) ( (DWORD64) pvDataModel + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForWorkspace: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvDataModel + dwSearch ),
                GetLastError()
            );

            continue;
        }

        if ( IsClass(
            hRoblox,
            pvAddress,
            ".?AVWorkspace@RBX@@"
        ) )
        {
            break;
        }
    }

    if ( dwSearch == dwMaxSearch )
    {
        return STATUS_NOT_FOUND;
    }

    *pvWorkspaceOut = pvAddress;

    return STATUS_SUCCESS;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForString(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_z_      CHAR*       szString,
    _In_        DWORD       dwMaxSearch,
    _In_        DWORD       dwAlignment,
    _Out_       DWORD*      pdwStringOffsetOut
)
{
    if ( !hRoblox || !pvStart || !szString )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( !pdwStringOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /*
        See RobloxReadString for string structure.
    */

    if ( strlen( szString ) > ( sizeof( ( (struct _ROBLOX_STRING *)0 )->szString ) - 1) ) /* 15 */
    {
        return STATUS_INVALID_PARAMETER;
    }

    for ( DWORD dwSearch = 0; dwSearch < ( dwAlignment * dwMaxSearch ); dwSearch += dwAlignment )
    {
        PVOID   pvAddress       = 0;
        CHAR    szBuffer[16]    = { 0 }; /* sizeof( ( (struct _ROBLOX_STRING *)0 )->szString */

        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvStart + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        ) )
        {
            OutputFormat(
                L"Warning: LinearSearchForString: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvStart + dwSearch ),
                GetLastError()
            );

            continue;
        }

        if ( !pvAddress )
        {
            continue;
        }

        if ( !IS_VALID_POINTER( hRoblox, pvAddress ) )
        {
            continue;
        }

        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)pvAddress,
            &szBuffer,
            sizeof( szBuffer ) - 1,
            NULL
        ) )
        {
            OutputFormat(
                L"Warning: LinearSearchForString: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID)pvAddress,
                GetLastError()
            );

            continue;
        }

        if ( strcmp(
            szBuffer,
            szString
        ) == 0 )
        {
            *pdwStringOffsetOut = dwSearch;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForChildren(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwChildrenOffsetOut
)
{
    if ( !hRoblox || !pvStart || !pdwChildrenOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    for ( DWORD dwSearch = 0; dwSearch < ( sizeof( PVOID ) * dwMaxSearch ); dwSearch += sizeof( PVOID ) )
    {
        NTSTATUS    lStatus         = STATUS_UNSUCCESSFUL;

        PVOID       pvAddress       = 0;
        
        PVOID       pvChild         = NULL;
        PVOID       pvChildrenPtr   = NULL;
        PVOID       pvListStart     = NULL;
        PVOID       pvListEnd       = NULL;
        PVOID       pChildrenBuff   = NULL;

        DWORD       dwChildCount    = 0;

        BOOL        bRet            = FALSE;
        BOOL        bOkay           = FALSE;

        bRet = ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvStart + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForChildren: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvStart + dwSearch ),
                GetLastError()
            );

            continue;
        }

        if ( !IS_VALID_POINTER( hRoblox, pvAddress ) )
        {
            continue;
        }

        /*
            Structure:
        
            RBX::Instance
             - xxx
             - xxx
             - Pointer children                 (PVOID)
                - Pointer to start of list      (PVOID)
                    - Ptr to child 1            (PVOID)
                    - Ptr to child 1 deleter    (PVOID)
                    - Ptr to child 2            (PVOID)
                    - Ptr to child 2 deleter    (PVOID)
                    ...
                - Pointer to end of list        (PVOID)
        */

        bRet = ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvStart + dwSearch ),
            &pvChildrenPtr,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForChildren: ReadProcessMemory for children ptr failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvStart + dwSearch ),
                GetLastError()
            );

            continue;
        }

        if ( !IS_VALID_POINTER( hRoblox, pvChildrenPtr ) )
        {
            continue;
        }

        bRet = ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvChildrenPtr ),
            &pvListStart,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForChildren: ReadProcessMemory for list start failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvChildrenPtr ),
                GetLastError()
            );

            continue;
        }

        if ( !IS_VALID_POINTER( hRoblox, pvListStart ) )
        {
            continue;
        }

        bRet = ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvChildrenPtr + sizeof( PVOID ) ),
            &pvListEnd,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForChildren: ReadProcessMemory for list end failed at %ph because %lu\n",
                (LPCVOID)( (DWORD64)pvChildrenPtr + sizeof(PVOID) ),
                GetLastError()
            );

            continue;
        }

        if ( !IS_VALID_POINTER( hRoblox, pvListEnd ) )
        {
            continue;
        }

        if ( pvListStart >= pvListEnd )
        {
            continue;
        }

        /*
            Loop through the children and check
            if it's a valid Roblox class.
        */

        dwChildCount = (DWORD)( (DWORD64)pvListEnd - (DWORD64)pvListStart ) / ( sizeof( PVOID ) * 2 );

        if ( dwChildCount == 0 )
        {
            continue;
        }

        lStatus = RobloxBatchGetChildren(
            hRoblox,
            pvListStart,
            &pChildrenBuff,
            dwChildCount
        );

        if ( !NT_SUCCESS( lStatus ) || !pChildrenBuff )
        {
            OutputFormat(
                L"Warning: LinearSearchForChildren: RobloxBatchGetChildren failed for children at %ph\n",
                (LPCVOID)pvListStart
            );

            continue;
        }

        bOkay = TRUE;

        for ( DWORD i = 0; i < dwChildCount; i++ )
        {
            pvChild = *(PVOID*)( (DWORD64)pChildrenBuff + ( i * sizeof( PVOID ) * 2 ) );

            if ( !pvChild )
            {
                continue;
            }

            if ( !IsRobloxClass(
                hRoblox,
                pvChild
            ) )
            {
                bOkay = FALSE;

                break;
            }
        }

        if ( bOkay )
        {
            *pdwChildrenOffsetOut = dwSearch;
            
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForProjectionViewMatrix(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvVisualEngine,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwViewMatrixOffsetOut
)
{
    if ( !hRoblox || !pvVisualEngine || !pdwViewMatrixOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    for ( DWORD dwSearch = 0; dwSearch < ( sizeof( PVOID ) * dwMaxSearch ); dwSearch += sizeof( PVOID ) )
    {
        PVOID       pvAddress       = (PVOID)( (DWORD64)pvVisualEngine + dwSearch );
        
        BOOL        bHasNaN         = FALSE;
        
        ViewMatrix  mViewMatrix     = { 0 };
        
        if ( !pvAddress )
        {
            continue;
        }

        if ( !ReadProcessMemory(
            hRoblox,
            pvAddress,
            &mViewMatrix,
            sizeof( ViewMatrix ),
            NULL
        ) )
        {
            OutputFormat(
                L"Warning: LinearSearchForProjectionViewMatrix: ReadProcessMemory failed at %ph because %lu\n",
                pvAddress,
                GetLastError()
            );

            continue;
        }

        /*
            [   0.9112     0.0000      -0.4057     -231.5724   ]
            [   -0.3308    1.1740      -0.7430     221.6931    ]
            [   0.0000     0.0000      0.0000      0.0997      ]
            [   -0.3343    -0.5695     -0.7510     267.7311    ]
        */

        if ( fabsf( mViewMatrix.fMatrix[2][0] ) > 0.01f     ||
             fabsf( mViewMatrix.fMatrix[2][1] ) > 0.01f     ||
             fabsf( mViewMatrix.fMatrix[2][2] ) > 0.01f     ||
             fabsf( mViewMatrix.fMatrix[2][3] - 0.1f ) > 0.01f )
        {
            continue;
        }

        /*
            NaN check.
        */

        for ( INT i = 0; i < 4; i++ )
        {
            for ( INT j = 0; j < 4; j++ )
            {
                if ( !isfinite( mViewMatrix.fMatrix[i][j] ) )
                {
                    bHasNaN = TRUE;
                    
                    break;
                }
            }

            if ( bHasNaN )
            {
                break;
            }
        }

        if ( bHasNaN )
        {
            continue;
        }

        *pdwViewMatrixOffsetOut = dwSearch;
        
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForViewportSize(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvVisualEngine,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwViewportSizeOffsetOut
)
{
    if ( !hRoblox || !pvVisualEngine || !pdwViewportSizeOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( dwMaxSearch == 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    HMONITOR    hMonitor            = NULL;

    HWND        hwndRoblox          = NULL;

    RECT        rcRoblox            = { 0 };

    UINT        nDPIX               = 0;
    UINT        nDPIY               = 0;

    FLOAT       fExpectedWidth      = 0.0f;
    FLOAT       fExpectedHeight     = 0.0f;

    hwndRoblox = FindWindowW(
        NULL,
        L"Roblox"
    );

    if ( !hwndRoblox )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !GetClientRect(
        hwndRoblox,
        &rcRoblox
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    hMonitor = MonitorFromWindow(
        hwndRoblox,
        MONITOR_DEFAULTTONEAREST
    );

    if ( !hMonitor )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( FAILED( GetDpiForMonitor(
        hMonitor,
        MDT_EFFECTIVE_DPI,
        &nDPIX,
        &nDPIY
    ) ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    /*
        ViewportSize is the un-scaled client size in pixels.
    */
    
    fExpectedWidth    = (FLOAT)rcRoblox.right    * 96.0f / (FLOAT)nDPIX;
    fExpectedHeight   = (FLOAT)rcRoblox.bottom   * 96.0f / (FLOAT)nDPIY;

    for ( DWORD dwSearch = 0; dwSearch < ( sizeof( PVOID ) * dwMaxSearch ); dwSearch += sizeof( PVOID ) )
    {
        PVOID           pvAddress       = (PVOID)( (DWORD64)pvVisualEngine + dwSearch );
        
        ViewportSize    vsViewportSize  = { 0 };
        
        if ( !pvAddress )
        {
            continue;
        }
        
        if ( !ReadProcessMemory(
            hRoblox,
            pvAddress,
            &vsViewportSize,
            sizeof( ViewportSize ),
            NULL
        ) )
        {
            OutputFormat(
                L"Warning: LinearSearchForViewportSize: ReadProcessMemory failed at %ph because %lu\n",
                pvAddress,
                GetLastError()
            );

            continue;
        }
        
        if ( !isfinite( vsViewportSize.fWidth ) || !isfinite( vsViewportSize.fHeight ) )
        {
            continue;
        }

        if ( vsViewportSize.fWidth  == 0 ||
             vsViewportSize.fHeight == 0 )
        {
            continue;
        }

        if ( vsViewportSize.fWidth  < ROBLOX_MINIMUM_WINDOW_WIDTH   ||
             vsViewportSize.fHeight < ROBLOX_MINIMUM_WINDOW_HEIGHT )
        {
            continue;
        }

        if ( fabsf( vsViewportSize.fWidth  - fExpectedWidth )  > 1.0f   ||
             fabsf( vsViewportSize.fHeight - fExpectedHeight ) > 1.0f )
        {
            continue;
        }

        *pdwViewportSizeOffsetOut = dwSearch;
        
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForCFrame(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pvPrimitive,
    _In_    DWORD   dwMaxSearch,
    _Out_   DWORD*  pdwCFrameOffsetOut
)
{
    if ( !hRoblox || !pvPrimitive || !pdwCFrameOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    for ( DWORD dwSearch = 0; dwSearch < ( sizeof( PVOID ) * dwMaxSearch ); dwSearch += sizeof( PVOID ) )
    {
        PVOID       pvAddress       = (PVOID)( (DWORD64)pvPrimitive + dwSearch );
        
        CFrame      cfCFrame        = { 0 };

        BOOL        bHasNaN         = FALSE;

        Vector3     v3Right         = { 0 };
        Vector3     v3Up            = { 0 };
        Vector3     v3Look          = { 0 };

        FLOAT       fRightLen       = 0.0f;
        FLOAT       fUpLen          = 0.0f;
        FLOAT       fLookLen        = 0.0f;

        FLOAT       fRightUpDot     = 0.0f;
        FLOAT       fRightLookDot   = 0.0f;
        FLOAT       fUpLookDot      = 0.0f;

        FLOAT       fCrossDiff      = 0.0f;

        Vector3     v3Cross         = { 0 };
        
        if ( !ReadProcessMemory(
            hRoblox,
            pvAddress,
            &cfCFrame,
            sizeof( CFrame ),
            NULL
        ) )
        {
            continue;
        }

        /*
            NaN check.
        */

        for ( INT i = 0; i < 12; i++ )
        {
            if ( !isfinite( ( (FLOAT*)&cfCFrame )[i] ) )
            {
                bHasNaN = TRUE;
                break;
            }
        }

        if ( bHasNaN )
        {
            continue;
        }

        /*
            Extract vectors from column-major matrix.
        */

        v3Right.X   = cfCFrame.m00;
        v3Right.Y   = cfCFrame.m01;
        v3Right.Z   = cfCFrame.m02;

        v3Up.X      = cfCFrame.m10;
        v3Up.Y      = cfCFrame.m11;
        v3Up.Z      = cfCFrame.m12;

        v3Look.X    = cfCFrame.m20;
        v3Look.Y    = cfCFrame.m21;
        v3Look.Z    = cfCFrame.m22;

        /*
            Assume player is in reasonable bounds.
        */

        if ( fabsf( cfCFrame.v3Position.X ) > 100000.0f     ||
             fabsf( cfCFrame.v3Position.Y ) > 100000.0f     ||
             fabsf( cfCFrame.v3Position.Z ) > 100000.0f )
        {
            continue;
        }

        /*
            Unit vector length check.
        */

        fRightLen = sqrtf(
            v3Right.X * v3Right.X +
            v3Right.Y * v3Right.Y +
            v3Right.Z * v3Right.Z
        );

        fUpLen = sqrtf(
            v3Up.X * v3Up.X +
            v3Up.Y * v3Up.Y +
            v3Up.Z * v3Up.Z
        );

        fLookLen = sqrtf(
            v3Look.X * v3Look.X +
            v3Look.Y * v3Look.Y +
            v3Look.Z * v3Look.Z
        );

        if ( fabsf( fRightLen - 1.0f )  > 0.1f  ||
             fabsf( fUpLen - 1.0f )     > 0.1f  ||
             fabsf( fLookLen - 1.0f )   > 0.1f )
        {
            continue;
        }

        /*
            Orthogonality check.
        */

        fRightUpDot     =   v3Right.X * v3Up.X +
                            v3Right.Y * v3Up.Y +
                            v3Right.Z * v3Up.Z;

        fRightLookDot   =   v3Right.X * v3Look.X +
                            v3Right.Y * v3Look.Y +
                            v3Right.Z * v3Look.Z;

        fUpLookDot      =   v3Up.X * v3Look.X +
                            v3Up.Y * v3Look.Y +
                            v3Up.Z * v3Look.Z;

        if ( fabsf( fRightUpDot )   > 0.1f  ||
             fabsf( fRightLookDot ) > 0.1f  ||
             fabsf( fUpLookDot )    > 0.1f )
        {
            continue;
        }

        /*
            Verify cross product.
        */

        v3Cross.X   =   v3Up.Y * v3Look.Z - v3Up.Z * v3Look.Y;
        v3Cross.Y   =   v3Up.Z * v3Look.X - v3Up.X * v3Look.Z;
        v3Cross.Z   =   v3Up.X * v3Look.Y - v3Up.Y * v3Look.X;

        fCrossDiff  =   fabsf( v3Cross.X - v3Right.X ) +
                        fabsf( v3Cross.Y - v3Right.Y ) +
                        fabsf( v3Cross.Z - v3Right.Z );

        if ( fCrossDiff > 0.2f )
        {
            continue;
        }

        *pdwCFrameOffsetOut = dwSearch;

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForFloat(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pvInstance,
    _In_    FLOAT   fValue,
    _In_    DWORD   dwMaxSearch,
    _Out_   DWORD*  pdwOffsetOut
)
{
    if ( !hRoblox || !pvInstance || !pdwOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( !isfinite( fValue ) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    PBYTE   pBuffer     = NULL;

    pBuffer = LocalAlloc(
        LPTR,
        sizeof( FLOAT ) * dwMaxSearch
    );

    if ( pBuffer == NULL )
    {
        return STATUS_NO_MEMORY;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        pvInstance,
        pBuffer,
        sizeof( FLOAT ) * dwMaxSearch,
        NULL
    ) )
    {
        (VOID)LocalFree(
            pBuffer
        );

        return STATUS_UNSUCCESSFUL;
    }

    for ( DWORD dwSearch = 0; dwSearch < ( sizeof( FLOAT ) * dwMaxSearch ); dwSearch += sizeof( FLOAT ) )
    {
        FLOAT   pvFloat = *(FLOAT*)(pBuffer + dwSearch);

        if ( !isfinite( pvFloat ) )
        {
            continue;
        }

        if ( fabsf( pvFloat - fValue ) < 0.1f )
        {
            (VOID)LocalFree(
                pBuffer
            );

            *pdwOffsetOut = dwSearch;

            return STATUS_SUCCESS;
        }
    }

    (VOID)LocalFree(
        pBuffer
    );

    return STATUS_NOT_FOUND;
}
