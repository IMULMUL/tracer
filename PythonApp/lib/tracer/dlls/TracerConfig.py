#===============================================================================
# Imports
#===============================================================================
import sys
from functools import partial

import ctypes
from ctypes import *
from ctypes.wintypes import *

from ..wintypes import *
from ..types import *

from ..util import NullObject

from .Allocator import (
    ALLOCATOR,
    PALLOCATOR,
    PPALLOCATOR,
)

#===============================================================================
# Globals
#===============================================================================

#===============================================================================
# Aliases
#===============================================================================

#===============================================================================
# Helpers
#===============================================================================

#===============================================================================
# Structures
#===============================================================================

class TRACER_PATHS(Structure):
    _fields_ = [
        ('Size', USHORT),
        ('Padding', USHORT * 3),
        ('InstallationDirectory', UNICODE_STRING),
        ('BaseTraceDirectory', UNICODE_STRING),
        ('DefaultPythonDirectory', UNICODE_STRING),
        ('RtlDllPath', UNICODE_STRING),
        ('PythonDllPath', UNICODE_STRING),
        ('TracerHeapDllPath', UNICODE_STRING),
        ('TraceStoreDllPath', UNICODE_STRING),
        ('StringTableDllPath', UNICODE_STRING),
        ('PythonTracerDllPath', UNICODE_STRING),
        ('TlsTracerHeapDllPath', UNICODE_STRING),
        ('TracedPythonSessionDllPath', UNICODE_STRING),
    ]
PTRACER_PATHS = POINTER(TRACER_PATHS)

class TRACER_FLAGS(Structure):
    _fields_ = [
        ('DebugBreakOnEntry', ULONG, 1),
        ('LoadDebugLibraries', ULONG, 1),
        ('DisableTraceSessionDirectoryCompression', ULONG, 1),
        ('DisablePrefaultPages', ULONG, 1),
        ('EnableMemoryTracing', ULONG, 1),
        ('EnableIoCounterTracing', ULONG, 1),
        ('EnableHandleCountTracing', ULONG, 1),
        ('DisableFileFlagOverlapped', ULONG, 1),
        ('DisableFileFlagSequentialScan', ULONG, 1),
        ('EnableFileFlagRandomAccess', ULONG, 1),
        ('EnableFileFlagWriteThrough', ULONG, 1),
    ]
PTRACER_FLAGS = POINTER(TRACER_FLAGS)

class TRACE_SESSION_DIRECTORY(Structure):
    _fields_ = [
        ('ListEntry', LIST_ENTRY),
        ('Directory', UNICODE_STRING),
        ('Context', PVOID),
    ]
PTRACE_SESSION_DIRECTORY = POINTER(TRACE_SESSION_DIRECTORY)

class TRACE_SESSION_DIRECTORIES(Structure):
    _fields_ = [
        ('ListHead', LIST_ENTRY),
        ('Lock', SRWLOCK),
        ('Count', ULONG),
    ]
PTRACE_SESSION_DIRECTORIES = POINTER(TRACE_SESSION_DIRECTORIES)

class TRACER_CONFIG(Structure):
    _fields_ = [
        ('Size', USHORT),
        ('Padding1', USHORT),
        ('Flags', TRACER_FLAGS),
        ('Allocator', PALLOCATOR),
        ('TlsTracerHeapModule', HMODULE),
        ('ListEntry', LIST_ENTRY),
        ('Paths', TRACER_PATHS),
        ('TraceSessionDirectories', TRACE_SESSION_DIRECTORIES),
    ]
PTRACER_CONFIG = POINTER(TRACER_CONFIG)

#===============================================================================
# Function Prototypes
#===============================================================================



#===============================================================================
# Functions
#===============================================================================
def vspyprof(path=None, dll=None):
    assert path or dll
    if not dll:
        dll = ctypes.PyDLL(path)

    dll.CreateProfiler.restype = c_void_p
    dll.CreateCustomProfiler.restype = c_void_p
    dll.CreateCustomProfiler.argtypes = [c_void_p, ctypes.c_void_p]
    dll.CloseThread.argtypes = [c_void_p]
    dll.CloseProfiler.argtypes = [c_void_p]
    dll.InitProfiler.argtypes = [c_void_p]
    dll.InitProfiler.restype = c_void_p

    #dll.SetTracing.argtypes = [c_void_p]
    #dll.UnsetTracing.argtypes = [c_void_p]
    #dll.IsTracing.argtypes = [c_void_p]
    #dll.IsTracing.restype = c_bool

    return dll

