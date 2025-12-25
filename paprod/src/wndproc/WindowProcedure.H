#pragma once

#include <stdio.h>
#include <dumper/InitDump.H>

#define IDC_CHECKBOX_CLIENT                 101
#define IDC_CHECKBOX_SUSPEND		        102
#define IDC_CHECKBOX_FAST_RENDERVIEW_SCAN   103

#define IDC_BUTTON_DUMP                     104
#define IDC_BUTTON_HELP                     105

#define IDC_RICHEDIT_OUTPUT                 106

LRESULT
CALLBACK
WindowProcedureW(
    _In_    HWND    hWnd,
    _In_    UINT    uMsg,
    _In_    WPARAM  wParam,
    _In_    LPARAM  lParam
);