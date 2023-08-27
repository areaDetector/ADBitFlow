#ifndef INCLUDED__BIT_FLOW__B_F_G_T_L__B_F_G_T_L__EXTENSIONS__API__H
#define INCLUDED__BIT_FLOW__B_F_G_T_L__B_F_G_T_L__EXTENSIONS__API__H

/* FILE:        BFGTLExtensionsApi.h
 * DATE:        4/5/2018
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2018, BitFlow, Inc.
 * DESCRIPTION: Non-standard BitFlow extensions to the GenTL interface.
 */

#if defined(_WIN32)

#   include <BFDef.h>
#   include <BFResolveGenTL.h>

typedef BFU32 BFGTLExtUInt32_t;
typedef Bd BFGTLExtBd_t;

#elif defined(__GNUC__)

#   include <BFciLib.h>
#   include "BFResolveGenTL.h"

typedef tCIU32 BFGTLExtUInt32_t;
typedef tCIp BFGTLExtBd_t;

#else
#   error Native library support not implemented on this platform.
#endif

#if defined(__cplusplus)

extern "C" {

#   if GC_GENTL_HEADER_VERSION >= GC_GENTL_HEADER_VERSION_CODE(1,5,0)
    using namespace GenTL;
#   else
    using namespace GenICam::Client;
#   endif

#endif

typedef struct _BFGTLLibHandle *BFGTLLibHandle;

// Acquire a handle representing the BFGTL global. BFGTL is initialized internally, if necessary.
GC_API BFGTLExtLibAcquire (BFGTLLibHandle *pLib);

// Release the previously locked BFGTL handle. Closes BFGTL, if necessary.
GC_API BFGTLExtLibRelease (BFGTLLibHandle hLib);

// Open a TLDevice handle directly from a BitFlow board handle.
GC_API BFGTLExtOpenDevice (BFGTLExtUInt32_t boardNumber, BFGTLExtBd_t hBoardHandle, DEV_HANDLE *phDevice);
    
#if defined(__cplusplus)
} // extern "C"
#endif

#endif // INCLUDED__BIT_FLOW__B_F_G_T_L__B_F_G_T_L__EXTENSIONS__API__H
