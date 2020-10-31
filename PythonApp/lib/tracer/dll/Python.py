#===============================================================================
# Imports
#===============================================================================

from ..util import (
    Constant,
)

from ..wintypes import (
    sizeof,

    Structure,
    Union,

    BOOL,
    BYTE,
    SHORT,
    USHORT,
    PUSHORT,
    LONG,
    ULONG,
    PULONG,
    PVOID,
    STRING,
    PSTRING,
    POINTER,
    CFUNCTYPE,

    HMODULE,
    ULONGLONG,
    ULONG_PTR,

    LIST_ENTRY,
    RTL_SPLAY_LINKS,
    RTL_GENERIC_TABLE,
    RTL_DYNAMIC_HASH_TABLE_ENTRY,
)

from .Rtl import (
    PRTL,
    RTL_FILE,
    PRTL_FILE
)

#===============================================================================
# Enums/Constants
#===============================================================================

class PythonPathEntryType(Constant):
    IsModuleDirectory       =        1
    IsNonModuleDirectory    = (1 <<  1)
    IsFileSystemDirectory   = (1 <<  2)
    IsFile                  = (1 <<  3)
    IsClass                 = (1 <<  4)
    IsFunction              = (1 <<  5)
    IsSpecial               = (1 <<  6)
    IsValid                 = (1 <<  7)
    IsDll                   = (1 <<  8)
    IsC                     = (1 <<  9)
    IsBuiltin               = (1 << 10)
    IsInitPy                = (1 << 11)

class PYTHON_PATH_ENTRY_TYPE(Structure):
    _fields_ = [
        ('IsModuleDirectory', ULONG, 1),
        ('IsNonModuleDirectory', ULONG, 1),
        ('IsFileSystemDirectory', ULONG, 1),
        ('IsFile', ULONG, 1),
        ('IsClass', ULONG, 1),
        ('IsFunction', ULONG, 1),
        ('IsSpecial', ULONG, 1),
        ('IsValid', ULONG, 1),
        ('IsDll', ULONG, 1),
        ('IsC', ULONG, 1),
        ('IsBuiltin', ULONG, 1),
        ('IsInitPy', ULONG, 1),
    ]

#===============================================================================
# Classes
#===============================================================================

class PYTHON_PATH_TABLE_ENTRY(Structure):
    @classmethod
    def _get_dtype(cls):
        import numpy as np
        return np.dtype([
            # PATH_TABLE_ENTRY
            # PREFIX_TABLE_ENTRY
            ('PrefixTreeNodeTypeCode', np.int16),       #   2       40      42
            ('PrefixTreeNameLength', np.int16),         #   2       42      44
            ('PathEntryType', np.uint32),               #   4       44      48
            ('NextPrefixTree', np.uint64),              #   8       48      56
            # RTL_SPLAY_LINKS
            ('SplayParent', np.uint64),                 #   8       16      24
            ('SplayLeftChild', np.uint64),              #   8       24      32
            ('SplayRightChild', np.uint64),             #   8       32      40
            ('Prefix', np.uint64),                      #   8       40      48

            ('PathLength', np.uint16),
            ('PathMaximumLength', np.uint16),
            ('PathHash', np.uint32),
            ('PathBuffer', np.uint64),

            ('FullNameLength', np.uint16),
            ('FullNameMaximumLength', np.uint16),
            ('FullNameHash', np.uint32),
            ('FullNameBuffer', np.uint64),

            ('ModuleNameLength', np.uint16),
            ('ModuleNameMaximumLength', np.uint16),
            ('ModuleNameHash', np.uint32),
            ('ModuleNameBuffer', np.uint64),

            ('NameLength', np.uint16),
            ('NameMaximumLength', np.uint16),
            ('NameHash', np.uint32),
            ('NameBuffer', np.uint64),

            ('ClassNameLength', np.uint16),
            ('ClassNameMaximumLength', np.uint16),
            ('ClassNameHash', np.uint32),
            ('ClassNameBuffer', np.uint64),
        ])

