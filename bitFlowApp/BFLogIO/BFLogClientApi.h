#ifndef INCLUDED__BIT_FLOW__B_F_LOG__CLIENT__API__H
#define INCLUDED__BIT_FLOW__B_F_LOG__CLIENT__API__H

/* FILE:        BFLogClientApi.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Simplified API to to send BFLogIO interprocess messages from a
 *              client application.
 */

#include "BFLogIODef.h"

#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)

// Attempt to connect to the local BFLogIO Sync server, and push a message with
// the specified parameters. All necessary resources are allocated and deallocated
// internally. In addition to the Client error codes, BFLOGIO_ERR_MISSING_SYNC may
// be returned.
BFLOGIO_API BFLogClientMessagePush (const BFLogIOMsgType Type, const BFLogIOMsgSource Source, BFLOGIO_CCSTR Title, BFLOGIO_CCSTR Text);

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)

#endif // INCLUDED__BIT_FLOW__B_F_LOG__CLIENT__API__H
