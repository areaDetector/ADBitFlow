#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__DEF__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__DEF__H

/* FILE:        BFLogIODef.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Common definitions for the BFLogIO library.
 */

#define BFLOGIO_API BFLOGIORC

#define BFLOGIO_PIPE_NAME "BitFlow_BFLogIO_MessagePipe"

#define BFLOGIO_STRLENMAX (1024)

#define BFLOGIO_INFINITE_TIMEOUT (~((unsigned int)0))
#define BFLOGIO_DEFAULT_TIMEOUT (0)

#define BFLOGIO_STRINGIZE(S) #S
#define BFLOGIO_TOSTRING(S) BFLOGIO_STRINGIZE(S)

#if defined(_WIN32)
#   include <BFType.h>
#   include <BFUniversalDef.h>
#elif defined(__GNUC__)
#   include <stdint.h>
#   include "BFUniversalDef.h"
#else
#   error Platform implementation missing.
#endif

#include <time.h>

#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)

#if defined(_WIN32)
    typedef BFBOOL          BFLOGIO_BOOL;
    typedef BFCHAR          *BFLOGIO_CSTR;
    typedef const BFCHAR    *BFLOGIO_CCSTR;
    typedef BFSIZET         BFLOGIO_SIZET;
    typedef BFU32           BFLOGIO_UINT32;
    typedef BFU64           BFLOGIO_UINT64;
    typedef unsigned int    BFLOGIO_TIMEOUT_UINT;
#elif defined(__GNUC__)
    typedef uint8_t         BFLOGIO_BOOL;
    typedef char            *BFLOGIO_CSTR;
    typedef const char      *BFLOGIO_CCSTR;
    typedef size_t          BFLOGIO_SIZET;
    typedef uint32_t        BFLOGIO_UINT32;
    typedef uint64_t        BFLOGIO_UINT64;
    typedef BFLOGIO_UINT32  BFLOGIO_TIMEOUT_UINT;
#else
#   error Platform implementation missing.
#endif

typedef enum BFLOGIORCEnum
{
    BFLOGIO_OK = 0,

    BFLOGIO_ERR_UNKNOWN = -10000,
    BFLOGIO_ERR_NULL_ARGUMENT = -10001,
    BFLOGIO_ERR_BAD_HANDLE = -10002,
    BFLOGIO_ERR_BAD_ALLOC = -10003,
    BFLOGIO_ERR_TIMEOUT = -10004,
    BFLOGIO_ERR_CANCELED = -10005,
    BFLOGIO_ERR_UNKNOWN_EXCEPTION = -10006,

    // Sync errors.
    BFLOGIO_ERR_SYNC_IS_BAD = -11000,
    BFLOGIO_ERR_SOURCE_CONNECT_FAILED = -11001,
    BFLOGIO_ERR_SOURCE_WAIT_FAILED = -11002,
    BFLOGIO_ERR_SOURCE_DISCONNECTED = -11003,
    BFLOGIO_ERR_START_READ_FAILED = -11004,
    BFLOGIO_ERR_READ_WAIT_FAILED = -11005,
    BFLOGIO_ERR_READ_FAILED = -11006,
    BFLOGIO_ERR_MESSAGE_DEFORMED = -11007,
    BFLOGIO_ERR_CANCEL_STATE_CHANGE = -11008,

    // Source errors.
    BFLOGIO_ERR_SOURCE_CONNECT_BLOCKED = -12000,
    BFLOGIO_ERR_ALREADY_CONNECTED = -12001,
    BFLOGIO_ERR_NOT_CONNECTED = -12002,
    BFLOGIO_ERR_MISSING_SYNC = -12003,
    BFLOGIO_ERR_SYNC_WAIT_FAILED = -12004,
    BFLOGIO_ERR_WRITE_FAILED = -12005,

    // High level Client errors.
    BFLOGIO_ERR_SOURCE_ACQUIRE_FAILED = -13000,
    BFLOGIO_ERR_COULD_NOT_CONNECT = -13001,
    BFLOGIO_ERR_MAKE_MESSAGE_FAILED = -13002,
    BFLOGIO_ERR_MESSAGE_PUSH_FAILED = -13003

} BFLOGIORC;

typedef struct _BFLogIOSync { void *p; } *BFLogIOSync;
typedef struct _BFLogIOSource { void *p; } *BFLogIOSource;
typedef struct _BFLogIOMessage { void *p; } *BFLogIOMessage;

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)

#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__DEF__H
