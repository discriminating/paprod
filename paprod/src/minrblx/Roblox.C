/*
File:       Roblox.C
Purpose:    Functions to interact with Roblox client
Author:     @discriminating
Date:       21 December 2025
*/

#include <minrblx/Roblox.H>

_Success_( return == STATUS_SUCCESS )
NTSTATUS
WINAPI
RobloxGetRenderView(
    _In_                                        HANDLE      hRoblox,
    _In_                                        BOOL        bFastScan,
    _Outptr_result_nullonfailure_   __restrict  PVOID*      ppvRenderViewOut
)
{
    DWORD64                     dwAddress   = PAGE_SIZE;
    MEMORY_BASIC_INFORMATION    mbiInfo     = { 0 };

    PBYTE                       pBuffer     = NULL;
    SIZE_T                      szBufferSz  = 0;

    if ( !hRoblox || hRoblox == INVALID_HANDLE_VALUE || !ppvRenderViewOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvRenderViewOut = NULL;

    /*
        Walk the heap for RenderView's class instance...
    */

    while ( dwAddress < MAX_MEMORY_ADDR )
    {
        SIZE_T      szBytesReturned     = 0;
        SIZE_T      szBytesRead         = 0;
        BOOL        bResult             = FALSE;

        szBytesReturned = VirtualQueryEx(
            hRoblox,
            (LPCVOID)dwAddress,
            &mbiInfo,
            sizeof( mbiInfo )
        );

        if ( szBytesReturned == 0 )
        {
            dwAddress += PAGE_SIZE;

            continue;
        }

        /*
            RV is in pages that are committed and always RW.
        */

        if ( !( mbiInfo.State == MEM_COMMIT &&
            ( mbiInfo.Protect & PAGE_READWRITE ) ) )
        {
            dwAddress += mbiInfo.RegionSize;

            continue;
        }

        if ( mbiInfo.RegionSize > szBufferSz )
        {
            if ( pBuffer )
            {
                (VOID)VirtualFree(
                    pBuffer,
                    0,
                    MEM_RELEASE
                );

                pBuffer = NULL;
            }

            pBuffer = (PBYTE)VirtualAlloc(
                NULL,
                mbiInfo.RegionSize,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE
            );

            if ( !pBuffer )
            {
                return STATUS_NO_MEMORY;
            }

            szBufferSz = mbiInfo.RegionSize;
        }

#pragma warning(push)
#pragma warning(disable: 6387) /* Dumb linter... */
        bResult = ReadProcessMemory(
#pragma warning(pop)
            hRoblox,
            (LPCVOID)dwAddress,
            pBuffer,
            mbiInfo.RegionSize,
            &szBytesRead
        );

        if ( !bResult ||
             ( szBytesRead != mbiInfo.RegionSize &&
               GetLastError() != ERROR_PARTIAL_COPY ) )
        {
            dwAddress += mbiInfo.RegionSize;

            continue;
        }

        for ( SIZE_T i = 0; i < szBytesRead; i += sizeof( PVOID ) )
        {
            if ( bFastScan )
            {
                if ( ( i + 0xB8 ) >= szBytesRead )
                {
                    break;
                }

                if ( i < 0x48 )
                {
                    continue;
                }

                /*
                    Weird structure RenderView and a few other
                    classes have. Exploitable to scan faster.

                    I've yet to fully figure out what exactly
                    this is. It used to start at +38h, but it
                    has changed to +48h. It may change again.
                    
                    The structure of each value, is three
                    DWORD32s. The first DWORD32 starts off as
                    16, but can be modified. The second DWORD32
                    increments respectively with the frame rate,
                    (higher = faster), and the third DWORD32 is
                    always n bytes larger then the second, with
                    n being the first DWORD32.
                    
                    Setting the first DWORD32 to 1 seems to stop
                    the second DWORD32 from updating.
                */

                if ( *(INT*)( pBuffer + i - 0x48 ) == 0x10 ||
                     *(INT*)( pBuffer + i + 0x48 ) != 0x10 ||
                     *(INT*)( pBuffer + i + 0x80 ) != 0x10 ||
                     *(INT*)( pBuffer + i + 0xB8 ) != 0x10 /* ||
                     *(INT*)( pBuffer + i + 0xF0 ) != 0x10*/ )
                {
                    continue;
                }
            }

            /*
                Check RTTI class
            */

            if ( !IsClass(
                hRoblox,
                (PVOID)( dwAddress + i ),
                ".?AVRenderView@Graphics@RBX@@"
            ) )
            {
                continue;
            }

            /*
                Not needed, probably safe to omit.
            
            if ( !IsClass(
                hRoblox,
                (PVOID)( *(DWORD64*)( pBuffer + i + ( sizeof( PVOID ) * 2 ) ) ),
                ".?AVVisualEngine@Graphics@RBX@@"
            ) )
            {
                continue;
            }
            */

            *ppvRenderViewOut = (PVOID)( dwAddress + i );

            if ( pBuffer )
            {
                (VOID) VirtualFree(
                    pBuffer,
                    0,
                    MEM_RELEASE
                );
            }

            return STATUS_SUCCESS;
        }

        dwAddress += mbiInfo.RegionSize;
    }

    if ( pBuffer )
    {
        (VOID) VirtualFree(
            pBuffer,
            0,
            MEM_RELEASE
        );
    }

    *ppvRenderViewOut = NULL;

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxGetDataModel(
    _In_                                        HANDLE      hRoblox,
    _In_                                        PVOID       pvRenderView,
    _Outptr_result_nullonfailure_   __restrict  PVOID*      ppvDataModelOut
)
{
    DWORD64         dwAddress               = 0;
    DWORD64         dwPointer               = 0;
    DWORD64         dwInvalidDataModel      = 0;
    DWORD64         dwDataModel             = 0;
    
    DWORD           dwMaxSearchDepth        = DATAMODEL_SEARCH_DEPTH;
    DWORD           dwCurrentSearchDepth    = 0;

    static  DWORD   dwIntermediateOffset    = 0;
    static  DWORD   dwDataModelOffset       = 0;

    if ( !hRoblox || !pvRenderView || !ppvDataModelOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvDataModelOut = NULL;

    /*
        Structure:
        
        RBX::RenderView
         - xxx
         - xxx
         - RBX::DataModel (Invalid)     <--- Weird, intermediate pointer.
            - xxx
            - RBX::DataModel (Valid)
    */

    if ( dwIntermediateOffset   &&
         dwDataModelOffset )
    {
        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvRenderView + dwDataModelOffset ),
            &dwPointer,
            sizeof( dwPointer ),
            NULL
        ) )
        {
            return STATUS_UNSUCCESSFUL;
        }

        if ( !dwPointer )
        {
            /*
                This edge case can happen when changing games,
                as Roblox reallocates the DataModel for each
                game session.
            */

            return STATUS_PENDING;
        }

        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)( dwPointer + dwIntermediateOffset ),
            &dwDataModel,
            sizeof( dwDataModel ),
            NULL
        ) )
        {
            return STATUS_UNSUCCESSFUL;
        }

        if ( !dwDataModel )
        {
            return STATUS_PENDING;
        }

        *ppvDataModelOut = (PVOID)dwDataModel;

        return STATUS_SUCCESS;
    }

    dwAddress = (DWORD64)pvRenderView;

