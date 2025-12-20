/*
File:		Entry.C
Purpose:	Window setup code, entry point
Author:		@discriminating
Date:		18 December 2025
*/

#include <wndproc/WindowProcedure.H>

INT
APIENTRY
WinMain(
    _In_        HINSTANCE   hInstance,
    _In_opt_    HINSTANCE   hPrevInstance,
    _In_        LPSTR       pCmdLine,
    _In_        INT         nCmdShow
)
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( pCmdLine );

    WNDCLASSEXW     hWndClassEx     = { 0 };
    HWND            hWnd            = NULL;
    MSG             msg             = { 0 };

    hWndClassEx.cbSize          = sizeof( WNDCLASSEXW );
    hWndClassEx.style           = CS_HREDRAW | CS_VREDRAW;
    hWndClassEx.lpfnWndProc     = WindowProcedureW;
    hWndClassEx.hInstance       = hInstance;
    hWndClassEx.lpszClassName   = L"DumperWindow";

    RegisterClassExW( &hWndClassEx );

    hWnd = CreateWindowExW(
        0,
        hWndClassEx.lpszClassName,
        L"Roblox Offset Dumper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        570,
        NULL,
        NULL,
        hWndClassEx.hInstance,
        NULL
    );

    if ( !hWnd )
    {
        MessageBoxW(
            NULL,
            L"Failed to create main window.",
            L"Error",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    ShowWindow(
        hWnd,
        nCmdShow
    );

    while ( GetMessageW( &msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    return 0;
}