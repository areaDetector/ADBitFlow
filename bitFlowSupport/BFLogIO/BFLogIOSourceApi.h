#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SOURCE__API__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SOURCE__API__H

/* FILE:        BFLogIOSourceApi.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Public API for the BFLogIO client Source. For simple cases,
 *              consider using the high-level BFLogClientApi instead.
 */

#include "BFLogIODef.h"

#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)

// Set or release the global source connections block for this process.
// pWasBlocked is optional (i.e., may be NULL).
BFLOGIO_API BFLogIOBlockAllSources (BFLOGIO_BOOL setBlocked, BFLOGIO_BOOL *const pWasBlocked);

// Determine if source connections are currently blocked in this process.
BFLOGIO_API BFLogIOAllSourcesBlocked (BFLOGIO_BOOL *const pBlocked);

// Acquire a new BFLogIO source handle.
BFLOGIO_API BFLogIOAcquireSource (BFLogIOSource *pSource);

// Release a BFLogIO source handle.
BFLOGIO_API BFLogIOReleaseSource (BFLogIOSource hSource);

// Connect to the BFLogIO sync server.
BFLOGIO_API BFLogIOConnect (BFLogIOSource hSource, BFLOGIO_UINT32 ms_timeout);

// Disconnect from the BFLogIO sync server.
BFLOGIO_API BFLogIODisconnect (BFLogIOSource hSource);

// Push a message to the BFLogIO sync. The source must first be connected, and
// should be disconencted after this use. This function is non-destructive, and
// the caller is responsible for freeing provided message memory.
BFLOGIO_API BFLogIOPushMessage (BFLogIOSource hSource, BFLogIOMessage hMsg);

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)


#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SOURCE__API__H
