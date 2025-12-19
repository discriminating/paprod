/*
File:       Roblox.C
Purpose:    Functions to interact with Roblox client
Author:     @discriminating
Date:       18 December 2025
*/

#include <minrblx/Roblox.H>

DWORD   g_dwRenderToInvalid     = 0x0;
DWORD   g_dwInvalidToReal       = 0x0;

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

    if ( !hRoblox || hRoblox == INVALID_HANDLE_VALUE || !ppvRenderViewOut )
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvRenderViewOut = NULL;

    /*
        Walk the heap for RenderView's class instance...
    */

    while ( dwAddress < ARBITRARY_HEAP_BARRIER )
    {
        SIZE_T      szBytesReturned     = 0;
        SIZE_T      szBytesRead         = 0;
        BOOL        bResult             = FALSE;
        PBYTE       pBuffer             = NULL;

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
            (VOID)VirtualFree(
                pBuffer,
                0,
                MEM_RELEASE
            );

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

                    Sometimes changes.
                */

                if ( *(INT*)( pBuffer + i - 0x48 ) == 0x10 ||
                     *(INT*)( pBuffer + i + 0x48 ) != 0x10 ||
                     *(INT*)( pBuffer + i + 0x80 ) != 0x10 ||
                     *(INT*)( pBuffer + i + 0xB8 ) != 0x10 )
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
                Third pointer is a pointer to VisualEngine
            */

            if ( !IsClass(
                hRoblox,
                (PVOID)( *(DWORD64*)( pBuffer + i + ( sizeof( PVOID ) * 2 ) ) ),
                ".?AVVisualEngine@Graphics@RBX@@"
            ) )
            {
                continue;
            }

            *ppvRenderViewOut = (PVOID)( dwAddress + i );

            (VOID)VirtualFree(
                pBuffer,
                0,
                MEM_RELEASE
            );

            return STATUS_SUCCESS;
        }

        (VOID)VirtualFree(
            pBuffer,
            0,
            MEM_RELEASE
        );

        dwAddress += mbiInfo.RegionSize;
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
    DWORD64     dwAddress               = 0;
    DWORD64     dwPointer               = 0;
    DWORD64     dwInvalidDataModel      = 0;
    DWORD64     dwDataModel             = 0;
    
    DWORD       dwMaxSearchDepth        = 0x400; /* Arbitrary... bad, bad, bad... */
    DWORD       dwCurrentSearchDepth    = 0;

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

    if ( g_dwInvalidToReal     &&
         g_dwRenderToInvalid )
    {
        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)( (DWORD64)pvRenderView + g_dwRenderToInvalid ),
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
                When changing games Roblox deletes the old
                DataModel and creates a new one, so caller
                needs to wait for it to be created.
            */

            return STATUS_PENDING;
        }

        if ( !ReadProcessMemory(
            hRoblox,
            (LPCVOID)( dwPointer + g_dwInvalidToReal ),
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

            dwInvalidDataModel          = dwPointer;
            g_dwRenderToInvalid         = dwCurrentSearchDepth;

            dwAddress                   = dwInvalidDataModel;
            dwCurrentSearchDepth        = 0;

            /*
                Restart search from Invalid DataModel.
            */

            goto lblRGDMSearch;
        }
        else
        {
            dwDataModel                 = dwPointer;
            g_dwInvalidToReal           = dwCurrentSearchDepth;
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