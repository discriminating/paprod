/*
File:       WindowProcedure.C
Purpose:    Window procedure for handling window messages
Author:     @discriminating
Date:       18 December 2025
*/

#include <windows.h>
#include <wndproc/WindowProcedure.H>

/*
    Blah, blah, global variables...
*/
HWND g_hRichEditOutput = NULL;

VOID
OutputFormat(
    _In_    LPCWSTR lpFormat,
    ...
)
{
    if ( !g_hRichEditOutput || !lpFormat )
        return;
    
    INT         nLength             = 0;
    WCHAR       szBuffer[ 4096 ]    = { 0 };
    va_list     args;
    
    va_start( args, lpFormat );
    
    (VOID)vswprintf_s(
        szBuffer,
        sizeof( szBuffer ) / sizeof( WCHAR ) - 1,
        lpFormat,
        args
    );
    
    va_end( args );

    nLength = GetWindowTextLengthW( g_hRichEditOutput );

    (VOID)SendMessageW(
        g_hRichEditOutput,
        EM_SETSEL,
        (WPARAM) nLength,
        (LPARAM) nLength
    );

    (VOID)SendMessageW(
        g_hRichEditOutput,
        EM_REPLACESEL,
        FALSE,
        (LPARAM) szBuffer
    );
}

LRESULT
CALLBACK
WindowProcedureW(
    _In_    HWND    hWnd,
    _In_    UINT    uMsg,
    _In_    WPARAM  wParam,
    _In_    LPARAM  lParam
)
{
    static HWND     hCheckboxClient             = NULL;
    static HWND     hCheckboxSuspend            = NULL;
    static HWND     hCheckboxFastScan           = NULL;

    static HWND     hButtonDump                 = NULL;

    switch ( uMsg )
    {
        case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            return 0;
        }

        case WM_CREATE:
        {
            HFONT hFont = NULL;

            /*
                For Rich Edit box...
            */

            LoadLibraryW( L"Msftedit.dll" );

            hCheckboxClient = CreateWindowW(
                L"BUTTON",
                L"Use Roblox Player",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                20,
                20,
                200,
                30,
                hWnd,
                (HMENU) IDC_CHECKBOX_CLIENT,
                ( (LPCREATESTRUCT) lParam )->hInstance,
                NULL
            );

            if ( !hCheckboxClient )
            {
                MessageBoxW(
                    hWnd,
                    L"Failed to create checkbox control.",
                    L"Error",
                    MB_OK | MB_ICONERROR
                );

                return -1;
            }

            hCheckboxSuspend = CreateWindowW(
                L"BUTTON",
                L"Suspend Process During Scan",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                240,
                20,
                250,
                30,
                hWnd,
                (HMENU)IDC_CHECKBOX_SUSPEND,
                ( (LPCREATESTRUCT)lParam )->hInstance,
                NULL
            );

            if ( !hCheckboxSuspend )
            {
                MessageBoxW(
                    hWnd,
                    L"Failed to create checkbox control.",
                    L"Roblox Offset Dumper",
                    MB_OK | MB_ICONERROR
                );

                return -1;
            }

            hCheckboxFastScan = CreateWindowW(
                L"BUTTON",
                L"Use Fast RenderView Scan",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                240,
                60,
                250,
                30,
                hWnd,
                (HMENU)IDC_CHECKBOX_FAST_RENDERVIEW_SCAN,
                ( (LPCREATESTRUCT) lParam )->hInstance,
                NULL
            );

            if ( !hCheckboxFastScan )
            {
                MessageBoxW(
                    hWnd,
                    L"Failed to create checkbox control.",
                    L"Error",
                    MB_OK | MB_ICONERROR
                );
                
                return -1;
            }

            hButtonDump = CreateWindowW(
                L"BUTTON",
                L"Dump Offsets",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20,
                70,
                100,
                30,
                hWnd,
                (HMENU)IDC_BUTTON_DUMP,
                ( (LPCREATESTRUCT)lParam )->hInstance,
                NULL
            );

            if ( !hButtonDump )
            {
                MessageBoxW(
                    hWnd,
                    L"Failed to create button control.",
                    L"Error",
                    MB_OK | MB_ICONERROR
                );

                return -1;
            }

            g_hRichEditOutput = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"RICHEDIT50W",
                L"",
                WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                20,
                110,
                750,
                400,
                hWnd,
                (HMENU) IDC_RICHEDIT_OUTPUT,
                ( (LPCREATESTRUCT)lParam )->hInstance,
                NULL
            );

            hFont = CreateFontW(
                16,
                0,
                0,
                0,
                FW_NORMAL,
                FALSE,
                FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                FIXED_PITCH | FF_MODERN,
                L"Consolas"
            );

            (VOID)SendMessageW(
                g_hRichEditOutput,
                WM_SETFONT,
                (WPARAM) hFont,
                TRUE
            );

            return 0;
        }

        case WM_COMMAND:
        {
            if ( LOWORD( wParam ) == IDC_BUTTON_DUMP )
            {
                BOOL    bUseClient         = FALSE;
                BOOL    bSuspend           = FALSE;
                BOOL    bUseFastScan       = FALSE;

                bUseClient = IsDlgButtonChecked(
                    hWnd,
                    IDC_CHECKBOX_CLIENT
                ) == BST_CHECKED;

                bSuspend = IsDlgButtonChecked(
                    hWnd,
                    IDC_CHECKBOX_SUSPEND
                ) == BST_CHECKED;

                bUseFastScan = IsDlgButtonChecked(
                    hWnd,
                    IDC_CHECKBOX_FAST_RENDERVIEW_SCAN
                ) == BST_CHECKED;

                if ( bUseClient )
                {
                    if ( MessageBoxW(
                        hWnd,
                        L"Are you sure you want to dump from the client?",
                        L"Roblox Offset Dumper",
                        MB_YESNO | MB_ICONQUESTION
                    ) != IDYES )
                    {
                        return 0;
                    }
                }

                OutputFormat(
                    L"Info: Starting dump with CLIENT=%s SUSPEND=%s FAST=%s...\n",
                    ( bUseClient    ? L"TRUE" : L"FALSE" ),
                    ( bSuspend      ? L"TRUE" : L"FALSE" ),
                    ( bUseFastScan  ? L"TRUE" : L"FALSE" )
                );

                InitDump(
                    bUseClient,
                    bSuspend,
                    bUseFastScan
                );

                return 0;
            }
        }

        case WM_PAINT:
        {
            PAINTSTRUCT psPaint     = { 0 };
            HDC         hDC         = { 0 };

            hDC = BeginPaint(
                hWnd,
                &psPaint
            );

            /*
                Will fill the background to the default window color.
            */

            FillRect(
                hDC,
                &psPaint.rcPaint,
                ( HBRUSH )( COLOR_WINDOW + 1 )
            );

            EndPaint(
                hWnd,
                &psPaint
            );

            return 0;

        }

        default:
        {
            return DefWindowProcW(
                hWnd,
                uMsg,
                wParam,
                lParam
            );
        }
    }
}