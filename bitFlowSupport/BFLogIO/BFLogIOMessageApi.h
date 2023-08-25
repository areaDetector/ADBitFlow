#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__MESSAGE__API__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__MESSAGE__API__H

/* FILE:        BFLogIOMessageApi.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Public API for BFLogIO messages.
 */

#include "BFLogIODef.h"

#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)

// Create a new message.
BFLOGIO_API BFLogIOMakeMessage (BFLogIOMessage *pMsg);

// Free a message.
BFLOGIO_API BFLogIOFreeMessage (BFLogIOMessage hMsg);

//// Message field modifiers.

// Set the message type.
BFLOGIO_API BFLogIOSetMsgType (BFLogIOMessage hMsg, const BFLogIOMsgType type);

// Set the message source.
BFLOGIO_API BFLogIOSetMsgSource (BFLogIOMessage hMsg, const BFLogIOMsgSource source);

// Set the message title.
BFLOGIO_API BFLogIOSetMsgTitle (BFLogIOMessage hMsg, BFLOGIO_CCSTR title);

// Set the message text.
BFLOGIO_API BFLogIOSetMsgText (BFLogIOMessage hMsg, BFLOGIO_CCSTR text);

// Set the message application name/path.
BFLOGIO_API BFLogIOSetMsgApp (BFLogIOMessage hMsg, BFLOGIO_CCSTR application);

//// Message field accessors.

// Retrieve the message timestamp sent. This will be zero valued, if the message has not been sent.
BFLOGIO_API BFLogIOGetMsgTimestampSent (BFLogIOMessage hMsg, time_t *pTime);

// Retrieve the message type.
BFLOGIO_API BFLogIOGetMsgType (BFLogIOMessage hMsg, BFLogIOMsgType *pType);

// Retrieve the message source.
BFLOGIO_API BFLogIOGetMsgSource (BFLogIOMessage hMsg, BFLogIOMsgSource *pSource);

// Retrieve the message title string. pBufSize is the buffer size as input, complete
// message size (including null terminator) as output. Buffer handle may be nullptr.
BFLOGIO_API BFLogIOGetMsgTitle (BFLogIOMessage hMsg, BFLOGIO_CSTR strBuf, BFLOGIO_SIZET *const pBufSize);

// Retrieve the message text. pBufSize is the buffer size as input, complete
// message size (including null terminator) as output. Buffer handle may be nullptr.
BFLOGIO_API BFLogIOGetMsgText (BFLogIOMessage hMsg, BFLOGIO_CSTR strBuf, BFLOGIO_SIZET *const pBufSize);

// Retrieve the message application name/path. pBufSize is the buffer size as input,
// complete message size (including null terminator) as output. Buffer handle may be
// nullptr.
BFLOGIO_API BFLogIOGetMsgApp (BFLogIOMessage hMsg, BFLOGIO_CSTR strBuf, BFLOGIO_SIZET *const pBufSize);

// Retrieve the Process ID for the process that created this message.
BFLOGIO_API BFLogIOGetMsgPID (BFLogIOMessage hMsg, BFLOGIO_UINT64 *const pPID);

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)

#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__MESSAGE__API__H
