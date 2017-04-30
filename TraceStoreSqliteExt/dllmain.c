/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    dllmain.c

Abstract:

    This is the DLL main entry point for the TraceStoreSqliteExt DLL.

--*/

#include "stdafx.h"

BOOL
APIENTRY
_DllMainCRTStartup(
    _In_    HMODULE     Module,
    _In_    DWORD       Reason,
    _In_    LPVOID      Reserved
    )
{
    switch (Reason) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            OutputDebugStringA("TraceStoreSqliteExt!_DllMainCRTStartup(): "
                               "DLL_THREAD_ATTACH.\n");
            break;
        case DLL_THREAD_DETACH:
            OutputDebugStringA("TraceStoreSqliteExt!_DllMainCRTStartup(): "
                               "DLL_THREAD_DETACH.\n");
            break;
        case DLL_PROCESS_DETACH:
            OutputDebugStringA("TraceStoreSqliteExt!_DllMainCRTStartup(): "
                               "DLL_PROCESS_DETACH.\n");
            break;
    }

    return TRUE;
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
