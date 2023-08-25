//
// Creation:    BFUniversalDef.h
// Created:     August 7, 2019
// Creator:     Jeremy Greene
//
// Copyright (C) 1993-2019 by BitFlow, Inc.  All Rights Reserved.
//
// BitFlow Public Definitions for cross platform applications.
//
// History:
//
// 08/07/19     jtg     Created file.
//

#ifndef BITFLOW__B_F_UNIVERSAL__DEF__H
#define BITFLOW__B_F_UNIVERSAL__DEF__H

#if defined(_WIN32)
#   include <BFVersion.h>
#endif

#if (defined(_WIN32) && (!defined(__BFDEF__) || (BF_SDK_VERSION >= 0x6500))) || (defined(__GNUC__))

// Public constants used by BFLogIO.
typedef enum _BFLogIOMsgTypeEnum
{
    BFLogIOUnknownType = 0,

    BFLogIODebugType = (1 << 0),
    BFLogIONotificationType = (1 << 1),
    BFLogIOWarningType = (1 << 2),
    BFLogIOErrorType = (1 << 3),
    BFLogIOFatalErrorType = (1 << 4),

    BFLogIOTypeMask = BFLogIODebugType|BFLogIONotificationType|BFLogIOWarningType|BFLogIOErrorType|BFLogIOFatalErrorType
} BFLogIOMsgType;

typedef enum _BFLogIOMsgSourceEnum
{
    BFLogIOUnknownSrc = 0,

    BFLogIOUserSrc = 1,
    BFLogIODriverSrc = 2,
    BFLogIOCLSerialSrc = 3,
    BFLogIOCxpRegSrc = 4,
	BFLogIOBufInSrc = 5,
    BFLogIOGenTLSrc = 6
} BFLogIOMsgSource;

#endif // __BFDEF__

#endif // BITFLOW__B_F_UNIVERSAL__DEF__H
