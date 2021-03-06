/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    main.c

Abstract:

    This module is the main entry point for the TestInjection executable.
    It implements mainCRTStartup(), which simply calls TestInjectionMain().

--*/

#include "stdafx.h"

VOID
WINAPI
mainCRTStartup()
{
    ExitProcess(TestInjectionMain());
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
