/* FILE:        BFLogIOMessageApi.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Public API for BFLogIO messages.
 */

#define NOMINMAX

#include "BFLogIOMessageApi.h"
#include "BFLogIOMessageClass.h"

#include <stdexcept>
#include <cstring>
#include <memory>
#include <algorithm>

// Create a new message.
BFLOGIO_API BFLogIOMakeMessage (BFLogIOMessage *pMsg)
{
    if (!pMsg)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        std::unique_ptr<BFLogIOMessageClass> message (new BFLogIOMessageClass);
        *pMsg = message->c_msg();
        message.release();
    }
    catch (std::bad_alloc const&)
    {
        return BFLOGIO_ERR_BAD_ALLOC;
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Free a message.
BFLOGIO_API BFLogIOFreeMessage (BFLogIOMessage hMsg)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        delete BFLogIOMessageClass::from(hMsg);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

//// Message field modifiers.

// Set the message type.
BFLOGIO_API BFLogIOSetMsgType (BFLogIOMessage hMsg, const BFLogIOMsgType type)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        BFLogIOMessageClass::from(hMsg)->setType(type);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Set the message source.
BFLOGIO_API BFLogIOSetMsgSource (BFLogIOMessage hMsg, const BFLogIOMsgSource source)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        BFLogIOMessageClass::from(hMsg)->setSource(source);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Set the message title.
BFLOGIO_API BFLogIOSetMsgTitle (BFLogIOMessage hMsg, BFLOGIO_CCSTR title)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!title)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        BFLogIOMessageClass::from(hMsg)->setTitle(title);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Set the message text.
BFLOGIO_API BFLogIOSetMsgText (BFLogIOMessage hMsg, BFLOGIO_CCSTR text)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!text)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        BFLogIOMessageClass::from(hMsg)->setText(text);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Set the message application name/path.
BFLOGIO_API BFLogIOSetMsgApp (BFLogIOMessage hMsg, BFLOGIO_CCSTR application)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!application)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        BFLogIOMessageClass::from(hMsg)->setApplication(application);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

//// Message field accessors.

// Retrieve the message timestamp sent. This will be zero valued, if the message has not been sent.
BFLOGIO_API BFLogIOGetMsgTimestampSent (BFLogIOMessage hMsg, time_t *pTime)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pTime)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        *pTime = BFLogIOMessageClass::from(hMsg)->timestampSent();
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the message type.
BFLOGIO_API BFLogIOGetMsgType (BFLogIOMessage hMsg, BFLogIOMsgType *pType)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pType)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        *pType = BFLogIOMessageClass::from(hMsg)->type();
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the message source.
BFLOGIO_API BFLogIOGetMsgSource (BFLogIOMessage hMsg, BFLogIOMsgSource *pSource)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pSource)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        *pSource = BFLogIOMessageClass::from(hMsg)->source();
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the message title string. pBufSize is the buffer size as input, complete
// message size (including null terminator) as output. Buffer handle may be nullptr.
BFLOGIO_API BFLogIOGetMsgTitle (BFLogIOMessage hMsg, BFLOGIO_CSTR strBuf, BFLOGIO_SIZET *const pBufSize)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pBufSize)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    const size_t bufSize = *pBufSize;

    try
    {
        const auto str = BFLogIOMessageClass::from(hMsg)->title();
        *pBufSize = str.length() + 1;

        if (strBuf && bufSize > 0)
        {
            memcpy(strBuf, str.c_str(), std::min(bufSize, *pBufSize));
            strBuf[bufSize - 1] = 0;
        }
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the message text. pBufSize is the buffer size as input, complete
// message size (including null terminator) as output. Buffer handle may be nullptr.
BFLOGIO_API BFLogIOGetMsgText (BFLogIOMessage hMsg, BFLOGIO_CSTR strBuf, BFLOGIO_SIZET *const pBufSize)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pBufSize)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    const size_t bufSize = *pBufSize;

    try
    {
        const auto str = BFLogIOMessageClass::from(hMsg)->text();
        *pBufSize = str.length() + 1;

        if (strBuf && bufSize > 0)
        {
            memcpy(strBuf, str.c_str(), std::min(bufSize, *pBufSize));
            strBuf[bufSize - 1] = 0;
        }
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the message application name/path. pBufSize is the buffer size as input,
// complete message size (including null terminator) as output. Buffer handle may be
// nullptr.
BFLOGIO_API BFLogIOGetMsgApp (BFLogIOMessage hMsg, BFLOGIO_CSTR strBuf, BFLOGIO_SIZET *const pBufSize)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pBufSize)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    const size_t bufSize = *pBufSize;

    try
    {
        const auto str = BFLogIOMessageClass::from(hMsg)->application();
        *pBufSize = str.length() + 1;

        if (strBuf && bufSize > 0)
        {
            memcpy(strBuf, str.c_str(), std::min(bufSize, *pBufSize));
            strBuf[bufSize - 1] = 0;
        }
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the Process ID for the process that created this message.
BFLOGIO_API BFLogIOGetMsgPID (BFLogIOMessage hMsg, BFLOGIO_UINT64 *const pPID)
{
    if (!hMsg)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pPID)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        *pPID = BFLogIOMessageClass::from(hMsg)->pid();
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}
