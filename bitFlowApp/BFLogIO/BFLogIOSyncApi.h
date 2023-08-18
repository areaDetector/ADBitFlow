#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SYNC__API__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SYNC__API__H

/* FILE:        BFLogIOSyncApi.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Public API for BFLogIO Sync servers.
 */

#include "BFLogIODef.h"

#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)

// Acquire a new BFLogIO sync handle.
BFLOGIO_API BFLogIOAcquireSync (BFLogIOSync *pSync);

// Release a BFLogIO sync handle.
BFLOGIO_API BFLogIOReleaseSync (BFLogIOSync hSync);

// Determine if the sync is valid, such that source objects may connect to it. Return BFLOGIO_OK
// if the sync is good, BFLOGIO_ERR_SYNC_IS_BAD if not.
BFLOGIO_API BFLogIOSyncIsGood (BFLogIOSync hSync);

// Set the name of the sync server. ".", for the local machine.
BFLOGIO_API BFLogIOSetSyncServer (BFLogIOSync hSync, BFLOGIO_CCSTR serverName);

// Retrieve the next BFLogIO message from the sync handle. On BFLOGIO_OK, Caller is responsible
// for freeing the returned message using BFLogIOFreeMessage.
BFLOGIO_API BFLogIOWaitNextMessage (BFLogIOSync hSync, BFLogIOMessage *pMsg, BFLOGIO_UINT32 ms_timeout);

// Cancel a call to BFLogIOWaitNextMessage. Cancel state will remain in effect until
// BFLogIOWaitNextMessage returns BFLOGIO_ERR_CANCELED, or the cancel is disabled.
BFLOGIO_API BFLogIOCancelWaitNextMessage (BFLogIOSync hSync, BFLOGIO_BOOL enabled);

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)

#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SYNC__API__H
