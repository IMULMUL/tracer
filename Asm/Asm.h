/*++

Copyright (c) 2016 Trent Nelson <trent@trent.me>

Module Name:

    Asm.h

Abstract:

    This is the main header file for the Asm (Assembly) component.

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASM_INTERNAL_BUILD

//
// This is an internal build of the Asm component.
//

#define ASM_API __declspec(dllexport)
#define ASM_DATA extern __declspec(dllexport)

#include "stdafx.h"

#elif _ASM_NO_API_EXPORT_IMPORT

//
// We're being included by someone who doesn't want dllexport or dllimport.
// This is useful for creating new .exe-based projects for things like unit
// testing or performance testing/profiling.
//

#define ASM_API
#define ASM_DATA extern

#else

//
// We're being included by an external component.
//

#define ASM_API __declspec(dllimport)
#define ASM_DATA extern __declspec(dllimport)

#endif

typedef struct _PAGE_COPY_TYPE {
    ULONG Movsb:1;
    ULONG Movsw:1;
    ULONG Movsq:1;
    ULONG Avx2C:1;
    ULONG Avx2NonTemporal:1;
} PAGE_COPY_TYPE, *PPAGE_COPY_TYPE;

typedef
VOID
(COPY_PAGES)(
    _Out_writes_bytes_all_(NumberOfPages << PAGE_SHIFT) PCHAR Dest,
    _In_reads_bytes_(NumberOfPages << PAGE_SHIFT) _Const_ PCHAR Source,
    _In_ ULONG NumberOfPages
    );
typedef COPY_PAGES *PCOPY_PAGES;

typedef
VOID
(COPY_PAGES_EX)(
    _Out_writes_bytes_all_(NumberOfPages << PAGE_SHIFT) PCHAR Dest,
    _In_reads_bytes_(NumberOfPages << PAGE_SHIFT) _Const_ PCHAR Source,
    _In_ ULONG NumberOfPages,
    _Out_opt_ PPAGE_COPY_TYPE PageCopyType
    );
typedef COPY_PAGES_EX *PCOPY_PAGES_EX;


//
// Disable browsing information generation when declaring instances of
// functions; if we don't do this, 'Go To Definition' ends up here, instead
// of the implementation body in the relevant .c file.
//

#pragma component(browser, off)

ASM_API COPY_PAGES CopyPagesNonTemporalAvx2_v1;
ASM_API COPY_PAGES CopyPagesNonTemporalAvx2_v2;

#pragma component(browser, on)

#ifdef __cplusplus
} // extern "C"
#endif

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab nowrap                              :