def pytrace(path=None, dll=None):
    assert path or dll
    dll = vspyprof(path, dll)

    dll.CreateTracer.restype = PVOID
    dll.CreateTracer.argtypes = [PVOID, PVOID]

    dll.InitializeTraceStores.restype = BOOL
    dll.InitializeTraceStores.argtypes = [
        PRTL,
        PWSTR,
        PVOID,
        PDWORD,
        PDWORD,
    ]

    return dll

def rtl(path=None, dll=None):
    assert path or dll
    if not dll:
        dll = ctypes.PyDLL(path)

    dll.InitializeRtl.restype = BOOL
    dll.InitializeRtl.argtypes = [
        PRTL,
        PULONG,
    ]

    return dll

def tracer(path=None, dll=None):
    assert path or dll
    if not dll:
        dll = ctypes.PyDLL(path)

    dll.InitializeTraceStores.restype = BOOL
    dll.InitializeTraceStores.argtypes = [
        PRTL,
        PWSTR,
        PVOID,
        PDWORD,
        PDWORD,
    ]

    dll.InitializeTraceContext.restype = BOOL
    dll.InitializeTraceContext.argtypes = [
        PRTL,
        PTRACE_CONTEXT,
        PDWORD,
        PTRACE_SESSION,
        PTRACE_STORES,
        PVOID,
    ]

    dll.InitializeTraceSession.restype = BOOL
    dll.InitializeTraceSession.argtypes = [
        PRTL,
        PTRACE_SESSION,
        PDWORD
    ]

    dll.SubmitTraceStoreFileExtensionThreadpoolWork.restype = None
    dll.SubmitTraceStoreFileExtensionThreadpoolWork.argtypes = [ PTRACE_STORE, ]

    #dll.CallSystemTimer.restype = BOOL
    #dll.CallSystemTimer.argtypes = [
    #    PFILETIME,
    #    PVOID,
    #]

    return dll

def python(path=None, dll=None):
    assert path or dll
    if not dll:
        dll = ctypes.PyDLL(path)

    dll.InitializePython.restype = BOOL
    dll.InitializePython.argtypes = [
        PRTL,
        HMODULE,
        PVOID,
        PULONG,
    ]

    return dll

def pythontracer(path=None, dll=None):
    assert path or dll
    if not dll:
        dll = ctypes.PyDLL(path)

    dll.InitializePythonTraceContext.restype = BOOL
    dll.InitializePythonTraceContext.argtypes = [
        PRTL,
        PPYTHON_TRACE_CONTEXT,
        PULONG,
        PPYTHON,
        PTRACE_CONTEXT,
        PPYTRACEFUNC,
        PUSERDATA
    ]

    dll.AddFunction.restype = BOOL
    #dll.AddFunction.argtypes = [ PVOID, ]

    dll.StartTracing.restype = BOOL
    dll.StartTracing.argtypes = [ PPYTHON_TRACE_CONTEXT, ]

    dll.StopTracing.restype = BOOL
    dll.StopTracing.argtypes = [ PPYTHON_TRACE_CONTEXT, ]

    return dll

def sqlite3(path=None, dll=None):
    assert path or dll
    if not dll:
        dll = ctypes.PyDLL(path)

    return dll

#===============================================================================
# Decorators
#===============================================================================
class trace:
    def __init__(self, func):
        self.func = func
    def __get__(self, obj, objtype=None):
        if obj is None:
            return self.func
        return partial(self, obj)
    def __call__(self, *args, **kw):
        global TRACER
        tracer = TRACER
        if not tracer:
            tracer = NullObject()

        tracer.add_function(self.func)
        tracer.start()
        result = self.func(*args, **kwds)
        tracer.stop()
        return result