PPYTHON_PATH_TABLE_ENTRY = POINTER(PYTHON_PATH_TABLE_ENTRY)

class PYTHON_PATH_TABLE(Structure):
    pass
PPYTHON_PATH_TABLE = POINTER(PYTHON_PATH_TABLE)

class _PYTHON_PATH_TABLE_ENTRY(Structure):
    pass

class PYTHON_PATH_TABLE_ENTRY(Structure):

    @property
    def path(self):
        return self.Path.Buffer[:
            self.Path.Length
        ] if self.Path.Buffer else ''

    @property
    def fullname(self):
        return self.FullName.Buffer[:
            self.FullName.Length
        ] if self.FullName.Buffer else ''

    @property
    def modulename(self):
        return self.ModuleName.Buffer[:
            self.ModuleName.Length
        ] if self.ModuleName.Buffer else ''

    @property
    def classname(self):
        return self.ClassName.Buffer[:
            self.ClassName.Length
        ] if self.ClassName.Buffer else ''

    @property
    def name(self):
        return self.Name.Buffer[:self.Name.Length] \
            if self.Name.Buffer else ''

    @property
    def is_valid(self):
        return self.PrefixTableEntry.PathEntryType.IsValid

PPYTHON_PATH_TABLE_ENTRY = POINTER(PYTHON_PATH_TABLE_ENTRY)

class PYTHON_FUNCTION(Structure):

    _exclude = (
        'Unused',
        'CodeObject',
        'CodeLineNumbers',
        'Histogram',
        'LineNumbersBitmap',
    )

    @classmethod
    def _get_dtype(cls):
        import numpy as np
        return np.dtype([
            # PATH_TABLE_ENTRY
            # PREFIX_TABLE_ENTRY
            ('PrefixTreeNodeTypeCode', np.int16),       #   2       40      42
            ('PrefixTreeNameLength', np.int16),         #   2       42      44
            ('PathEntryType', np.uint32),               #   4       44      48
            ('NextPrefixTree', np.uint64),              #   8       48      56
            # RTL_SPLAY_LINKS
            ('SplayParent', np.uint64),                 #   8       16      24
            ('SplayLeftChild', np.uint64),              #   8       24      32
            ('SplayRightChild', np.uint64),             #   8       32      40
            ('Prefix', np.uint64),                      #   8       40      48

            ('PathLength', np.uint16),
            ('PathMaximumLength', np.uint16),
            ('PathHash', np.int32),
            ('PathBuffer', np.uint64),

            ('FullNameLength', np.uint16),
            ('FullNameMaximumLength', np.uint16),
            ('FullNameHash', np.int32),
            ('FullNameBuffer', np.uint64),

            ('ModuleNameLength', np.uint16),
            ('ModuleNameMaximumLength', np.uint16),
            ('ModuleNameHash', np.int32),
            ('ModuleNameBuffer', np.uint64),

            ('NameLength', np.uint16),
            ('NameMaximumLength', np.uint16),
            ('NameHash', np.int32),
            ('NameBuffer', np.uint64),

            ('ClassNameLength', np.uint16),
            ('ClassNameMaximumLength', np.uint16),
            ('ClassNameHash', np.int32),
            ('ClassNameBuffer', np.uint64),
            # End of PYTHON_PATH_TABLE_ENTRY

            ('ParentPathEntry', np.uint64),
            ('CodeObject', np.uint64),
            ('LineNumbersBitmap', np.uint64),
            ('Histogram', np.uint64),
            ('CodeLineNumbers', np.uint64),

            ('ReferenceCount', np.uint32),
            ('CodeObjectHash', np.uint32),
            ('FunctionHash', np.uint32),
            ('Unused1', np.uint32),

            ('FirstLineNumber', np.uint16),
            ('NumberOfLines', np.uint16),
            ('NumberOfCodeLines', np.uint16),
            ('SizeOfByteCode', np.uint16),

            ('Unused2', np.uint64),
            ('Unused3', np.uint64),
            ('Unused4', np.uint64),
        ])

    @property
    def path(self):
        return self.PathEntry.Path.Buffer[:
            self.PathEntry.Path.Length
        ] if self.PathEntry.Path.Buffer else ''

    @property
    def fullname(self):
        return self.PathEntry.FullName.Buffer[:
            self.PathEntry.FullName.Length
        ] if self.PathEntry.FullName.Buffer else ''

    @property
    def modulename(self):
        return self.PathEntry.ModuleName.Buffer[:
            self.PathEntry.ModuleName.Length
        ] if self.PathEntry.ModuleName.Buffer else ''

    @property
    def classname(self):
        return self.PathEntry.ClassName.Buffer[:
            self.PathEntry.ClassName.Length
        ] if self.PathEntry.ClassName.Buffer else ''

    @property
    def name(self):
        return self.PathEntry.Name.Buffer[:self.PathEntry.Name.Length] \
            if self.PathEntry.Name.Buffer else ''

    @property
    def is_valid(self):
        return self.PathEntry.PrefixTableEntry.PathEntryType.IsValid