lblRGDMSearch:
    /*
        Linear scan to find the pointer to RBX::DataModel.
    */

    while ( dwCurrentSearchDepth < dwMaxSearchDepth )
    {
        dwAddress               += sizeof( PVOID );
        dwCurrentSearchDepth    += sizeof( PVOID );

        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)dwAddress,
            &dwPointer,
            sizeof( dwPointer ),
            NULL
        ) )
        {
            return STATUS_UNSUCCESSFUL;
        }

        if ( dwPointer == 0 ||
             !IsClass(
                 hRoblox,
                 (PVOID)dwPointer,
                 ".?AVDataModel@RBX@@"
             ) )
        {
            continue;
        }

        if ( dwInvalidDataModel == 0 )
        {
            /*
                RBX::RenderView -> RBX::DataModel (Intermediate) -> RBX::DataModel.
            */

            dwInvalidDataModel      = dwPointer;
            dwDataModelOffset       = dwCurrentSearchDepth;

            dwAddress               = dwInvalidDataModel;
            dwCurrentSearchDepth    = 0;

            /*
                Restart search from Invalid DataModel.
            */

            goto lblRGDMSearch;
        }
        else
        {
            dwDataModel             = dwPointer;
            dwIntermediateOffset    = dwCurrentSearchDepth;
        }

        break;
    }

    if ( !dwDataModel || !dwInvalidDataModel )
    {
        return STATUS_NOT_FOUND;
    }

    *ppvDataModelOut = (PVOID)dwDataModel;

    return STATUS_SUCCESS;
}

