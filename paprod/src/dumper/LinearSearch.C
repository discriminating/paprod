/*
File:       LinearSearch.C
Purpose:    Linear search functions for memory scanning
Author:     @discriminating
Date:       20 December 2025
*/

#include <dumper/LinearSearch.H>

extern
VOID
OutputFormat(
    _In_    LPCWSTR lpFormat,
    ...
);;

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
            (LPCVOID) ( (DWORD64) pvStart + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForClass: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID) ( (DWORD64) pvStart + dwSearch ),
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
        return STATUS_UNSUCCESSFUL;
    }

    *pdwOffsetOut = dwSearch;

    return STATUS_SUCCESS;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForWorkspace(
    _In_                                        HANDLE  hRoblox,
    _In_                                        PVOID   pvDataModel,
    _Outptr_result_nullonfailure_   __restrict  PVOID*  pvOutWorkspace
)
{
    PVOID   pvAddress   = 0;
    
    DWORD   dwSearch    = 0;
    DWORD   dwMaxSearch = sizeof( PVOID ) * 50;

    BOOL    bRet        = FALSE;

    if ( !hRoblox || !pvDataModel || !pvOutWorkspace )
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
                (LPCVOID) ( (DWORD64) pvDataModel + dwSearch ),
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
        return STATUS_UNSUCCESSFUL;
    }

    *pvOutWorkspace = pvAddress;

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
    _Out_       DWORD*      pdwOffsetOut
)
{
    if ( !hRoblox || !pvStart || !szString )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( !pdwOffsetOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /*
        Roblox uses std::strings for its names.

        I'm not going to implement scanning for a
        string longer than 15 characters, because
        then std::string uses a heap allocation.
    */

    if ( strlen( szString ) > 15 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    for ( DWORD dwSearch = 0; dwSearch < ( dwAlignment * dwMaxSearch ); dwSearch += dwAlignment )
    {
        PVOID   pvAddress       = 0;
        CHAR    szBuffer[16]    = { 0 };

        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID) ( (DWORD64) pvStart + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        ) )
        {
            OutputFormat(
                L"Warning: LinearSearchForString: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID) ( (DWORD64) pvStart + dwSearch ),
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
            (LPCVOID) pvAddress,
            &szBuffer,
            sizeof( szBuffer ) - 1,
            NULL
        ) )
        {
            OutputFormat(
                L"Warning: LinearSearchForString: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID) pvAddress,
                GetLastError()
            );

            continue;
        }

        if ( strcmp(
            szBuffer,
            szString
        ) == 0 )
        {
            *pdwOffsetOut = dwSearch;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForChildren(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvStart,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwOffsetOut
)
{
    if ( !hRoblox || !pvStart || !pdwOffsetOut )
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
            (LPCVOID) ( (DWORD64)pvStart + dwSearch ),
            &pvAddress,
            sizeof( PVOID ),
            NULL
        );

        if ( !bRet )
        {
            OutputFormat(
                L"Warning: LinearSearchForChildren: ReadProcessMemory failed at %ph because %lu\n",
                (LPCVOID) ( (DWORD64)pvStart + dwSearch ),
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
                (LPCVOID) ( (DWORD64)pvStart + dwSearch ),
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
                (LPCVOID) ( (DWORD64) pvChildrenPtr + sizeof(PVOID) ),
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

        dwChildCount = (DWORD) ( (DWORD64) pvListEnd - (DWORD64) pvListStart ) / ( sizeof( PVOID ) * 2 );

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
            pvChild = *(PVOID*) ( (DWORD64) pChildrenBuff + ( i * sizeof( PVOID ) * 2 ) );

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
            *pdwOffsetOut = dwSearch;
            
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
LinearSearchForProjectionViewMatrix(
    _In_        HANDLE      hRoblox,
    _In_        PVOID       pvVisualEngine,
    _In_        DWORD       dwMaxSearch,
    _Out_       DWORD*      pdwViewMatrixOffset
)
{
    if ( !hRoblox || !pvVisualEngine || !pdwViewMatrixOffset )
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    for ( DWORD dwSearch = 0; dwSearch < ( sizeof( PVOID ) * dwMaxSearch ); dwSearch += sizeof( PVOID ) )
    {
        PVOID       pvAddress       = (PVOID) ( (DWORD64)pvVisualEngine + dwSearch );
        
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

        if ( fabsf( mViewMatrix.fMatrix[2][0] ) > 0.01f     ||
             fabsf( mViewMatrix.fMatrix[2][1] ) > 0.01f     ||
             fabsf( mViewMatrix.fMatrix[2][2] ) > 0.01f     ||
             fabsf( mViewMatrix.fMatrix[2][3]   - 0.1f ) > 0.01f )
        {
            continue;
        }

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

        *pdwViewMatrixOffset = dwSearch;
        
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}
