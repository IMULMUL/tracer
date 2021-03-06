/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    DebugEngineInterfaces.c

Abstract:

    This module declares interface GUID constants used by the DebugEngine
    component.

--*/

#include "stdafx.h"

//
// Locally define the IIDs of the interfaces we want in order to avoid the
// EXTERN_C linkage we'll pick up if we use the values directly from DbgEng.h.
//

//
// IID_IUnknown: 00000000-0000-0000-C000-000000000046
//

DEFINE_GUID_EX(IID_IUNKNOWN, 0x00000000, 0x0000, 0x0000,
               0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

//
// IID_IDebugAdvanced4: d1069067-2a65-4bf0-ae97-76184b67856b
//

DEFINE_GUID_EX(IID_IDEBUG_ADVANCED, 0xd1069067, 0x2a65, 0x4bf0,
               0xae, 0x97, 0x76, 0x18, 0x4b, 0x67, 0x85, 0x6b);

//
// IDebugSymbols5: c65fa83e-1e69-475e-8e0e-b5d79e9cc17e
//

DEFINE_GUID_EX(IID_IDEBUG_SYMBOLS, 0xc65fa83e, 0x1e69, 0x475e,
               0x8e, 0x0e, 0xb5, 0xd7, 0x9e, 0x9c, 0xc1, 0x7e);

//
// IDebugSymbolGroup:  f2528316-0f1a-4431-aeed-11d096e1e2ab
// IDebugSymbolGroup2: 6a7ccc5f-fb5e-4dcc-b41c-6c20307bccc7
//

DEFINE_GUID_EX(IID_IDEBUG_SYMBOLGROUP, 0xf2528316, 0x0f1a, 0x4431,
               0xae, 0xed, 0x11, 0xd0, 0x96, 0xe1, 0xe2, 0xab);

//DEFINE_GUID_EX(IID_IDEBUG_SYMBOLGROUP, 0x6a7ccc5f, 0xfb5e, 0x4dcc,
//               0xb4, 0x1c, 0x6c, 0x20, 0x30, 0x7b, 0xcc, 0xc7);

//
// IDebugDataSpaces4: d98ada1f-29e9-4ef5-a6c0-e53349883212
//

DEFINE_GUID_EX(IID_IDEBUG_DATASPACES, 0xd98ada1f, 0x29e9, 0x4ef5,
               0xa6, 0xc0, 0xe5, 0x33, 0x49, 0x88, 0x32, 0x12);

//
// IDebugRegisters2: 1656afa9-19c6-4e3a-97e7-5dc9160cf9c4
//

DEFINE_GUID_EX(IID_IDEBUG_REGISTERS, 0x1656afa9, 0x19c6, 0x4e3a,
               0x97, 0xe7, 0x5d, 0xc9, 0x16, 0x0c, 0xf9, 0xc4);

//
// IID_IDebugSystemObjects4: 489468e6-7d0f-4af5-87ab-25207454d553
//

DEFINE_GUID_EX(IID_IDEBUG_SYSTEMOBJECTS, 0x489468e6, 0x7d0f, 0x4af5,
               0x87, 0xab, 0x25, 0x20, 0x74, 0x54, 0xd5, 0x53);

//
// IID_IDebugClient7: 13586be3-542e-481e-b1f2-8497ba74f9a9
//

DEFINE_GUID_EX(IID_IDEBUG_CLIENT, 0x13586be3, 0x542e, 0x481e,
               0xb1, 0xf2, 0x84, 0x97, 0xba, 0x74, 0xf9, 0xa9);

//
// IID_IDebugControl7: b86fb3b1-80d4-475b-aea3-cf06539cf63a
//

DEFINE_GUID_EX(IID_IDEBUG_CONTROL, 0xb86fb3b1, 0x80d4, 0x475b,
               0xae, 0xa3, 0xcf, 0x06, 0x53, 0x9c, 0xf6, 0x3a);

//
// IID_IDebugEventCallbacksWide: 0690e046-9c23-45ac-a04f-987ac29ad0d3
//

DEFINE_GUID_EX(IID_IDEBUG_EVENT_CALLBACKS, 0x0690e046, 0x9c23, 0x45ac,
               0xa0, 0x4f, 0x98, 0x7a, 0xc2, 0x9a, 0xd0, 0xd3);

//
// IID_IDebugOutputCallbacks: 4bf58045-d654-4c40-b0af-683090f356dc
//

DEFINE_GUID_EX(IID_IDEBUG_OUTPUT_CALLBACKS, 0x4bf58045, 0xd654, 0x4c40,
               0xb0, 0xaf, 0x68, 0x30, 0x90, 0xf3, 0x56, 0xdc);

//
// IID_IDebugOutputCallbacks2: 67721fe9-56d2-4a44-a325-2b65513ce6eb
//

DEFINE_GUID_EX(IID_IDEBUG_OUTPUT_CALLBACKS2, 0x67721fe9, 0x56d2, 0x4a44,
               0xa3, 0x25, 0x2b, 0x65, 0x51, 0x3c, 0xe6, 0xeb);

//
// IID_IDebugInputCallbacks: 9f50e42c-f136-499e-9a97-73036c94ed2d
//

DEFINE_GUID_EX(IID_IDEBUG_INPUT_CALLBACKS, 0x9f50e42c, 0xf136, 0x499e,
               0x9a, 0x97, 0x73, 0x03, 0x6c, 0x94, 0xed, 0x2d);

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