PPYTHON_FUNCTION = POINTER(PYTHON_FUNCTION)

class PYTHON_FUNCTION_TABLE_ENTRY(Structure):
    _exclude = (
        'Unused',
        'NextPrefixTree',
    )
    @classmethod
    def _get_dtype(cls):
        import numpy as np
        return np.dtype([
            # TABLE_ENTRY_HEADER
            ('HeaderSplayParent', np.uint64),           #   8       0       8
            ('HeaderSplayLeftChild', np.uint64),        #   8       8       16
            ('HeaderSplayRightChild', np.uint64),       #   8       16      24
            ('HeaderFlink', np.uint64),                 #   8       24      32
            ('HeaderBlink', np.uint64),                 #   8       32      40
            # PATH_TABLE_ENTRY
            # PREFIX_TABLE_ENTRY
            ('PrefixTreeNodeTypeCode', np.int16),       #   2       40      42
            ('PrefixTreeNameLength', np.int16),         #   2       42      44
            ('PathEntryType', np.uint32),               #   4       44      48
            ('NextPrefixTree', np.uint64),              #   8       48      56
            # RTL_SPLAY_LINKS
            ('SplayParent', np.uint64),                 #   8       16      24
            ('SplayLeftChild', np.uint64),              #   8       24      32
            ('SplayRightChild', np.uint64),             #   8       32      40
            ('Prefix', np.uint64),                      #   8       40      48

            ('PathLength', np.uint16),
            ('PathMaximumLength', np.uint16),
            ('PathHash', np.int32),
            ('PathBuffer', np.uint64),

            ('FullNameLength', np.uint16),
            ('FullNameMaximumLength', np.uint16),
            ('FullNameHash', np.int32),
            ('FullNameBuffer', np.uint64),

            ('ModuleNameLength', np.uint16),
            ('ModuleNameMaximumLength', np.uint16),
            ('ModuleNameHash', np.int32),
            ('ModuleNameBuffer', np.uint64),

            ('NameLength', np.uint16),
            ('NameMaximumLength', np.uint16),
            ('NameHash', np.int32),
            ('NameBuffer', np.uint64),

            ('ClassNameLength', np.uint16),
            ('ClassNameMaximumLength', np.uint16),
            ('ClassNameHash', np.int32),
            ('ClassNameBuffer', np.uint64),
            # End of PYTHON_PATH_TABLE_ENTRY

            ('ParentPathEntry', np.uint64),
            ('CodeObject', np.uint64),
            ('LineNumbersBitmap', np.uint64),
            ('Histogram', np.uint64),
            ('CodeLineNumbers', np.uint64),

            ('ReferenceCount', np.uint32),
            ('CodeObjectHash', np.int32),
            ('FunctionHash', np.uint32),
            ('Unused1', np.uint32),

            ('FirstLineNumber', np.uint16),
            ('NumberOfLines', np.uint16),
            ('NumberOfCodeLines', np.uint16),
            ('SizeOfByteCode', np.uint16),

            ('Unused2', np.uint64),
            ('Unused3', np.uint64),
            ('Unused4', np.uint64),
        ])

    @property
    def is_valid(self):
        return self.Function.PathEntry.PrefixTableEntry.PathEntryType.IsValid

