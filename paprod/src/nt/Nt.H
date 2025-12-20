#pragma once

#include <windows.h>
#include <winternl.h>
#pragma warning(push)
#pragma warning(disable:4005)
#include <ntstatus.h>
#pragma warning(pop)

#pragma comment(lib, "ntdll.lib")

#define MAX_MEMORY_ADDR     0x7FFFFFFFFFFF
#define PAGE_SIZE           0x1000