_Success_( return == STATUS_SUCCESS )
_Must_inspect_result_
NTSTATUS
RobloxBatchGetChildren(
    _In_    HANDLE  hRoblox,
    _In_    PVOID   pChildrenStart,
    _Outptr_result_bytebuffer_maybenull_( dwChildrenAmount * ( sizeof( PVOID ) * 2 ) )
            PVOID*  ppvChildrenOutBuff,
    _In_    DWORD   dwChildrenAmount
)
{
    if ( !hRoblox || !pChildrenStart || !ppvChildrenOutBuff )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( dwChildrenAmount == 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvChildrenOutBuff = NULL;

    PVOID   pChildrenBuff       = NULL;
    SIZE_T  szChildrenBuffSize  = dwChildrenAmount * ( sizeof( PVOID ) * 2 ); /* Child + Deleter */

    pChildrenBuff = VirtualAlloc(
        NULL,
        szChildrenBuffSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if ( !pChildrenBuff )
    {
        return STATUS_NO_MEMORY;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        pChildrenStart,
        pChildrenBuff,
        szChildrenBuffSize,
        NULL
    ) )
    {
        (VOID)VirtualFree(
            pChildrenBuff,
            0,
            MEM_RELEASE
        );
        
        return STATUS_UNSUCCESSFUL;
    }

    *ppvChildrenOutBuff = pChildrenBuff;

    return STATUS_SUCCESS;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxFindFirstChildOfRTTIClass(
    _In_                                        HANDLE      hRoblox,
    _In_                                        PVOID       pvInstance,
    _In_                                        DWORD       dwChildrenOffset,
    _In_                                        PCSTR       pszRTTIClassName,
    _Outptr_result_nullonfailure_   __restrict  PVOID*      ppvChildOut
)
{
    if ( !hRoblox || !pvInstance || !pszRTTIClassName || !ppvChildOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvChildOut = NULL;

    /*
        RBX::Instance
         - xxx
         - xxx
         - Pointer to children              (PVOID)
            - Pointer to start of list      (PVOID)
                - Ptr to child 1            (PVOID)
                - Ptr to child 1 deleter    (PVOID)
                - Ptr to child 2            (PVOID)
                - Ptr to child 2 deleter    (PVOID)
                ...
            - Pointer to end of list        (PVOID)
    */

    NTSTATUS    lStatus                 = STATUS_SUCCESS;

    PVOID       pvChildrenClassPtr      = NULL;
    PVOID       pvChildrenStart         = NULL;
    PVOID       pvChildrenEnd           = NULL;
    PVOID       pChildrenBuff           = NULL;

    PVOID       pvChild                 = NULL;


    DWORD       dwChildrenCount         = 0;

    PVOID       pPointersBuff[2]        = { 0 };

    if ( !ReadProcessMemory(
        hRoblox,
        (LPCVOID)( (DWORD64)pvInstance + dwChildrenOffset ),
        &pvChildrenClassPtr,
        sizeof( pvChildrenClassPtr ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !pvChildrenClassPtr )
    {
        return STATUS_NOT_FOUND;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        pvChildrenClassPtr,
        &pPointersBuff,
        sizeof( pPointersBuff ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    pvChildrenStart     = pPointersBuff[0];
    pvChildrenEnd       = pPointersBuff[1];

    dwChildrenCount     = 
        (DWORD)( (DWORD64)pvChildrenEnd - (DWORD64)pvChildrenStart ) / ( sizeof( PVOID ) * 2 );

    if ( dwChildrenCount == 0 )
    {
        return STATUS_NOT_FOUND;
    }

    if ( !pvChildrenStart || !pvChildrenEnd || pvChildrenStart >= pvChildrenEnd )
    {
        return STATUS_NOT_FOUND;
    }

    lStatus = RobloxBatchGetChildren(
        hRoblox,
        pvChildrenStart,
        &pChildrenBuff,
        dwChildrenCount
    );

    if ( !NT_SUCCESS( lStatus ) || !pChildrenBuff )
    {
        return lStatus;
    }

    for ( DWORD i = 0; i < dwChildrenCount; i++ )
    {
        pvChild = *(PVOID*)( (DWORD64)pChildrenBuff + ( i * sizeof( PVOID ) * 2 ) );
        
        if ( !pvChild )
        {
            continue;
        }
        
        if ( IsClass(
            hRoblox,
            pvChild,
            pszRTTIClassName
        ) )
        {
            *ppvChildOut = pvChild;
            
            (VOID)VirtualFree(
                pChildrenBuff,
                0,
                MEM_RELEASE
            );
            
            return STATUS_SUCCESS;
        }
    }

    (VOID)VirtualFree(
        pChildrenBuff,
        0,
        MEM_RELEASE
    );

    return STATUS_NOT_FOUND;
}

_Success_( return == STATUS_SUCCESS )
NTSTATUS
RobloxReadString(
    _In_                            HANDLE          hRoblox,
    _In_                            PVOID           pvInstance,
    _In_                            DWORD           dwStringOffset,
    _In_                            DWORD           dwBufferSize,
    _Out_writes_z_( dwBufferSize )  PCHAR           pszOutBuffer
)
{
    ROBLOX_STRING       sRbxString      = { 0 };
    PCHAR               pszString       = NULL;
    PVOID               pvStringPtr     = NULL;
    SIZE_T              szStringSize    = 0;
    NTSTATUS            lStatus         = STATUS_UNSUCCESSFUL;

    if ( !hRoblox || !pvInstance || !pszOutBuffer || dwBufferSize == 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( dwBufferSize < 16 )
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    /*
        Structure:

        RBX::Instance
         - ...
         - Pointer to string (dwStringOffset)
            - ROBLOX_STRING
           {
                - Union:
                    - PCHAR pszLongString;
                    - CHAR  szShortString[16];
                - DWORD32 dwStringLen;
           }
    */

    if ( !ReadProcessMemory(
        hRoblox,
        (LPCVOID)( (DWORD64)pvInstance + dwStringOffset ),
        &pvStringPtr,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        (LPCVOID)pvStringPtr,
        &sRbxString,
        sizeof( ROBLOX_STRING ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( sRbxString.dwStringLen == 0 )
    {
        pszOutBuffer[0] = '\0';
        
        return STATUS_SUCCESS;
    }

    if ( sRbxString.dwStringLen > ROBLOX_STRING_MAX_LEN )
    {
        /*
            Should never happen... but just in case.
        */

        return STATUS_INTERNAL_ERROR;
    }

    if ( sRbxString.dwStringLen <= 15 )
    {
        /*
            Short string optimization.
        */

        return ( StringCchCopyNA(
            pszOutBuffer,
            dwBufferSize,
            sRbxString.szString.szShortString,
            sRbxString.dwStringLen
        ) == S_OK ) ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;
    }

    /*
        Long string.
    */

    pszString = sRbxString.szString.pszLongString;

    if ( !IS_VALID_POINTER( hRoblox, pszString ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( sRbxString.dwStringLen < dwBufferSize )
    {
        szStringSize    = sRbxString.dwStringLen;
        lStatus         = STATUS_SUCCESS;
    }
    else
    {
        szStringSize    = dwBufferSize - 1;
        lStatus         = STATUS_BUFFER_TOO_SMALL;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        (LPCVOID)pszString,
        pszOutBuffer,
        szStringSize,
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

#pragma warning(suppress: 6386) /* pszOutBuffer is guaranteed to be at least szStringSize + 1... seriously, MSVC... */
    pszOutBuffer[szStringSize] = '\0';

    return lStatus;
}
