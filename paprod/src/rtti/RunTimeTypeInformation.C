/*
File:       RunTimeTypeInformation.C
Purpose:    Functions related to RTTI
Author:     @discriminating
Date:       22 December 2025
*/

#include <rtti/RunTimeTypeInformation.H>

_Success_( return == STATUS_SUCCESS )
NTSTATUS
GetClass(
    _In_                            HANDLE          hRoblox,
    _In_                            PVOID           pVirtualAddress,
    _In_                            DWORD           dwBufferSize,
    _Out_writes_z_( dwBufferSize )
    _When_( return == STATUS_SUCCESS, _Null_terminated_ )
                                    CHAR*           pszOutBuffer
)
{
    if ( !hRoblox || !pVirtualAddress || !dwBufferSize )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( hRoblox == INVALID_HANDLE_VALUE || dwBufferSize == 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( dwBufferSize < TYPE_DESCRIPTOR_NAME_SIZE )
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *pszOutBuffer = '\0';

    struct RTTICompleteObjectLocator    sRTTI               = { 0 };
    struct TypeDescriptor               sTypeDescriptor     = { 0 };

    PVOID**     pppVTable                       = (PVOID**)pVirtualAddress;
    PVOID       pVTable                         = NULL;
    PVOID       pRTTICompleteObjectLocator      = NULL;

    DWORD64     pImageBase                      = 0;
    DWORD64     pTypeDescriptor                 = 0;

    if ( !ReadProcessMemory(
        hRoblox,
        pppVTable,
        &pVTable,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !pVTable )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !IS_VALID_POINTER( hRoblox, pVTable ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        (PVOID)( (DWORD64)pVTable - sizeof( PVOID ) ),
        &pRTTICompleteObjectLocator,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !IS_VALID_POINTER( hRoblox, pRTTICompleteObjectLocator ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        pRTTICompleteObjectLocator,
        &sRTTI,
        sizeof( sRTTI ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    if ( sRTTI.dwSignature != 1 ) /* x64 */
    {
        return STATUS_UNSUCCESSFUL;
    }

    pImageBase         = (DWORD64)pRTTICompleteObjectLocator - sRTTI.pSelf;
    pTypeDescriptor    = pImageBase + sRTTI.pTypeDescriptor;

    if ( !ReadProcessMemory(
        hRoblox,
        (PVOID)pTypeDescriptor,
        &sTypeDescriptor,
        sizeof( struct TypeDescriptor ),
        NULL
    ) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    sTypeDescriptor.szName[TYPE_DESCRIPTOR_NAME_SIZE - 1] = '\0';

    return ( StringCchCopyNA(
        pszOutBuffer,
        dwBufferSize,
        sTypeDescriptor.szName,
        TYPE_DESCRIPTOR_NAME_SIZE - 1
    ) == S_OK ) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

_Success_( return != 0 )
BOOL
IsClass(
    _In_        HANDLE          hRoblox,
    _In_        VOID*           pVirtualAddress,
    _In_z_      CONST CHAR*     szClassName
)
{
    if ( !hRoblox || !pVirtualAddress || !szClassName )
    {
        return FALSE;
    }

    CHAR    szClassNameRead[TYPE_DESCRIPTOR_NAME_SIZE]  = { 0 };

    if ( !NT_SUCCESS( GetClass(
        hRoblox,
        pVirtualAddress,
        TYPE_DESCRIPTOR_NAME_SIZE,
        szClassNameRead
    ) ) )
    {
        return FALSE;
    }

    return strcmp(
        szClassName,
        szClassNameRead
    ) == 0;
}

_Success_( return != 0 )
BOOL
IsRobloxClass(
    _In_    HANDLE          hRoblox,
    _In_    VOID*           pVirtualAddress
)
{
    if ( !hRoblox || !pVirtualAddress)
    {
        return FALSE;
    }

    CHAR    szClassNameRead[TYPE_DESCRIPTOR_NAME_SIZE]  = { 0 };

    if ( !NT_SUCCESS( GetClass(
        hRoblox,
        pVirtualAddress,
        TYPE_DESCRIPTOR_NAME_SIZE,
        szClassNameRead
    ) ) )
    {
        return FALSE;
    }

    /*
        Return true if class name ends with @RBX@@
    */

    SIZE_T          nClassNameLen = strlen( szClassNameRead );
    SIZE_T          nRBXSuffixLen = sizeof( "@RBX@@" ) - 1;

    if ( nClassNameLen < nRBXSuffixLen )
    {
        return FALSE;
    }

    return memcmp(
        szClassNameRead + ( nClassNameLen - nRBXSuffixLen ),
        "@RBX@@",
        nRBXSuffixLen
    ) == 0;
}
