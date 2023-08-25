/* FILE:        BFLogIOSourceApi.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Public API for the BFLogIO client Source. For simple cases,
 *              consider using the high-level BFLogClientApi instead.
 */

#include "BFLogIOSourceApi.h"
#include "BFLogIOSourceClass.h"
#include "BFLogIOMessageClass.h"

#include <stdexcept>
#include <memory>

// Set or release the global source connections block for this process.
// pWasBlocked is optional (i.e., may be NULL).
BFLOGIO_API BFLogIOBlockAllSources (BFLOGIO_BOOL setBlocked, BFLOGIO_BOOL *const pWasBlocked)
{
    try
    {
        const bool wasBlocked = BFLogIOSourceClass::BlockAllSources(!!setBlocked);
        if (pWasBlocked)
            *pWasBlocked = wasBlocked;
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Determine if source connections are currently blocked in this process.
BFLOGIO_API BFLogIOAllSourcesBlocked (BFLOGIO_BOOL *const pBlocked)
{
    if (!pBlocked)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        *pBlocked = BFLogIOSourceClass::AllSourcesBlocked();
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Acquire a new BFLogIO source handle.
BFLOGIO_API BFLogIOAcquireSource (BFLogIOSource *pSource)
{
    if (!pSource)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        std::unique_ptr<BFLogIOSourceClass> sourceClass (new BFLogIOSourceClass);
        *pSource = sourceClass->c_src();
        sourceClass.release();
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

// Release a BFLogIO source handle.
BFLOGIO_API BFLogIOReleaseSource (BFLogIOSource hSource)
{
    if (!hSource)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        delete BFLogIOSourceClass::from(hSource);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Connect to the BFLogIO sync server.
BFLOGIO_API BFLogIOConnect (BFLogIOSource hSource, BFLOGIO_UINT32 ms_timeout)
{
    if (!hSource)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        switch (BFLogIOSourceClass::from(hSource)->connect(ms_timeout))
        {
        case BFLogIOSourceClass::Connect::OK:
            break;
        case BFLogIOSourceClass::Connect::Blocked:
            return BFLOGIO_ERR_SOURCE_CONNECT_BLOCKED;
        case BFLogIOSourceClass::Connect::AlreadyConnected:
            return BFLOGIO_ERR_ALREADY_CONNECTED;
        case BFLogIOSourceClass::Connect::MissingSync:
            return BFLOGIO_ERR_MISSING_SYNC;
        case BFLogIOSourceClass::Connect::Timeout:
            return BFLOGIO_ERR_TIMEOUT;
        case BFLogIOSourceClass::Connect::SyncWaitFailed:
            return BFLOGIO_ERR_SYNC_WAIT_FAILED;
        default:
            return BFLOGIO_ERR_UNKNOWN;
        }
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN_EXCEPTION;
    }

    return BFLOGIO_OK;
}

// Disconnect from the BFLogIO sync server.
BFLOGIO_API BFLogIODisconnect (BFLogIOSource hSource)
{
    if (!hSource)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        switch (BFLogIOSourceClass::from(hSource)->disconnect())
        {
        case BFLogIOSourceClass::Disconnect::OK:
            break;
        case BFLogIOSourceClass::Disconnect::NotConnected:
            return BFLOGIO_ERR_NOT_CONNECTED;
        default:
            return BFLOGIO_ERR_UNKNOWN;
        }
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN_EXCEPTION;
    }

    return BFLOGIO_OK;
}

// Push a message to the BFLogIO sync. The source must first be connected, and
// should be disconencted after this use. This function is non-destructive, and
// the caller is responsible for freeing provided message memory.
BFLOGIO_API BFLogIOPushMessage (BFLogIOSource hSource, BFLogIOMessage hMsg)
{
    if (!hSource)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!hMsg)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        switch (BFLogIOSourceClass::from(hSource)->push(*BFLogIOMessageClass::from(hMsg)))
        {
        case BFLogIOSourceClass::Push::OK:
            break;
        case BFLogIOSourceClass::Push::NotConnected:
            return BFLOGIO_ERR_NOT_CONNECTED;
        case BFLogIOSourceClass::Push::WriteFailed:
            return BFLOGIO_ERR_WRITE_FAILED;
        default:
            return BFLOGIO_ERR_UNKNOWN;
        }
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN_EXCEPTION;
    }

    return BFLOGIO_OK;
}
