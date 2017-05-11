/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    TraceStoreSqlite3Module.c

Abstract:

    WIP.

--*/

#include "stdafx.h"

//
// This is required in order to link without the CRT.
//

LONG _fltused;

//
// Define the default Trace Store sqlite3 module function pointers.
//

SQLITE3_MODULE TraceStoreSqlite3Module = {
    0,  // Version
    0,  // Padding
    TraceStoreSqlite3ModuleCreate,
    TraceStoreSqlite3ModuleCreate,
    TraceStoreSqlite3ModuleBestIndex,
    TraceStoreSqlite3ModuleDisconnect,
    NULL, // TraceStoreSqlite3ModuleDestroy,
    TraceStoreSqlite3ModuleOpenCursor,
    TraceStoreSqlite3ModuleCloseCursor,
    TraceStoreSqlite3ModuleFilter,
    TraceStoreSqlite3ModuleNext,
    TraceStoreSqlite3ModuleEof,
    TraceStoreSqlite3ModuleColumn,
    TraceStoreSqlite3ModuleRowid,
    NULL, // TraceStoreSqlite3ModuleUpdate,
    NULL, // TraceStoreSqlite3ModuleBegin,
    NULL, // TraceStoreSqlite3ModuleSync,
    NULL, // TraceStoreSqlite3ModuleCommit,
    NULL, // TraceStoreSqlite3ModuleRollback,
    TraceStoreSqlite3ModuleFindFunction,
    NULL, // TraceStoreSqlite3ModuleRename,
    NULL, // TraceStoreSqlite3ModuleSavepoint,
    NULL, // TraceStoreSqlite3ModuleRelease,
    NULL, // TraceStoreSqlite3ModuleRollbackTo,
};

//
// Define a dummy column function used by default.
//

