#include <rtti/RunTimeTypeInformation.H>

BOOL
IsClass(
    _In_    HANDLE          hRoblox,
    _In_    VOID*           pVirtualAddress,
    _In_    CONST CHAR*     szClassName
)
{
    if ( !hRoblox || !pVirtualAddress || !szClassName )
    {
        return FALSE;
    }

    struct RTTICompleteObjectLocator    sRTTI               = { 0 };
    struct TypeDescriptor               sTypeDescriptor     = { 0 };

    PVOID**     pppVTable                       = (PVOID**)pVirtualAddress;
    PVOID       pVTable                         = NULL;
    PVOID       pRTTICompleteObjectLocator      = NULL;

    DWORD64     dwImageBase                     = 0;
    DWORD64     dwTypeDescriptor                = 0;

    if ( !ReadProcessMemory(
        hRoblox,
        pppVTable,
        &pVTable,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return FALSE;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        (PVOID)( (DWORD64)pVTable - sizeof( PVOID ) ),
        &pRTTICompleteObjectLocator,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return FALSE;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        pRTTICompleteObjectLocator,
        &sRTTI,
        sizeof( sRTTI ),
        NULL
    ) )
    {
        return FALSE;
    }

    dwImageBase      = (DWORD64)pRTTICompleteObjectLocator - sRTTI.pSelf;
    dwTypeDescriptor = dwImageBase + sRTTI.pTypeDescriptor;

    if ( !ReadProcessMemory(
        hRoblox,
        (PVOID)dwTypeDescriptor,
        &sTypeDescriptor,
        16 + 127 + 1, /* First 2 pointers, max length of class name, null terminator. */
        NULL
    ) )
    {
        return FALSE;
    }

    return strcmp(
        sTypeDescriptor.szName,
        szClassName
    ) == 0;
}

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

    struct RTTICompleteObjectLocator    sRTTI               = { 0 };
    struct TypeDescriptor               sTypeDescriptor     = { 0 };

    PVOID**     pppVTable                       = (PVOID**)pVirtualAddress;
    PVOID       pVTable                         = NULL;
    PVOID       pRTTICompleteObjectLocator      = NULL;

    DWORD64     dwImageBase                     = 0;
    DWORD64     dwTypeDescriptor                = 0;

    if ( !ReadProcessMemory(
        hRoblox,
        pppVTable,
        &pVTable,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return FALSE;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        (PVOID)( (DWORD64)pVTable - sizeof( PVOID ) ),
        &pRTTICompleteObjectLocator,
        sizeof( PVOID ),
        NULL
    ) )
    {
        return FALSE;
    }

    if ( !ReadProcessMemory(
        hRoblox,
        pRTTICompleteObjectLocator,
        &sRTTI,
        sizeof( sRTTI ),
        NULL
    ) )
    {
        return FALSE;
    }

    dwImageBase      = (DWORD64)pRTTICompleteObjectLocator - sRTTI.pSelf;
    dwTypeDescriptor = dwImageBase + sRTTI.pTypeDescriptor;

    if ( !ReadProcessMemory(
        hRoblox,
        (PVOID)dwTypeDescriptor,
        &sTypeDescriptor,
        16 + 127 + 1, /* See IsClass... */
        NULL
    ) )
    {
        return FALSE;
    }

    /*
        Return true if class name ends with @RBX@@
    */

    SIZE_T          nClassNameLen = strlen( sTypeDescriptor.szName );
    SIZE_T          nRBXSuffixLen = sizeof( "@RBX@@" ) - 1;

    if ( nClassNameLen < nRBXSuffixLen )
    {
        return FALSE;
    }

    return memcmp(
        sTypeDescriptor.szName + ( nClassNameLen - nRBXSuffixLen ),
        "@RBX@@",
        nRBXSuffixLen
    ) == 0;
}