/*++

Copyright (c) 2018 Trent Nelson <trent@trent.me>

Module Name:

    main.c

Abstract:

    This is the main file for the PerfectHashTableSelfTest component..

--*/

#include "stdafx.h"

//
// Define globals.
//

RTL GlobalRtl;
ALLOCATOR GlobalAllocator;

PRTL Rtl;
PALLOCATOR Allocator;

HMODULE GlobalModule = 0;

PERFECT_HASH_TABLE_API_EX GlobalApi;
PPERFECT_HASH_TABLE_API_EX Api;

HMODULE GlobalRtlModule = 0;
HMODULE GlobalPerfectHashTableModule = 0;

//
// Main entry point.
//

DECLSPEC_NORETURN
VOID
WINAPI
mainCRTStartup()
{
    BOOL Success;
    LONG ExitCode;
    LONG SizeOfRtl = sizeof(GlobalRtl);
    ULONG OldCodePage;
    HMODULE RtlModule;
    HANDLE StdErrorHandle;
    RTL_BOOTSTRAP Bootstrap;
    HMODULE Shell32Module = NULL;
    PCOMMAND_LINE_TO_ARGVW CommandLineToArgvW;
    PWSTR CommandLineW;
    LONG NumberOfArguments;
    PPSTR ArgvA;
    PPWSTR ArgvW;
    const STRING Usage = RTL_CONSTANT_STRING(
        "Usage: PerfectHashTableSelfTest.exe <Test Data Directory> "
        "<AlgorithmId (1)> "
        "<HashFunctionId (1-4)> "
        "<MaskFunctionId (1-4)> "
        "<MaximumConcurrency (0-ncpu)> "
        "<NumberOfTableElements (0..)>\n"
    );
    UNICODE_STRING Path;
    PPERFECT_HASH_TABLE_ANY_API AnyApi;

    if (!BootstrapRtl(&RtlModule, &Bootstrap)) {
        ExitCode = 1;
        goto Error;
    }

    Success = Bootstrap.InitializeHeapAllocatorEx(&GlobalAllocator,
                                                  HEAP_GENERATE_EXCEPTIONS,
                                                  0,
                                                  0);

    if (!Success) {
        ExitCode = 1;
        goto Error;
    }

    CHECKED_MSG(
        Bootstrap.InitializeRtl(&GlobalRtl, &SizeOfRtl),
        "InitializeRtl()"
    );

    Rtl = &GlobalRtl;
    Allocator = &GlobalAllocator;
    Api = &GlobalApi;
    AnyApi = (PPERFECT_HASH_TABLE_ANY_API)&GlobalApi;

    SetCSpecificHandler(Rtl->__C_specific_handler);

    ASSERT(LoadPerfectHashTableApi(Rtl,
                                   &GlobalPerfectHashTableModule,
                                   NULL,
                                   sizeof(GlobalApi),
                                   AnyApi));

    //
    // Extract the command line for the current process.
    //

    LOAD_LIBRARY_A(Shell32Module, Shell32);

    RESOLVE_FUNCTION(CommandLineToArgvW,
                     Shell32Module,
                     PCOMMAND_LINE_TO_ARGVW,
                     CommandLineToArgvW);

    CHECKED_MSG(CommandLineW = GetCommandLineW(), "GetCommandLineW()");

    ArgvW = CommandLineToArgvW(CommandLineW, &NumberOfArguments);

    CHECKED_MSG(ArgvW, "Shell32!CommandLineToArgvW()");

    CHECKED_MSG(
        Rtl->ArgvWToArgvA(
            ArgvW,
            NumberOfArguments,
            &ArgvA,
            NULL,
            Allocator
        ),
        "Rtl!ArgvWToArgA"
    );

    switch (NumberOfArguments) {
        case 7: {

            ULONG MaxConcurrency;
            ULARGE_INTEGER NumberOfTableElements;
            PERFECT_HASH_TABLE_ALGORITHM_ID AlgorithmId;
            PERFECT_HASH_TABLE_HASH_FUNCTION_ID HashFunctionId;
            PERFECT_HASH_TABLE_MASK_FUNCTION_ID MaskFunctionId;

            //
            // Extract test data directory.
            //

            Path.Buffer = ArgvW[1];
            Path.Length = (USHORT)wcslen(Path.Buffer) << 1;
            Path.MaximumLength = Path.Length + sizeof(Path.Buffer[0]);

            //
            // Extract algorithm ID.
            //

            if (FAILED(Rtl->RtlCharToInteger(ArgvA[2],
                                             10,
                                             (PULONG)&AlgorithmId))) {
                goto PrintUsage;
            }

            //
            // Extract hash function ID.
            //

            if (FAILED(Rtl->RtlCharToInteger(ArgvA[3],
                                             10,
                                             (PULONG)&HashFunctionId))) {
                goto PrintUsage;
            }

            //
            // Extract mask function ID.
            //

            if (FAILED(Rtl->RtlCharToInteger(ArgvA[4],
                                             10,
                                             (PULONG)&MaskFunctionId))) {
                goto PrintUsage;
            }

            //
            // Extract maximum concurrency.
            //

            if (FAILED(Rtl->RtlCharToInteger(ArgvA[5], 10, &MaxConcurrency))) {
                goto PrintUsage;
            }

            //
            // Extract number of table elements.
            //

            NumberOfTableElements.QuadPart = 0;
            if (FAILED(Rtl->RtlCharToInteger(ArgvA[5],
                                             10,
                                             &NumberOfTableElements.LowPart))) {
                goto PrintUsage;
            }


            Success = Api->SelfTestPerfectHashTable(Rtl,
                                                    Allocator,
                                                    AnyApi,
                                                    &PerfectHashTableTestData,
                                                    &Path,
                                                    &MaxConcurrency,
                                                    AlgorithmId,
                                                    HashFunctionId,
                                                    MaskFunctionId,
                                                    &NumberOfTableElements);

            ExitCode = (Success ? 0 : 1);
            break;
        }

PrintUsage:
        default:
            StdErrorHandle = GetStdHandle(STD_ERROR_HANDLE);
            ASSERT(StdErrorHandle);
            OldCodePage = GetConsoleCP();
            ASSERT(SetConsoleCP(20127));
            Success = WriteFile(StdErrorHandle,
                                Usage.Buffer,
                                Usage.Length,
                                NULL,
                                NULL);
            ASSERT(Success);
            SetConsoleCP(OldCodePage);
            ExitCode = 1;
            break;
    }

Error:

    ExitProcess(ExitCode);
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
