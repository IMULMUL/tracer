#include "Rtl.h"

INIT_ONCE InitOnceSystemTimerFunction = INIT_ONCE_STATIC_INIT;

PVECTORED_EXCEPTION_HANDLER VectoredExceptionHandler = NULL;

INIT_ONCE InitOnceCSpecificHandler = INIT_ONCE_STATIC_INIT;

CONST static UNICODE_STRING ExtendedLengthVolumePrefix = RTL_CONSTANT_STRING(L"\\\\?\\");

LONG
WINAPI
CopyMappedMemoryVectoredExceptionHandler(
_In_ PEXCEPTION_POINTERS ExceptionInfo
)
{
    PCONTEXT Context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD Exception = ExceptionInfo->ExceptionRecord;

    if (Exception->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL
CALLBACK
SetCSpecificHandlerCallback(
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *lpContext
)
{
    VectoredExceptionHandler = AddVectoredExceptionHandler(1, CopyMappedMemoryVectoredExceptionHandler);
    return (VectoredExceptionHandler ? TRUE : FALSE);
}

BOOL
SetCSpecificHandler(PCSPECIFICHANDLER Handler)
{
    BOOL Status;

    Status = InitOnceExecuteOnce(
        &InitOnceCSpecificHandler,
        SetCSpecificHandlerCallback,
        Handler,
        NULL
    );

    return Status;
}

BOOL
CALLBACK
GetSystemTimerFunctionCallback(
    _Inout_     PINIT_ONCE  InitOnce,
    _Inout_     PVOID       Parameter,
    _Inout_opt_ PVOID       *lpContext
)
{
    HMODULE Module;
    FARPROC Proc;
    static SYSTEM_TIMER_FUNCTION SystemTimerFunction = { 0 };

    if (!lpContext) {
        return FALSE;
    }

    Module = GetModuleHandle(TEXT("kernel32"));
    if (Module == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    Proc = GetProcAddress(Module, "GetSystemTimePreciseAsFileTime");
    if (Proc) {
        SystemTimerFunction.GetSystemTimePreciseAsFileTime = (PGETSYSTEMTIMEPRECISEASFILETIME)Proc;
    } else {
        Module = LoadLibrary(TEXT("ntdll"));
        if (!Module) {
            return FALSE;
        }
        Proc = GetProcAddress(Module, "NtQuerySystemTime");
        if (!Proc) {
            return FALSE;
        }
        SystemTimerFunction.NtQuerySystemTime = (PNTQUERYSYSTEMTIME)Proc;
    }

    *((PPSYSTEM_TIMER_FUNCTION)lpContext) = &SystemTimerFunction;
    return TRUE;
}

PSYSTEM_TIMER_FUNCTION
GetSystemTimerFunction()
{
    BOOL Status;
    PSYSTEM_TIMER_FUNCTION SystemTimerFunction;

    Status = InitOnceExecuteOnce(
        &InitOnceSystemTimerFunction,
        GetSystemTimerFunctionCallback,
        NULL,
        (LPVOID *)&SystemTimerFunction
    );

    if (!Status) {
        return NULL;
    } else {
        return SystemTimerFunction;
    }
}

_Check_return_
BOOL
CallSystemTimer(
    _Out_       PFILETIME   SystemTime,
    _Inout_opt_ PPSYSTEM_TIMER_FUNCTION ppSystemTimerFunction
)
{
    PSYSTEM_TIMER_FUNCTION SystemTimerFunction = NULL;

    if (ppSystemTimerFunction) {
        if (*ppSystemTimerFunction) {
            SystemTimerFunction = *ppSystemTimerFunction;
        } else {
            SystemTimerFunction = GetSystemTimerFunction();
            *ppSystemTimerFunction = SystemTimerFunction;
        }
    } else {
        SystemTimerFunction = GetSystemTimerFunction();
    }

    if (!SystemTimerFunction) {
        return FALSE;
    }

    if (SystemTimerFunction->GetSystemTimePreciseAsFileTime) {
        SystemTimerFunction->GetSystemTimePreciseAsFileTime(SystemTime);
    } else if (SystemTimerFunction->NtQuerySystemTime) {
        if (!SystemTimerFunction->NtQuerySystemTime((PLARGE_INTEGER)SystemTime)) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    return TRUE;
}

_Check_return_
PVOID
CopyToMemoryMappedMemory(
    PVOID Destination,
    LPCVOID Source,
    SIZE_T Size
)
{
    return memcpy(Destination, Source, Size);
}


BOOL
FindCharsInUnicodeString(
    _In_     PRTL                Rtl,
    _In_     PUNICODE_STRING     String,
    _In_     WCHAR               CharToFind,
    _Inout_  PRTL_BITMAP         Bitmap,
    _In_     BOOL                Reverse
)
{
    USHORT Index;
    USHORT NumberOfCharacters = String->Length >> 1;
    ULONG  Bit;
    WCHAR  Char;
    PRTL_SET_BIT RtlSetBit = Rtl->RtlSetBit;

    //
    // We use two loop implementations in order to avoid an additional
    // branch during the loop (after we find a character match).
    //

    if (Reverse) {
        for (Index = 0; Index < NumberOfCharacters; Index++) {
            Char = String->Buffer[Index];
            if (Char == CharToFind) {
                Bit = NumberOfCharacters - Index;
                RtlSetBit(Bitmap, Bit);
            }
        }
    }
    else {
        for (Index = 0; Index < NumberOfCharacters; Index++) {
            Char = String->Buffer[Index];
            if (Char == CharToFind) {
                RtlSetBit(Bitmap, Index);
            }
        }
    }

    return TRUE;
}

_Check_return_
BOOL
CreateBitmapIndexForUnicodeString(
    _In_     PRTL                Rtl,
    _In_     PUNICODE_STRING     String,
    _In_     WCHAR               Char,
    _Inout_  PHANDLE             HeapHandlePointer,
    _Inout_  PPRTL_BITMAP        BitmapPointer,
    _In_     BOOL                Reverse
    )

/*++

Routine Description:

    This is a helper function that simplifies creating bitmap indexes for
    UNICODE_STRING structures.  It can allocate either the entire RTL_BITMAP
    structure plus corresponding space for the bitmap buffer, or just the
    bitmap buffer itself if the user provides an existing RTL_BITMAP struct.
    
    Typically, callers would provide their own pointer to a stack-allocated
    RTL_BITMAP struct if they only need the bitmap for the scope of their
    function call.  For longer-lived bitmaps, a pointer to a NULL pointer
    would be provided, indicating that the entire structure should be heap
    allocated.

    Caller is responsible for freeing either the entire RTL_BITMAP or the
    underlying Bitmap->Buffer.

Arguments:

    Rtl - Supplies the pointer to the RTL structure (mandatory).

    String - Supplies the UNICODE_STRING structure to create the bitmap
        index for (mandatory).

    Char - Supplies the character to create the bitmap index for.  This is
        passed directly to FindCharsInUnicodeString().

    HeapHandlePointer - Supplies a pointer to the underlying heap handle
        to use for allocation.  If this pointer points to a NULL value,
        the default process heap handle will be used (obtained via
        GetProcessHeap()), and the pointed-to location will be updated
        with the handle value.  The caller will need this in order to
        perform the subsequent HeapFree().

    BitmapPointer - Supplies a pointer to a PRTL_BITMAP structure.  If the
        pointed-to location is NULL, additional space for the RTL_BITMAP
        structure will be allocated on top of the bitmap buffer space, and
        the pointed-to location will be updated with the resulting address.
        User is responsible for freeing either the entire PRTL_BITMAP struct
        or the Bitmap->Buffer, depending on usage.

    Reverse - Supplies a boolean flag indicating the bitmap index should be
        created in reverse.  This is passed to FindCharsInUnicodeString().

Return Value:

    TRUE on success, FALSE on error.

--*/

{
    USHORT NumberOfCharacters;
    USHORT AlignedNumberOfCharacters;
    SIZE_T BitmapBufferSizeInBytes;
    PRTL_BITMAP Bitmap;
    HANDLE HeapHandle;
    BOOL UpdateBitmapPointer;
    BOOL UpdateHeapHandlePointer;
    BOOL Success;

    //
    // Verify arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(String)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(HeapHandlePointer)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(BitmapPointer)) {
        return FALSE;
    }

    if (!*HeapHandlePointer) {
        
        //
        // If the pointer to the heap handle to use is NULL, default to using
        // the default process heap via GetProcessHeap().  Note that we also
        // assign back to the user's pointer, such that they get a copy of the
        // heap handle that was used for allocation.
        //

        HeapHandle = GetProcessHeap();

        if (!HeapHandle) {
            return FALSE;
        }

        UpdateHeapHandlePointer = TRUE;
    }
    else {
        
        //
        // Use the handle the user provided.
        //

        HeapHandle = *HeapHandlePointer;
        UpdateHeapHandlePointer = FALSE;
    }

    //
    // Resolve the number of characters, then make sure it's aligned to the
    // platform's pointer size.
    //
    NumberOfCharacters = String->Length >> 1;
    AlignedNumberOfCharacters = ALIGN_UP_USHORT_TO_POINTER_SIZE(NumberOfCharacters);

    BitmapBufferSizeInBytes = AlignedNumberOfCharacters >> 3;

    if (!*BitmapPointer) {

        //
        // If the pointer to the PRTL_BITMAP structure is NULL, the caller
        // wants us to allocate the space for the RTL_BITMAP structure as
        // well.
        //

        Bitmap = (PRTL_BITMAP)HeapAlloc(
            HeapHandle,
            HEAP_ZERO_MEMORY,
            BitmapBufferSizeInBytes + sizeof(RTL_BITMAP)
            );

        if (!Bitmap) {
            return FALSE;
        }

        //
        // Point the bitmap buffer to the end of the RTL_BITMAP struct.
        //
        
        Bitmap->Buffer = (PULONG)RtlOffsetToPointer(Bitmap, sizeof(RTL_BITMAP));

        //
        // Make a note that we need to update the user's bitmap pointer.
        //

        UpdateBitmapPointer = TRUE;

    }
    else {

        //
        // The user has provided an existing PRTL_BITMAP structure, so we
        // only need to allocate memory for the actual underlying bitmap
        // buffer.
        //

        Bitmap = *BitmapPointer;

        Bitmap->Buffer = (PULONG)HeapAlloc(
            HeapHandle,
            HEAP_ZERO_MEMORY,
            BitmapBufferSizeInBytes
            );

        if (!Bitmap->Buffer) {
            return FALSE;
        }

        //
        // Make a note that we do *not* need to update the user's bitmap
        // pointer.
        //

        UpdateBitmapPointer = FALSE;

    }

    //
    // There will be one bit per character.
    //
    
    Bitmap->SizeOfBitMap = AlignedNumberOfCharacters;

    if (!Bitmap->Buffer) {
        __debugbreak();
    }

    //
    // Fill in the bitmap index.
    //

    Success = Rtl->FindCharsInUnicodeString(Rtl, String, Char, Bitmap, Reverse);

    if (!Success) {

        if (UpdateBitmapPointer) {

            //
            // Free the entire structure.
            //
            
            HeapFree(HeapHandle, 0, Bitmap);

        }
        else {
            
            //
            // Free just the buffer.
            //

            HeapFree(HeapHandle, 0, Bitmap->Buffer);

        }

        return FALSE;
    }

    //
    // Update user pointers if necessary, then return successfully.
    //

    if (UpdateBitmapPointer) {
        *BitmapPointer = Bitmap;
    }

    if (UpdateHeapHandlePointer) {
        *HeapHandlePointer = HeapHandle;
    }

    return TRUE;
}

_Check_return_
BOOL
FilesExist(
    _In_      PRTL Rtl,
    _In_      PUNICODE_STRING Directory,
    _In_      USHORT NumberOfFilenames,
    _In_      PPUNICODE_STRING Filenames,
    _In_      USHORT MaxFilenameLength,
    _Out_     PBOOL Exists,
    _Out_opt_ PUSHORT WhichIndex,
    _Out_opt_ PPUNICODE_STRING WhichFilename
    )
{
    USHORT Index;
    PWCHAR HeapBuffer;
    HANDLE HeapHandle;
    ULONG CombinedSizeInBytes;
    USHORT DirectoryLength;
    UNICODE_STRING Path;
    PUNICODE_STRING Filename;
    HRESULT Result;
    DWORD Attributes;
    BOOL Success = FALSE;
    WCHAR StackBuffer[_MAX_PATH];

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Directory)) {
        return FALSE;
    }

    if (NumberOfFilenames == 0) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Filenames) || !ARGUMENT_PRESENT(Filenames[0])) {
        return FALSE;
    }

    //
    // See if the combined size of the extended volume prefix ("\\?\"), 
    // directory, joining backslash, maximum filename length and terminating
    // NUL is less than or equal to _MAX_PATH.  If it is, we can use the
    // stack-allocated Path buffer above; if not, allocate a new buffer from
    // the default heap.
    //

    CombinedSizeInBytes = (
        ExtendedLengthVolumePrefix.Length +
        Directory->Length                 +
        sizeof(WCHAR)                     + // joining backslash
        MaxFilenameLength                 +
        sizeof(WCHAR)                       // terminating NUL
    );

    //
    // Point Path->Buffer at the stack or heap buffer depending on the
    // combined size.
    //

    if (CombinedSizeInBytes <= _MAX_PATH) {

        //
        // We can use our stack buffer.
        //

        Path.Buffer = &StackBuffer[0];

    }
    else if (CombinedSizeInBytes > MAX_USTRING) {

        goto Error;

    }
    else {

        //
        // The combined size exceeds _MAX_PATH so allocate the required memory
        // from the heap.
        //

        HeapHandle = GetProcessHeap();
        if (!HeapHandle) {
            return FALSE;
        }

        HeapBuffer = (PWCHAR)HeapAlloc(HeapHandle, 0, CombinedSizeInBytes);

        if (!HeapBuffer) {
            return FALSE;
        }

        Path.Buffer = HeapBuffer;

    }

    Path.Length = 0;
    Path.MaximumLength = (USHORT)CombinedSizeInBytes;

    //
    // Copy the volume prefix, then append the directory and joining backslash.
    //

    Rtl->RtlCopyUnicodeString(&Path, &ExtendedLengthVolumePrefix);

    if (FAILED(Rtl->RtlAppendUnicodeStringToString(&Path, Directory)) ||
        FAILED(Rtl->RtlAppendUnicodeToString(&Path, L"\\")))
    {
        goto Error;
    }

    //
    // Make a note of the length at this point as we'll need to revert to it
    // after each unsuccessful file test.
    //

    DirectoryLength = Path.Length;

    //
    // Enumerate over the array of filenames and look for the first one that
    // exists.
    //

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        Filename = Filenames[Index];

        //
        // Quick sanity check.
        //

        if (!Filename || Filename->Length == 0 || Filename->Buffer == NULL) {
            goto Error;
        }

        Result = Rtl->RtlAppendUnicodeStringToString(&Path, Filename);

        if (!FAILED(Result)) {

            //
            // Add the trailing NUL.
            //

            Result = Rtl->RtlAppendUnicodeToString(&Path, UNICODE_NULL);
        }

        if (FAILED(Result)) {

            //
            // If either call failed, it'll be because of STATUS_BUFFER_TO_SMALL,
            // which means the user was telling fibs about the longest filename
            // length.
            //

            goto Error;

        }

        //
        // We successfully constructed the path, so we can now look up the file
        // attributes.
        //

        Attributes = GetFileAttributesW(Path.Buffer);

        if (Attributes == INVALID_FILE_ATTRIBUTES ||
            (Attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            
            //
            // File doesn't exist or is a directory.  Reset the path length
            // and continue.
            //

            Path.Length = DirectoryLength;

            continue;
        }

        //
        // Success!  File exists and *isn't* a directory.  We're done.
        //

        Success = TRUE;
        break;
    }

    if (!Success) {

        *Exists = FALSE;

    } else {

        *Exists = TRUE;

        //
        // Update the user's pointers if applicable.
        //

        if (ARGUMENT_PRESENT(WhichIndex)) {
            *WhichIndex = Index;
        }

        if (ARGUMENT_PRESENT(WhichFilename)) {
            *WhichFilename = Filename;
        }

    }

    //
    // Intentional follow-on to "Error"; Success code will be set
    // appropriately by this stage.
    //