#===============================================================================
# Classes
#===============================================================================
class TracerError(BaseException):
    pass

class Tracer:
    def __init__(self,
                 basedir,
                 tracer_dll_path,
                 tracer_rtl_dll_path,
                 tracer_sqlite3_dll_path,
                 tracer_python_dll_path,
                 tracer_pythontracer_dll_path,
                 threadpool=None,
                 threadpool_callback_environment=None):

        self.basedir = basedir
        self.system_dll = sys.dllhandle

        self.tracer_dll_path = tracer_dll_path
        self.tracer_rtl_dll_path = tracer_rtl_dll_path
        self.tracer_sqlite3_dll_path = tracer_sqlite3_dll_path
        self.tracer_python_dll_path = tracer_python_dll_path
        self.tracer_pythontracer_dll_path = tracer_pythontracer_dll_path

        self.tracer_dll = tracer(self.tracer_dll_path)
        self.tracer_rtl_dll = rtl(self.tracer_rtl_dll_path)
        self.tracer_sqlite3_dll = sqlite3(self.tracer_sqlite3_dll_path)
        self.tracer_python_dll = python(self.tracer_python_dll_path)
        self.tracer_pythontracer_dll = (
            pythontracer(self.tracer_pythontracer_dll_path)
        )

        # Rtl needs to be initialized first.
        self.rtl = RTL()
        self.rtl_size = ULONG(sizeof(self.rtl))
        success = self.tracer_rtl_dll.InitializeRtl(
            byref(self.rtl),
            byref(self.rtl_size)
        )

        if not success:
            if self.rtl_size.value != sizeof(self.rtl):
                msg = "Warning: RTL size mismatch: %d != %d\n" % (
                    self.rtl_size.value,
                    sizeof(self.rtl)
                )
                sys.stderr.write(msg)
                self.rtl = create_string_buffer(
                    self.rtl_size.value
                )
                success = self.tracer_rtl_dll.InitializeRtl(
                    byref(self.rtl),
                    byref(self.rtl_size),
                )

            if not success:
                raise TracerError("InitializeRtl() failed")

        # The Python structure is complex; we haven't written a ctypes.Structure
        # wrapper for it yet.  So, for now, we use a raw buffer instead.
        self.python_size = ULONG()
        # Get the size required for the structure first.
        self.tracer_python_dll.InitializePython(
            None,
            None,
            None,
            byref(self.python_size),
        )
        self.python = create_string_buffer(self.python_size.value)
        success = self.tracer_python_dll.InitializePython(
            sys.dllhandle,
            byref(self.python),
            byref(self.python_size),
        )
        if not success:
            raise TracerError("InitializePython() failed")

        self.trace_session = TRACE_SESSION.create()

        self.trace_stores = TRACE_STORES()
        self.trace_stores_size = ULONG(sizeof(self.trace_stores))

        success = self.tracer_dll.InitializeTraceStores(
            self.rtl,
            self.basedir,
            byref(self.trace_stores),
            byref(self.trace_stores_size),
            None,
        )
        if not success:
            if self.trace_stores_size.value != sizeof(self.trace_stores):
                msg = "Warning: TRACE_STORES size mismatch: %d != %d\n" % (
                    self.trace_stores_size.value,
                    sizeof(self.trace_stores)
                )
                sys.stderr.write(msg)
                self.trace_stores = create_string_buffer(
                    self.trace_stores_size.value
                )
                success = self.tracer_dll.InitializeTraceStores(
                    self.rtl,
                    self.basedir,
                    byref(self.trace_stores),
                    byref(self.trace_stores_size),
                    None
                )

            if not success:
                raise TracerError("InitializeTraceStores() failed")

        if not threadpool:
            threadpool = kernel32.CreateThreadpool(None)
            if not threadpool:
                raise TracerError("CreateThreadpool() failed")
            num_cpus = cpu_count()
            kernel32.SetThreadpoolThreadMinimum(threadpool, num_cpus)
            kernel32.SetThreadpoolThreadMaximum(threadpool, num_cpus)

        self.threadpool = threadpool

        if not threadpool_callback_environment:
            threadpool_callback_environment = TP_CALLBACK_ENVIRON()
            InitializeThreadpoolEnvironment(threadpool_callback_environment)
            SetThreadpoolCallbackPool(
                threadpool_callback_environment,
                threadpool
            )

        self.threadpool_callback_environment = threadpool_callback_environment

        self.trace_context = TRACE_CONTEXT()
        self.trace_context_size = ULONG(sizeof(TRACE_CONTEXT))
        success = self.tracer_dll.InitializeTraceContext(
            byref(self.rtl),
            byref(self.trace_context),
            byref(self.trace_context_size),
            byref(self.trace_session),
            byref(self.trace_stores),
            byref(self.threadpool_callback_environment),
            None,
        )
        if not success:
            if self.trace_context_size.value != sizeof(self.trace_context):
                msg = "TRACE_CONTEXT size mismatch: %d != %d\n" % (
                    self.trace_context_size.value,
                    sizeof(self.trace_context)
                )
                sys.stderr.write(msg)
                self.trace_context = create_string_buffer(
                    self.trace_context_size.value
                )
                success = self.tracer_dll.InitializeTraceContext(
                    byref(self.rtl),
                    byref(self.trace_context),
                    byref(self.trace_context_size),
                    byref(self.trace_session),
                    byref(self.trace_stores),
                    byref(self.threadpool_callback_environment),
                    None,
                )

            if not success:
                kernel32.CloseThreadpool(self.threadpool)
                self.threadpool = None
                raise TracerError("InitializeTraceContext() failed")

        self.python_trace_context = PYTHON_TRACE_CONTEXT()
        self.python_trace_context_size = ULONG(sizeof(PYTHON_TRACE_CONTEXT))
        success = self.tracer_pythontracer_dll.InitializePythonTraceContext(
            byref(self.rtl),
            byref(self.trace_context),
            byref(self.python_trace_context),
            byref(self.python_trace_context_size),
            byref(self.python),
            byref(self.trace_context),
            self.tracer_pythontracer_dll.PyTraceCallbackFast,
            None,
        )
        if not success:
            raise TracerError("InitializePythonTraceContext() failed")

        global TRACER
        TRACER = self

    @classmethod
    def create_debug(cls, basedir, conf=None):
        if not conf:
            from .config import get_or_create_config
            conf = get_or_create_config()

        return cls(
            basedir,
            conf.tracer_debug_dll_path,
            conf.tracer_rtl_debug_dll_path,
            conf.tracer_sqlite3_debug_dll_path,
            conf.tracer_python_debug_dll_path,
            conf.tracer_pythontracer_debug_dll_path,
        )

    @classmethod
    def create_release(cls, basedir, conf=None):
        if not conf:
            from .config import get_or_create_config
            conf = get_or_create_config()

        return cls(
            basedir,
            conf.tracer_dll_path,
            conf.tracer_rtl_dll_path,
            conf.tracer_sqlite3_dll_path,
            conf.tracer_python_dll_path,
            conf.tracer_pythontracer_dll_path,
        )

    def add_function(self, func):
        dll = self.tracer_pythontracer_dll
        if not dll.AddFunction(self.python_trace_context, func):
            raise TracerError("AddFunction() failed")

    def start(self):
        dll = self.tracer_pythontracer_dll
        if not dll.StartTracing(self.python_trace_context):
            raise TracerError("StartTracing() failed")

    def stop(self):
        dll = self.tracer_pythontracer_dll
        if not dll.StopTracing(self.python_trace_context):
            raise TracerError("StopTracing() failed")

    def start_profiling(self):
        dll = self.tracer_pythontracer_dll
        if not dll.StartProfiling(self.python_trace_context):
            raise TracerError("StartProfiling() failed")

    def stop_profiling(self):
        dll = self.tracer_pythontracer_dll
        if not dll.StopProfiling(self.python_trace_context):
            raise TracerError("StopProfiling() failed")


    def close_trace_stores(self):
        self.tracer_dll.CloseTraceStores(byref(self.trace_stores))

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *exc_info):
        self.stop()

# vim:set ts=8 sw=4 sts=4 tw=80 ai et                                          :