PPYTHON_FUNCTION_TABLE_ENTRY = POINTER(PYTHON_FUNCTION_TABLE_ENTRY)

class PYTHON_FUNCTION_TABLE(Structure):
    _fields_ = [
        ('GenericTable', RTL_GENERIC_TABLE),
    ]
PPYTHON_FUNCTION_TABLE = POINTER(PYTHON_FUNCTION_TABLE)

class PYTHON_TRACE_EVENT(Structure):
    pass
PPYTHON_TRACE_EVENT = POINTER(PYTHON_TRACE_EVENT)

_PYTHON_PATH_TABLE_ENTRY._fields_ = [
    ('NodeTypeCode', SHORT),
    ('PrefixNameLength', SHORT),
    ('PathEntryType', PYTHON_PATH_ENTRY_TYPE),
    ('NextPrefixTree', PPYTHON_PATH_TABLE_ENTRY),
    ('Links', RTL_SPLAY_LINKS),
    ('Prefix', PSTRING),
]

PYTHON_PATH_TABLE_ENTRY._fields_ = [
    ('PrefixTableEntry', _PYTHON_PATH_TABLE_ENTRY),
    ('Path', STRING),
    ('FullName', STRING),
    ('ModuleName', STRING),
    ('Name', STRING),
    ('ClassName', STRING),
    ('File', RTL_FILE),
    ('Reserved', BYTE * 384),
]

class _PYTHON_FUNCTION_CODEOBJECT_INNER(Structure):
    _fields_ = [
        ('CodeLineNumbers', PUSHORT),
        ('CodeObjectHash', LONG),
        ('FirstLineNumber', USHORT),
        ('NumberOfLines', USHORT),
        ('NumberOfCodeLines', USHORT),
        ('SizeOfByteCode', USHORT),
    ]

class _PYTHON_FUNCTION_PYCFUNCTION_INNER(Structure):
    _fields_ = [
        ('ModuleHandle', HMODULE),
    ]

class _PYTHON_FUNCTION_INNER_UNION(Union):
    _fields_ = [
        ('PythonCodeObject', _PYTHON_FUNCTION_CODEOBJECT_INNER),
        ('PyCFunction', _PYTHON_FUNCTION_PYCFUNCTION_INNER),
    ]


PYTHON_FUNCTION._fields_ = [
    ('PathEntry', PYTHON_PATH_TABLE_ENTRY),
    ('ParentPathEntry', PPYTHON_PATH_TABLE_ENTRY),
    ('Key', ULONG_PTR),
    ('CodeObject', PVOID),
    ('PyCFunctionObject', PVOID),
    ('MaxCallStackDepth', ULONG),
    ('CallCount', ULONG),
    ('u', _PYTHON_FUNCTION_INNER_UNION),
    ('ListEntry', LIST_ENTRY),
    ('Signature', ULONG_PTR),
    ('HashEntry', RTL_DYNAMIC_HASH_TABLE_ENTRY),
    ('Reserved', BYTE * 864),
    ('Dummy', ULONGLONG),
]

PYTHON_FUNCTION_TABLE_ENTRY._fields_ = [
    ('Links', RTL_SPLAY_LINKS),
    ('ListEntry', LIST_ENTRY),
    ('Function', PYTHON_FUNCTION),
]

SizeOfPythonFunctionTableEntry = sizeof(PYTHON_FUNCTION_TABLE_ENTRY)

class PYTHON(Structure):
    pass
PPYTHON = POINTER(PYTHON)

#===============================================================================
# Functions
#===============================================================================

INITIALIZE_PYTHON = CFUNCTYPE(BOOL, PRTL, HMODULE, PPYTHON, PULONG)

# vim:set ts=8 sw=4 sts=4 tw=80 et                                             :