Error:
    if (HeapHandle) {
        HeapFree(HeapHandle, 0, Path.Buffer);
    }

    return Success;
}


_Check_return_
BOOL
LoadRtlSymbols(_Inout_ PRTL Rtl)
{

    if (!(Rtl->Kernel32Module = LoadLibraryA("kernel32"))) {
        return FALSE;
    }

    if (!(Rtl->NtdllModule = LoadLibraryA("ntdll"))) {
        return FALSE;
    }

    if (!(Rtl->NtosKrnlModule = LoadLibraryA("ntoskrnl.exe"))) {
        return FALSE;
    }

    Rtl->GetSystemTimePreciseAsFileTime = (PGETSYSTEMTIMEPRECISEASFILETIME)
        GetProcAddress(Rtl->Kernel32Module, "GetSystemTimePreciseAsFileTime");

    Rtl->NtQuerySystemTime = (PNTQUERYSYSTEMTIME)
        GetProcAddress(Rtl->NtdllModule, "NtQuerySystemTime");

    if (Rtl->GetSystemTimePreciseAsFileTime) {
        Rtl->SystemTimerFunction.GetSystemTimePreciseAsFileTime =
            Rtl->GetSystemTimePreciseAsFileTime;
    } else if (Rtl->NtQuerySystemTime) {
        Rtl->SystemTimerFunction.NtQuerySystemTime =
            Rtl->NtQuerySystemTime;
    } else {
        return FALSE;
    }

    //
    // Start of auto-generated function resolutions.  Any manual modifications
    // will be lost; run `tracerdev sync-rtl-header` to keep in sync.  If you
    // need to tweak the template, make sure the first block (RtlCharToInteger)
    // matches the new template -- the sync-rtl-header command relies on the
    // first block to match in order to determine the starting point for where
    // the blocks should be placed.
    //

    if (!(Rtl->RtlCharToInteger = (PRTLCHARTOINTEGER)
        GetProcAddress(Rtl->NtdllModule, "RtlCharToInteger"))) {

        if (!(Rtl->RtlCharToInteger = (PRTLCHARTOINTEGER)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlCharToInteger"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlCharToInteger'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInitializeGenericTable = (PRTL_INITIALIZE_GENERIC_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlInitializeGenericTable"))) {

        if (!(Rtl->RtlInitializeGenericTable = (PRTL_INITIALIZE_GENERIC_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInitializeGenericTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInitializeGenericTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInsertElementGenericTable = (PRTL_INSERT_ELEMENT_GENERIC_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlInsertElementGenericTable"))) {

        if (!(Rtl->RtlInsertElementGenericTable = (PRTL_INSERT_ELEMENT_GENERIC_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInsertElementGenericTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInsertElementGenericTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInsertElementGenericTableFull = (PRTL_INSERT_ELEMENT_GENERIC_TABLE_FULL)
        GetProcAddress(Rtl->NtdllModule, "RtlInsertElementGenericTableFull"))) {

        if (!(Rtl->RtlInsertElementGenericTableFull = (PRTL_INSERT_ELEMENT_GENERIC_TABLE_FULL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInsertElementGenericTableFull"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInsertElementGenericTableFull'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlDeleteElementGenericTable = (PRTL_DELETE_ELEMENT_GENERIC_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlDeleteElementGenericTable"))) {

        if (!(Rtl->RtlDeleteElementGenericTable = (PRTL_DELETE_ELEMENT_GENERIC_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlDeleteElementGenericTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlDeleteElementGenericTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlLookupElementGenericTable = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlLookupElementGenericTable"))) {

        if (!(Rtl->RtlLookupElementGenericTable = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlLookupElementGenericTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlLookupElementGenericTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlLookupElementGenericTableFull = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE_FULL)
        GetProcAddress(Rtl->NtdllModule, "RtlLookupElementGenericTableFull"))) {

        if (!(Rtl->RtlLookupElementGenericTableFull = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE_FULL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlLookupElementGenericTableFull"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlLookupElementGenericTableFull'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEnumerateGenericTable = (PRTL_ENUMERATE_GENERIC_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlEnumerateGenericTable"))) {

        if (!(Rtl->RtlEnumerateGenericTable = (PRTL_ENUMERATE_GENERIC_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEnumerateGenericTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEnumerateGenericTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEnumerateGenericTableWithoutSplaying = (PRTL_ENUMERATE_GENERIC_TABLE_WITHOUT_SPLAYING)
        GetProcAddress(Rtl->NtdllModule, "RtlEnumerateGenericTableWithoutSplaying"))) {

        if (!(Rtl->RtlEnumerateGenericTableWithoutSplaying = (PRTL_ENUMERATE_GENERIC_TABLE_WITHOUT_SPLAYING)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEnumerateGenericTableWithoutSplaying"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEnumerateGenericTableWithoutSplaying'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlGetElementGenericTable = (PRTL_GET_ELEMENT_GENERIC_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlGetElementGenericTable"))) {

        if (!(Rtl->RtlGetElementGenericTable = (PRTL_GET_ELEMENT_GENERIC_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlGetElementGenericTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlGetElementGenericTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlNumberGenericTableElements = (PRTL_NUMBER_GENERIC_TABLE_ELEMENTS)
        GetProcAddress(Rtl->NtdllModule, "RtlNumberGenericTableElements"))) {

        if (!(Rtl->RtlNumberGenericTableElements = (PRTL_NUMBER_GENERIC_TABLE_ELEMENTS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlNumberGenericTableElements"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlNumberGenericTableElements'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlIsGenericTableEmpty = (PRTL_IS_GENERIC_TABLE_EMPTY)
        GetProcAddress(Rtl->NtdllModule, "RtlIsGenericTableEmpty"))) {

        if (!(Rtl->RtlIsGenericTableEmpty = (PRTL_IS_GENERIC_TABLE_EMPTY)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlIsGenericTableEmpty"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlIsGenericTableEmpty'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInitializeGenericTableAvl = (PRTL_INITIALIZE_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlInitializeGenericTableAvl"))) {

        if (!(Rtl->RtlInitializeGenericTableAvl = (PRTL_INITIALIZE_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInitializeGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInitializeGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInsertElementGenericTableAvl = (PRTL_INSERT_ELEMENT_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlInsertElementGenericTableAvl"))) {

        if (!(Rtl->RtlInsertElementGenericTableAvl = (PRTL_INSERT_ELEMENT_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInsertElementGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInsertElementGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInsertElementGenericTableFullAvl = (PRTL_INSERT_ELEMENT_GENERIC_TABLE_FULL_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlInsertElementGenericTableFullAvl"))) {

        if (!(Rtl->RtlInsertElementGenericTableFullAvl = (PRTL_INSERT_ELEMENT_GENERIC_TABLE_FULL_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInsertElementGenericTableFullAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInsertElementGenericTableFullAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlDeleteElementGenericTableAvl = (PRTL_DELETE_ELEMENT_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlDeleteElementGenericTableAvl"))) {

        if (!(Rtl->RtlDeleteElementGenericTableAvl = (PRTL_DELETE_ELEMENT_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlDeleteElementGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlDeleteElementGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlLookupElementGenericTableAvl = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlLookupElementGenericTableAvl"))) {

        if (!(Rtl->RtlLookupElementGenericTableAvl = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlLookupElementGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlLookupElementGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlLookupElementGenericTableFullAvl = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE_FULL_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlLookupElementGenericTableFullAvl"))) {

        if (!(Rtl->RtlLookupElementGenericTableFullAvl = (PRTL_LOOKUP_ELEMENT_GENERIC_TABLE_FULL_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlLookupElementGenericTableFullAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlLookupElementGenericTableFullAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEnumerateGenericTableAvl = (PRTL_ENUMERATE_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlEnumerateGenericTableAvl"))) {

        if (!(Rtl->RtlEnumerateGenericTableAvl = (PRTL_ENUMERATE_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEnumerateGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEnumerateGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEnumerateGenericTableWithoutSplayingAvl = (PRTL_ENUMERATE_GENERIC_TABLE_WITHOUT_SPLAYING_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlEnumerateGenericTableWithoutSplayingAvl"))) {

        if (!(Rtl->RtlEnumerateGenericTableWithoutSplayingAvl = (PRTL_ENUMERATE_GENERIC_TABLE_WITHOUT_SPLAYING_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEnumerateGenericTableWithoutSplayingAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEnumerateGenericTableWithoutSplayingAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlLookupFirstMatchingElementGenericTableAvl = (PRTL_LOOKUP_FIRST_MATCHING_ELEMENT_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlLookupFirstMatchingElementGenericTableAvl"))) {

        if (!(Rtl->RtlLookupFirstMatchingElementGenericTableAvl = (PRTL_LOOKUP_FIRST_MATCHING_ELEMENT_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlLookupFirstMatchingElementGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlLookupFirstMatchingElementGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEnumerateGenericTableLikeADirectory = (PRTL_ENUMERATE_GENERIC_TABLE_LIKE_A_DICTIONARY)
        GetProcAddress(Rtl->NtdllModule, "RtlEnumerateGenericTableLikeADirectory"))) {

        if (!(Rtl->RtlEnumerateGenericTableLikeADirectory = (PRTL_ENUMERATE_GENERIC_TABLE_LIKE_A_DICTIONARY)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEnumerateGenericTableLikeADirectory"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEnumerateGenericTableLikeADirectory'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlGetElementGenericTableAvl = (PRTL_GET_ELEMENT_GENERIC_TABLE_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlGetElementGenericTableAvl"))) {

        if (!(Rtl->RtlGetElementGenericTableAvl = (PRTL_GET_ELEMENT_GENERIC_TABLE_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlGetElementGenericTableAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlGetElementGenericTableAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlNumberGenericTableElementsAvl = (PRTL_NUMBER_GENERIC_TABLE_ELEMENTS_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlNumberGenericTableElementsAvl"))) {

        if (!(Rtl->RtlNumberGenericTableElementsAvl = (PRTL_NUMBER_GENERIC_TABLE_ELEMENTS_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlNumberGenericTableElementsAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlNumberGenericTableElementsAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlIsGenericTableEmptyAvl = (PRTL_IS_GENERIC_TABLE_EMPTY_AVL)
        GetProcAddress(Rtl->NtdllModule, "RtlIsGenericTableEmptyAvl"))) {

        if (!(Rtl->RtlIsGenericTableEmptyAvl = (PRTL_IS_GENERIC_TABLE_EMPTY_AVL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlIsGenericTableEmptyAvl"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlIsGenericTableEmptyAvl'");
            return FALSE;
        }
    }

    if (!(Rtl->PfxInitialize = (PPFX_INITIALIZE)
        GetProcAddress(Rtl->NtdllModule, "PfxInitialize"))) {

        if (!(Rtl->PfxInitialize = (PPFX_INITIALIZE)
            GetProcAddress(Rtl->NtosKrnlModule, "PfxInitialize"))) {

            OutputDebugStringA("Rtl: failed to resolve 'PfxInitialize'");
            return FALSE;
        }
    }

    if (!(Rtl->PfxInsertPrefix = (PPFX_INSERT_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "PfxInsertPrefix"))) {

        if (!(Rtl->PfxInsertPrefix = (PPFX_INSERT_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "PfxInsertPrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'PfxInsertPrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->PfxRemovePrefix = (PPFX_REMOVE_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "PfxRemovePrefix"))) {

        if (!(Rtl->PfxRemovePrefix = (PPFX_REMOVE_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "PfxRemovePrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'PfxRemovePrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->PfxFindPrefix = (PPFX_FIND_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "PfxFindPrefix"))) {

        if (!(Rtl->PfxFindPrefix = (PPFX_FIND_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "PfxFindPrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'PfxFindPrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlPrefixUnicodeString = (PRTL_PREFIX_UNICODE_STRING)
        GetProcAddress(Rtl->NtdllModule, "RtlPrefixUnicodeString"))) {

        if (!(Rtl->RtlPrefixUnicodeString = (PRTL_PREFIX_UNICODE_STRING)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlPrefixUnicodeString"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlPrefixUnicodeString'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlCreateHashTable = (PRTL_CREATE_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlCreateHashTable"))) {

        if (!(Rtl->RtlCreateHashTable = (PRTL_CREATE_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlCreateHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlCreateHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlDeleteHashTable = (PRTL_DELETE_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlDeleteHashTable"))) {

        if (!(Rtl->RtlDeleteHashTable = (PRTL_DELETE_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlDeleteHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlDeleteHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInsertEntryHashTable = (PRTL_INSERT_ENTRY_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlInsertEntryHashTable"))) {

        if (!(Rtl->RtlInsertEntryHashTable = (PRTL_INSERT_ENTRY_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInsertEntryHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInsertEntryHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlRemoveEntryHashTable = (PRTL_REMOVE_ENTRY_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlRemoveEntryHashTable"))) {

        if (!(Rtl->RtlRemoveEntryHashTable = (PRTL_REMOVE_ENTRY_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlRemoveEntryHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlRemoveEntryHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlLookupEntryHashTable = (PRTL_LOOKUP_ENTRY_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlLookupEntryHashTable"))) {

        if (!(Rtl->RtlLookupEntryHashTable = (PRTL_LOOKUP_ENTRY_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlLookupEntryHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlLookupEntryHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlGetNextEntryHashTable = (PRTL_GET_NEXT_ENTRY_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlGetNextEntryHashTable"))) {

        if (!(Rtl->RtlGetNextEntryHashTable = (PRTL_GET_NEXT_ENTRY_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlGetNextEntryHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlGetNextEntryHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEnumerateEntryHashTable = (PRTL_ENUMERATE_ENTRY_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlEnumerateEntryHashTable"))) {

        if (!(Rtl->RtlEnumerateEntryHashTable = (PRTL_ENUMERATE_ENTRY_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEnumerateEntryHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEnumerateEntryHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEndEnumerationHashTable = (PRTL_END_ENUMERATION_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlEndEnumerationHashTable"))) {

        if (!(Rtl->RtlEndEnumerationHashTable = (PRTL_END_ENUMERATION_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEndEnumerationHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEndEnumerationHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInitWeakEnumerationHashTable = (PRTL_INIT_WEAK_ENUMERATION_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlInitWeakEnumerationHashTable"))) {

        if (!(Rtl->RtlInitWeakEnumerationHashTable = (PRTL_INIT_WEAK_ENUMERATION_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInitWeakEnumerationHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInitWeakEnumerationHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlWeaklyEnumerateEntryHashTable = (PRTL_WEAKLY_ENUMERATE_ENTRY_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlWeaklyEnumerateEntryHashTable"))) {

        if (!(Rtl->RtlWeaklyEnumerateEntryHashTable = (PRTL_WEAKLY_ENUMERATE_ENTRY_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlWeaklyEnumerateEntryHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlWeaklyEnumerateEntryHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlEndWeakEnumerationHashTable = (PRTL_END_WEAK_ENUMERATION_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlEndWeakEnumerationHashTable"))) {

        if (!(Rtl->RtlEndWeakEnumerationHashTable = (PRTL_END_WEAK_ENUMERATION_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlEndWeakEnumerationHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlEndWeakEnumerationHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlExpandHashTable = (PRTL_EXPAND_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlExpandHashTable"))) {

        if (!(Rtl->RtlExpandHashTable = (PRTL_EXPAND_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlExpandHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlExpandHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlContractHashTable = (PRTL_CONTRACT_HASH_TABLE)
        GetProcAddress(Rtl->NtdllModule, "RtlContractHashTable"))) {

        if (!(Rtl->RtlContractHashTable = (PRTL_CONTRACT_HASH_TABLE)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlContractHashTable"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlContractHashTable'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInitializeBitMap = (PRTL_INITIALIZE_BITMAP)
        GetProcAddress(Rtl->NtdllModule, "RtlInitializeBitMap"))) {

        if (!(Rtl->RtlInitializeBitMap = (PRTL_INITIALIZE_BITMAP)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInitializeBitMap"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInitializeBitMap'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlClearBit = (PRTL_CLEAR_BIT)
        GetProcAddress(Rtl->NtdllModule, "RtlClearBit"))) {

        if (!(Rtl->RtlClearBit = (PRTL_CLEAR_BIT)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlClearBit"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlClearBit'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlSetBit = (PRTL_SET_BIT)
        GetProcAddress(Rtl->NtdllModule, "RtlSetBit"))) {

        if (!(Rtl->RtlSetBit = (PRTL_SET_BIT)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlSetBit"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlSetBit'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlTestBit = (PRTL_TEST_BIT)
        GetProcAddress(Rtl->NtdllModule, "RtlTestBit"))) {

        if (!(Rtl->RtlTestBit = (PRTL_TEST_BIT)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlTestBit"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlTestBit'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlClearAllBits = (PRTL_CLEAR_ALL_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlClearAllBits"))) {

        if (!(Rtl->RtlClearAllBits = (PRTL_CLEAR_ALL_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlClearAllBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlClearAllBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlSetAllBits = (PRTL_SET_ALL_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlSetAllBits"))) {

        if (!(Rtl->RtlSetAllBits = (PRTL_SET_ALL_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlSetAllBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlSetAllBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindClearBits = (PRTL_FIND_CLEAR_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlFindClearBits"))) {

        if (!(Rtl->RtlFindClearBits = (PRTL_FIND_CLEAR_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindClearBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindClearBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindSetBits = (PRTL_FIND_SET_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlFindSetBits"))) {

        if (!(Rtl->RtlFindSetBits = (PRTL_FIND_SET_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindSetBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindSetBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindClearBitsAndSet = (PRTL_FIND_CLEAR_BITS_AND_SET)
        GetProcAddress(Rtl->NtdllModule, "RtlFindClearBitsAndSet"))) {

        if (!(Rtl->RtlFindClearBitsAndSet = (PRTL_FIND_CLEAR_BITS_AND_SET)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindClearBitsAndSet"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindClearBitsAndSet'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindSetBitsAndClear = (PRTL_FIND_SET_BITS_AND_CLEAR)
        GetProcAddress(Rtl->NtdllModule, "RtlFindSetBitsAndClear"))) {

        if (!(Rtl->RtlFindSetBitsAndClear = (PRTL_FIND_SET_BITS_AND_CLEAR)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindSetBitsAndClear"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindSetBitsAndClear'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlClearBits = (PRTL_CLEAR_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlClearBits"))) {

        if (!(Rtl->RtlClearBits = (PRTL_CLEAR_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlClearBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlClearBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlSetBits = (PRTL_SET_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlSetBits"))) {

        if (!(Rtl->RtlSetBits = (PRTL_SET_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlSetBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlSetBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindClearRuns = (PRTL_FIND_CLEAR_RUNS)
        GetProcAddress(Rtl->NtdllModule, "RtlFindClearRuns"))) {

        if (!(Rtl->RtlFindClearRuns = (PRTL_FIND_CLEAR_RUNS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindClearRuns"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindClearRuns'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindLongestRunClear = (PRTL_FIND_LONGEST_RUN_CLEAR)
        GetProcAddress(Rtl->NtdllModule, "RtlFindLongestRunClear"))) {

        if (!(Rtl->RtlFindLongestRunClear = (PRTL_FIND_LONGEST_RUN_CLEAR)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindLongestRunClear"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindLongestRunClear'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlNumberOfClearBits = (PRTL_NUMBER_OF_CLEAR_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlNumberOfClearBits"))) {

        if (!(Rtl->RtlNumberOfClearBits = (PRTL_NUMBER_OF_CLEAR_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlNumberOfClearBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlNumberOfClearBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlNumberOfSetBits = (PRTL_NUMBER_OF_SET_BITS)
        GetProcAddress(Rtl->NtdllModule, "RtlNumberOfSetBits"))) {

        if (!(Rtl->RtlNumberOfSetBits = (PRTL_NUMBER_OF_SET_BITS)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlNumberOfSetBits"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlNumberOfSetBits'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlAreBitsClear = (PRTL_ARE_BITS_CLEAR)
        GetProcAddress(Rtl->NtdllModule, "RtlAreBitsClear"))) {

        if (!(Rtl->RtlAreBitsClear = (PRTL_ARE_BITS_CLEAR)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlAreBitsClear"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlAreBitsClear'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlAreBitsSet = (PRTL_ARE_BITS_SET)
        GetProcAddress(Rtl->NtdllModule, "RtlAreBitsSet"))) {

        if (!(Rtl->RtlAreBitsSet = (PRTL_ARE_BITS_SET)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlAreBitsSet"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlAreBitsSet'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindFirstRunClear = (PRTL_FIND_FIRST_RUN_CLEAR)
        GetProcAddress(Rtl->NtdllModule, "RtlFindFirstRunClear"))) {

        if (!(Rtl->RtlFindFirstRunClear = (PRTL_FIND_FIRST_RUN_CLEAR)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindFirstRunClear"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindFirstRunClear'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindNextForwardRunClear = (PRTL_FIND_NEXT_FORWARD_RUN_CLEAR)
        GetProcAddress(Rtl->NtdllModule, "RtlFindNextForwardRunClear"))) {

        if (!(Rtl->RtlFindNextForwardRunClear = (PRTL_FIND_NEXT_FORWARD_RUN_CLEAR)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindNextForwardRunClear"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindNextForwardRunClear'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindLastBackwardRunClear = (PRTL_FIND_LAST_BACKWARD_RUN_CLEAR)
        GetProcAddress(Rtl->NtdllModule, "RtlFindLastBackwardRunClear"))) {

        if (!(Rtl->RtlFindLastBackwardRunClear = (PRTL_FIND_LAST_BACKWARD_RUN_CLEAR)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindLastBackwardRunClear"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindLastBackwardRunClear'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInitializeUnicodePrefix = (PRTL_INITIALIZE_UNICODE_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "RtlInitializeUnicodePrefix"))) {

        if (!(Rtl->RtlInitializeUnicodePrefix = (PRTL_INITIALIZE_UNICODE_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInitializeUnicodePrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInitializeUnicodePrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlInsertUnicodePrefix = (PRTL_INSERT_UNICODE_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "RtlInsertUnicodePrefix"))) {

        if (!(Rtl->RtlInsertUnicodePrefix = (PRTL_INSERT_UNICODE_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlInsertUnicodePrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlInsertUnicodePrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlRemoveUnicodePrefix = (PRTL_REMOVE_UNICODE_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "RtlRemoveUnicodePrefix"))) {

        if (!(Rtl->RtlRemoveUnicodePrefix = (PRTL_REMOVE_UNICODE_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlRemoveUnicodePrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlRemoveUnicodePrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlFindUnicodePrefix = (PRTL_FIND_UNICODE_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "RtlFindUnicodePrefix"))) {

        if (!(Rtl->RtlFindUnicodePrefix = (PRTL_FIND_UNICODE_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlFindUnicodePrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlFindUnicodePrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlNextUnicodePrefix = (PRTL_NEXT_UNICODE_PREFIX)
        GetProcAddress(Rtl->NtdllModule, "RtlNextUnicodePrefix"))) {

        if (!(Rtl->RtlNextUnicodePrefix = (PRTL_NEXT_UNICODE_PREFIX)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlNextUnicodePrefix"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlNextUnicodePrefix'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlCopyUnicodeString = (PRTL_COPY_UNICODE_STRING)
        GetProcAddress(Rtl->NtdllModule, "RtlCopyUnicodeString"))) {

        if (!(Rtl->RtlCopyUnicodeString = (PRTL_COPY_UNICODE_STRING)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlCopyUnicodeString"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlCopyUnicodeString'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlAppendUnicodeToString = (PRTL_APPEND_UNICODE_TO_STRING)
        GetProcAddress(Rtl->NtdllModule, "RtlAppendUnicodeToString"))) {

        if (!(Rtl->RtlAppendUnicodeToString = (PRTL_APPEND_UNICODE_TO_STRING)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlAppendUnicodeToString"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlAppendUnicodeToString'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlAppendUnicodeStringToString = (PRTL_APPEND_UNICODE_STRING_TO_STRING)
        GetProcAddress(Rtl->NtdllModule, "RtlAppendUnicodeStringToString"))) {

        if (!(Rtl->RtlAppendUnicodeStringToString = (PRTL_APPEND_UNICODE_STRING_TO_STRING)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlAppendUnicodeStringToString"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlAppendUnicodeStringToString'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlCompareMemory = (PRTL_COMPARE_MEMORY)
        GetProcAddress(Rtl->NtdllModule, "RtlCompareMemory"))) {

        if (!(Rtl->RtlCompareMemory = (PRTL_COMPARE_MEMORY)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlCompareMemory"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlCompareMemory'");
            return FALSE;
        }
    }

    if (!(Rtl->RtlPrefetchMemoryNonTemporal = (PRTL_PREFETCH_MEMORY_NON_TEMPORAL)
        GetProcAddress(Rtl->NtdllModule, "RtlPrefetchMemoryNonTemporal"))) {

        if (!(Rtl->RtlPrefetchMemoryNonTemporal = (PRTL_PREFETCH_MEMORY_NON_TEMPORAL)
            GetProcAddress(Rtl->NtosKrnlModule, "RtlPrefetchMemoryNonTemporal"))) {

            OutputDebugStringA("Rtl: failed to resolve 'RtlPrefetchMemoryNonTemporal'");
            return FALSE;
        }
    }

    //
    // End of auto-generated function resolutions.
    //

    return TRUE;
}


RTL_API
BOOLEAN
RtlCheckBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG BitPosition
    )
{
    return BitTest64((LONG64 const *)BitMapHeader->Buffer, (LONG64)BitPosition);
}

//
// Functions for Splay Macros
//

RTL_API
VOID
RtlInitializeSplayLinks(
    _Out_ PRTL_SPLAY_LINKS Links
    )
{
    Links->Parent = Links;
    Links->LeftChild = NULL;
    Links->RightChild = NULL;
}

RTL_API
PRTL_SPLAY_LINKS
RtlParent(_In_ PRTL_SPLAY_LINKS Links)
{
    return Links->Parent;
}

RTL_API
PRTL_SPLAY_LINKS
RtlLeftChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return Links->LeftChild;
}

RTL_API
PRTL_SPLAY_LINKS
RtlRightChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return Links->RightChild;
}

RTL_API
BOOLEAN
RtlIsRoot(_In_ PRTL_SPLAY_LINKS Links)
{
    return (RtlParent(Links) == Links);
}

RTL_API
BOOLEAN
RtlIsLeftChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return (RtlLeftChild(RtlParent(Links)) == Links);
}

RTL_API
BOOLEAN
RtlIsRightChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return (RtlRightChild(RtlParent(Links)) == Links);
}

RTL_API
VOID
RtlInsertAsLeftChild (
    _Inout_ PRTL_SPLAY_LINKS ParentLinks,
    _Inout_ PRTL_SPLAY_LINKS ChildLinks
    )
{
    ParentLinks->LeftChild = ChildLinks;
    ChildLinks->Parent = ParentLinks;
}

RTL_API
VOID
RtlInsertAsRightChild (
    _Inout_ PRTL_SPLAY_LINKS ParentLinks,
    _Inout_ PRTL_SPLAY_LINKS ChildLinks
    )
{
    ParentLinks->RightChild = ChildLinks;
    ChildLinks->Parent = ParentLinks;
}


_Check_return_
BOOL
LoadRtlExFunctions(
    _In_opt_ HMODULE RtlExModule,
    _Inout_  PRTLEXFUNCTIONS RtlExFunctions
    )
{
    if (!RtlExModule) {
        return FALSE;
    }

    if (!RtlExFunctions) {
        return FALSE;
    }

    //
    // Start of auto-generated section.
    //

    if (!(RtlExFunctions->RtlCheckBit = (PRTL_CHECK_BIT)
        GetProcAddress(RtlExModule, "RtlCheckBit"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlCheckBit'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlInitializeSplayLinks = (PRTL_INITIALIZE_SPLAY_LINKS)
        GetProcAddress(RtlExModule, "RtlInitializeSplayLinks"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlInitializeSplayLinks'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlParent = (PRTL_PARENT)
        GetProcAddress(RtlExModule, "RtlParent"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlParent'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlLeftChild = (PRTL_LEFT_CHILD)
        GetProcAddress(RtlExModule, "RtlLeftChild"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlLeftChild'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlRightChild = (PRTL_RIGHT_CHILD)
        GetProcAddress(RtlExModule, "RtlRightChild"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlRightChild'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlIsRoot = (PRTL_IS_ROOT)
        GetProcAddress(RtlExModule, "RtlIsRoot"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlIsRoot'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlIsLeftChild = (PRTL_IS_LEFT_CHILD)
        GetProcAddress(RtlExModule, "RtlIsLeftChild"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlIsLeftChild'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlIsRightChild = (PRTL_IS_RIGHT_CHILD)
        GetProcAddress(RtlExModule, "RtlIsRightChild"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlIsRightChild'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlInsertAsLeftChild = (PRTL_INSERT_AS_LEFT_CHILD)
        GetProcAddress(RtlExModule, "RtlInsertAsLeftChild"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlInsertAsLeftChild'");
        return FALSE;
    }

    if (!(RtlExFunctions->RtlInsertAsRightChild = (PRTL_INSERT_AS_RIGHT_CHILD)
        GetProcAddress(RtlExModule, "RtlInsertAsRightChild"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'RtlInsertAsRightChild'");
        return FALSE;
    }

    if (!(RtlExFunctions->CopyToMemoryMappedMemory = (PCOPYTOMEMORYMAPPEDMEMORY)
        GetProcAddress(RtlExModule, "CopyToMemoryMappedMemory"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'CopyToMemoryMappedMemory'");
        return FALSE;
    }

    if (!(RtlExFunctions->FindCharsInUnicodeString = (PFIND_CHARS_IN_UNICODE_STRING)
        GetProcAddress(RtlExModule, "FindCharsInUnicodeString"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'FindCharsInUnicodeString'");
        return FALSE;
    }

    if (!(RtlExFunctions->CreateBitmapIndexForUnicodeString = (PCREATE_BITMAP_INDEX_FOR_UNICODE_STRING)
        GetProcAddress(RtlExModule, "CreateBitmapIndexForUnicodeString"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'CreateBitmapIndexForUnicodeString'");
        return FALSE;
    }

    if (!(RtlExFunctions->FilesExist = (PFILES_EXIST)
        GetProcAddress(RtlExModule, "FilesExist"))) {

        OutputDebugStringA("RtlEx: failed to resolve 'FilesExist'");
        return FALSE;
    }

    //
    // End of auto-generated section.
    //

    return TRUE;
}

_Check_return_
BOOL
LoadRtlExSymbols(
    _In_opt_ HMODULE RtlExModule,
    _Inout_  PRTL    Rtl
)
{
    HMODULE Module;

    if (!Rtl) {
        return FALSE;
    }

    if (RtlExModule) {
        Module = RtlExModule;

    } else {

        DWORD Flags = (
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS          |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
        );

        if (!GetModuleHandleEx(Flags, (LPCTSTR)&LoadRtlExFunctions, &Module)) {
            return FALSE;
        }

        if (!Module) {
            return FALSE;
        }
    }

    if (!LoadRtlExFunctions(Module, &Rtl->RtlExFunctions)) {
        return FALSE;
    }

    return TRUE;

}

BOOL
InitializeRtl(
    _Out_bytecap_(*SizeOfRtl) PRTL   Rtl,
    _Inout_                   PULONG SizeOfRtl
    )
{
    if (!Rtl) {
        if (SizeOfRtl) {
            *SizeOfRtl = sizeof(*Rtl);
        }
        return FALSE;
    }

    if (!SizeOfRtl) {
        return FALSE;
    }

    if (*SizeOfRtl < sizeof(*Rtl)) {
        *SizeOfRtl = sizeof(*Rtl);
        return FALSE;
    } else {
        *SizeOfRtl = sizeof(*Rtl);
    }

    SecureZeroMemory(Rtl, sizeof(*Rtl));

    if (!LoadRtlSymbols(Rtl)) {
        return FALSE;
    }

    if (!LoadRtlExSymbols(NULL, Rtl)) {
        return FALSE;
    }

    return TRUE;
}

VOID
Debugbreak()
{
    __debugbreak();
}