_Use_decl_annotations_
LONG
TraceStoreSqlite3DefaultColumnImpl(
    PCSQLITE3 Sqlite3,
    PTRACE_STORE TraceStore,
    PTRACE_STORE_SQLITE3_CURSOR Cursor,
    PSQLITE3_CONTEXT Context,
    LONG ColumnNumber
    )
{
    Sqlite3->ResultNull(Context);
    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleCreate(
    PSQLITE3_DB Sqlite3Db,
    PVOID Aux,
    LONG NumberOfArguments,
    PCSZ Arguments,
    PSQLITE3_VTAB *VirtualTable
    )
{
    ULONG Failed;
    PCSQLITE3 Sqlite3;
    PTRACE_STORE TraceStore;
    PTRACE_STORE_SQLITE3_DB Db;

    TraceStore = (PTRACE_STORE)Aux;
    Db = TraceStore->Db;
    Sqlite3 = Db->Sqlite3;

    Failed = Sqlite3->DeclareVirtualTable(Sqlite3Db, TraceStore->Sqlite3Schema);
    if (Failed) {
        __debugbreak();
        return Failed;
    }

    *VirtualTable = &TraceStore->Sqlite3VirtualTable.AsSqlite3Vtab;

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleBestIndex(
    PSQLITE3_VTAB VirtualTable,
    PSQLITE3_INDEX_INFO IndexInfo
    )
{
    LONG Index;
    PCSQLITE3 Sqlite3;
    PTRACE_STORE TraceStore;
    PTRACE_STORE_SQLITE3_DB Db;
    PSQLITE3_INDEX_CONSTRAINT Constraint;

    TraceStore = CONTAINING_RECORD(VirtualTable,
                                   TRACE_STORE,
                                   Sqlite3VirtualTable);

    Db = TraceStore->Db;
    Sqlite3 = Db->Sqlite3;

    for (Index = 0; Index < IndexInfo->NumberOfConstraints; Index++) {
        Constraint = IndexInfo->Constraints + Index;
        if (!Constraint->IsUsable) {
            continue;
        }
    }

    IndexInfo->IndexNumber = 1;
    IndexInfo->EstimatedCost = 1.0;
    IndexInfo->EstimatedRows = TraceStore->Totals->NumberOfRecords.QuadPart;

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleDisconnect(
    PSQLITE3_VTAB VirtualTable
    )
{
    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleDestroy(
    PSQLITE3_VTAB VirtualTable
    )
{
    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleOpenCursor(
    PSQLITE3_VTAB VirtualTable,
    PSQLITE3_VTAB_CURSOR *CursorPointer
    )
{
    PCSQLITE3 Sqlite3;
    PTRACE_STORE TraceStore;
    TRACE_STORE_TRAITS Traits;
    PTRACE_STORE_TOTALS Totals;
    PTRACE_STORE_SQLITE3_DB Db;
    PTRACE_STORE_SQLITE3_CURSOR Cursor;

    TraceStore = CONTAINING_RECORD(VirtualTable,
                                   TRACE_STORE,
                                   Sqlite3VirtualTable);

    Db = TraceStore->Db;
    Sqlite3 = Db->Sqlite3;
    Totals = TraceStore->Totals;

    Cursor = (PTRACE_STORE_SQLITE3_CURSOR)Sqlite3->Malloc(sizeof(*Cursor));
    if (!Cursor) {
        return SQLITE_NOMEM;
    }

    ZeroStructPointer(Cursor);

    //
    // Initialize the cursor.
    //

    Cursor->Db = Db;
    Cursor->TraceStore = TraceStore;
    Traits = Cursor->Traits = *TraceStore->pTraits;
    Cursor->Sqlite3 = Sqlite3;
    Cursor->Sqlite3Column = TraceStore->Sqlite3Column;
    Cursor->TotalNumberOfAllocations = Totals->NumberOfAllocations.QuadPart;

    if (!TraceStore->IsMetadata) {
        Cursor->Address = TraceStore->ReadonlyAddresses;
        Cursor->MemoryMap = &TraceStore->FlatMemoryMap;
        Cursor->AddressRange = TraceStore->ReadonlyAddressRanges;
        Cursor->TotalNumberOfRecords = Totals->NumberOfRecords.QuadPart;

        if (IsFixedRecordSize(Traits)) {
            if (IsRecordSizeAlwaysPowerOf2(Traits)) {
                Cursor->Flags.FixedRecordSizeAndAlwaysPowerOf2 = TRUE;
                Cursor->TraceStoreRecordSizeInBytes = (
                    Totals->AllocationSize.QuadPart /
                    Totals->NumberOfAllocations.QuadPart
                );
            }
        }

    } else {
        Cursor->Flags.IsMetadataStore = TRUE;
        Cursor->MemoryMap = &TraceStore->SingleMemoryMap;
        Cursor->TotalNumberOfRecords = Cursor->TotalNumberOfAllocations;
        Cursor->MetadataRecordSizeInBytes = (
            Totals->AllocationSize.QuadPart /
            Totals->NumberOfAllocations.QuadPart
        );
    }

    //
    // Check for the rare case that our flat mapping hasn't finished loading
    // yet.
    //

    if (!TraceStore->FlatMappingLoaded) {
        ULONG WaitResult;
        HANDLE Event;

        Event = Db->TraceContext->LoadingCompleteEvent;
        WaitResult = WaitForSingleObject(Event, INFINITE);

        if (WaitResult != WAIT_OBJECT_0) {
            __debugbreak();
            return SQLITE_ERROR;
        }

        if (Db->TraceContext->FailedCount > 0) {
            __debugbreak();
            return SQLITE_ERROR;
        }
    }

    Cursor->FirstRow.AsVoid = Cursor->MemoryMap->BaseAddress;
    Cursor->CurrentRow.AsVoid = Cursor->MemoryMap->BaseAddress;

    QueryPerformanceCounter(&Cursor->OpenedTimestamp);

    *CursorPointer = &Cursor->AsSqlite3VtabCursor;

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleCloseCursor(
    PSQLITE3_VTAB_CURSOR Sqlite3Cursor
    )
{
    LARGE_INTEGER CloseTimestamp;
    LARGE_INTEGER Elapsed;
    PTRACE_STORE_SQLITE3_CURSOR Cursor;

    Cursor = CONTAINING_RECORD(Sqlite3Cursor,
                               TRACE_STORE_SQLITE3_CURSOR,
                               AsSqlite3VtabCursor);

    TraceTimeQueryPerformanceCounter(Cursor->TraceStore->Time,
                                     &Elapsed,
                                     &CloseTimestamp);

    Cursor->Sqlite3->Free(Cursor);

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleFilter(
    PSQLITE3_VTAB_CURSOR Sqlite3Cursor,
    LONG IndexNumber,
    PCSZ IndexString,
    LONG ArgumentCount,
    PSQLITE3_VALUE *Arguments
    )
{
    PTRACE_STORE_SQLITE3_CURSOR Cursor;

    Cursor = CONTAINING_RECORD(Sqlite3Cursor,
                               TRACE_STORE_SQLITE3_CURSOR,
                               AsSqlite3VtabCursor);

    Cursor->Rowid = 0;

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleNext(
    PSQLITE3_VTAB_CURSOR Sqlite3Cursor
    )
{
    ULONGLONG NumberOfRecords;
    ULONGLONG NextRecordOffset = 0;
    TRACE_STORE_TRAITS Traits;
    PTRACE_STORE_SQLITE3_CURSOR Cursor;
    PTRACE_STORE_ALLOCATION NextAllocation = NULL;

    //
    // Resolve cursor and initialize local traits variable.
    //

    Cursor = CONTAINING_RECORD(Sqlite3Cursor,
                               TRACE_STORE_SQLITE3_CURSOR,
                               AsSqlite3VtabCursor);

    Traits = Cursor->Traits;

    //
    // Advance the cursor and perform an EOF check; if true, return now and
    // avoid any unnecessary record size deduction.
    //

    if (Cursor->Rowid++ >= Cursor->TotalNumberOfRecords - 1) {
        return SQLITE_OK;
    }

    //
    // Invariant check: the multiple records trait should be set for the trace
    // store at this point if we passed the EOF test above.
    //

    if (IsSingleRecord(Traits)) {
        __debugbreak();
        return SQLITE_ERROR;
    }

    //
    // We need to determine the size of the active record (row) in order to
    // determine how many bytes we need to advance the current row pointer
    // such that it points at the next row.
    //

    if (Cursor->Flags.IsMetadataStore) {

        //
        // For metadata stores, this is easy, as they always have a fixed
        // record size, and we capture it up-front when the cursor is created.
        //

        NextRecordOffset = Cursor->MetadataRecordSizeInBytes;

    } else {

        //
        // For trace stores, things are a little more involved.  Let's handle
        // the easiest case first: if the trace store's record size is already
        // set for us, we don't need to do any more work.
        //

        if (Cursor->TraceStoreRecordSizeInBytes) {
            NextRecordOffset = Cursor->TraceStoreRecordSizeInBytes;
            goto UpdateRow;
        }

        //
        // If we're already in the middle of enumerating a coalesced allocation,
        // continue with that logic.
        //

        if (Cursor->Flags.IsCoalescedAllocationActive) {

            //
            // Update our current coalesced allocation counter.  If it has
            // surpassed the total number of coalesced allocations in this
            // chunk, advance to the next allocation record.  Otherwise,
            // use the coalesced allocation's record size as the next record
            // offset and jump to the update row logic.
            //

            Cursor->CurrentCoalescedAllocationNumber += 1;

            if (Cursor->CurrentCoalescedAllocationNumber ==
                Cursor->TotalCoalescedAllocations) {

                //
                // Advance to the next allocation record and reset the current
                // coalesced allocation state.
                //

                Cursor->Allocation++;
                Cursor->Flags.IsCoalescedAllocationActive = FALSE;

            } else {

                //
                // Current allocation record is still active, use its record
                // size.
                //

                NextRecordOffset = Cursor->Allocation->RecordSize.QuadPart;
                goto UpdateRow;
            }

        } else {

            //
            // If no coalesced record is active, advance the allocation record.
            //

            Cursor->Allocation++;
        }

        //
        // If we get here, we will have just advanced the allocation record.
        // Thus, we need to test if the currently active allocation is a dummy
        // one, and if so, skip past it and any subsequent dummy allocations
        // until we find the first real one.
        //

        while (IsDummyAllocation(Cursor->Allocation)) {
            Cursor->Allocation++;
        }

        //
        // The cursor's allocation record is now definitely the one we want.
        // That is, it describes the size of the record that was allocated for
        // the address reflected in the cursor's current row.  (Note that the
        // allocation record actually lags one iteration behind the row; i.e.
        // in order to find out where row 2 starts, we process the allocation
        // record number 1.)
        //
        // Obtain the record offset from the allocation record.  Then, test to
        // see if it represents multiple allocations; if so, initialize the
        // cursor's coalesced counters such that the relevant state can be
        // resumed upon subsequent invocations of Next().
        //

        NextRecordOffset = Cursor->Allocation->RecordSize.QuadPart;

        NumberOfRecords = Cursor->Allocation->NumberOfRecords.QuadPart;
        if (NumberOfRecords > 1) {

            //
            // Initialize coalesced allocation state.
            //

            Cursor->Flags.IsCoalescedAllocationActive = TRUE;
            Cursor->CurrentCoalescedAllocationNumber = 1;
            Cursor->TotalCoalescedAllocations = NumberOfRecords;
        }
    }

    //
    // Invariant check, next record offset should be non-zero here.
    //

    if (!NextRecordOffset) {
        __debugbreak();
        return SQLITE_ERROR;
    }

UpdateRow:

    //
    // Update the cursor and return success.
    //

    Cursor->CurrentRowRaw = Cursor->CurrentRowRaw + NextRecordOffset;

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleEof(
    PSQLITE3_VTAB_CURSOR Sqlite3Cursor
    )
{
    PTRACE_STORE_SQLITE3_CURSOR Cursor;

    Cursor = CONTAINING_RECORD(Sqlite3Cursor,
                               TRACE_STORE_SQLITE3_CURSOR,
                               AsSqlite3VtabCursor);

    return (Cursor->Rowid >= Cursor->TotalNumberOfRecords);
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleColumn(
    PSQLITE3_VTAB_CURSOR Sqlite3Cursor,
    PSQLITE3_CONTEXT Context,
    LONG ColumnNumber
    )
{
    PTRACE_STORE_SQLITE3_CURSOR Cursor;

    Cursor = CONTAINING_RECORD(Sqlite3Cursor,
                               TRACE_STORE_SQLITE3_CURSOR,
                               AsSqlite3VtabCursor);

    return Cursor->Sqlite3Column(Cursor->Sqlite3,
                                 Cursor->TraceStore,
                                 Cursor,
                                 Context,
                                 ColumnNumber);
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleRowid(
    PSQLITE3_VTAB_CURSOR Sqlite3Cursor,
    PSQLITE3_INT64 RowidPointer
    )
{
    PTRACE_STORE_SQLITE3_CURSOR Cursor;

    Cursor = CONTAINING_RECORD(Sqlite3Cursor,
                               TRACE_STORE_SQLITE3_CURSOR,
                               AsSqlite3VtabCursor);

    *RowidPointer = Cursor->Rowid;

    return SQLITE_OK;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleUpdate(
    PSQLITE3_VTAB VirtualTable,
    LONG Unknown1,
    PSQLITE3_VALUE *Value,
    PSQLITE3_INT64 Unknown2
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleBegin(
    PSQLITE3_VTAB VirtualTable
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleSync(
    PSQLITE3_VTAB VirtualTable
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleCommit(
    PSQLITE3_VTAB VirtualTable
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleRollback(
    PSQLITE3_VTAB VirtualTable
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleFindFunction(
    PSQLITE3_VTAB VirtualTable,
    LONG ArgumentNumber,
    PCSZ FunctionName,
    PSQLITE3_SCALAR_FUNCTION *ScalarFunctionPointer,
    PVOID *ArgumentPointer
    )
{
    USHORT MatchIndex;
    STRING String;
    PCSQLITE3 Sqlite3;
    STRING_MATCH Match;
    PTRACE_STORE TraceStore;
    PSTRING_TABLE StringTable;
    PTRACE_STORE_SQLITE3_DB Db;
    TRACE_STORE_SQLITE3_FUNCTION_ID FunctionId;
    PIS_PREFIX_OF_STRING_IN_TABLE IsPrefixOfStringInTable;

    TraceStore = CONTAINING_RECORD(VirtualTable,
                                   TRACE_STORE,
                                   Sqlite3VirtualTable);

    Db = TraceStore->Db;
    Sqlite3 = Db->Sqlite3;

    String.Length = (USHORT)strlen(FunctionName);
    String.MaximumLength = String.Length;
    String.Buffer = (PSTR)FunctionName;

    StringTable = Db->FunctionStringTable1;
    IsPrefixOfStringInTable = StringTable->IsPrefixOfStringInTable;

    MatchIndex = IsPrefixOfStringInTable(StringTable, &String, &Match);
    if (MatchIndex == NO_MATCH_FOUND) {
        return SQLITE_ERROR;
    }

    FunctionId = MatchIndex;

    switch (FunctionId) {

        case CountFunctionId:
            break;

        case AverageFunctionId:
            break;

        case SumFunctionId:
            break;

        default:
            break;
    }

    return SQLITE_ERROR;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleRename(
    PSQLITE3_VTAB VirtualTable,
    PCSZ NewName
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleSavepoint(
    PSQLITE3_VTAB VirtualTable,
    LONG Unknown
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleRelease(
    PSQLITE3_VTAB VirtualTable,
    LONG Unknown
    )
{
    return SQLITE_READONLY;
}

_Use_decl_annotations_
LONG
TraceStoreSqlite3ModuleRollbackTo(
    PSQLITE3_VTAB VirtualTable,
    LONG Unknown
    )
{
    return SQLITE_READONLY;
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
