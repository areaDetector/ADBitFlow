/* FILE:        BFLogIOSyncApi.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Public API for BFLogIO Sync servers.
 */

#include "BFLogIOSyncApi.h"
#include "BFLogIOSyncClass.h"

#include <stdexcept>
#include <memory>

// Acquire a new BFLogIO sync handle.
BFLOGIO_API BFLogIOAcquireSync (BFLogIOSync *pSync)
{
    if (!pSync)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        std::unique_ptr<BFLogIOSyncClass> syncClass (new BFLogIOSyncClass);
        *pSync = syncClass->c_sync();
        syncClass.release();
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

// Release a BFLogIO sync handle.
BFLOGIO_API BFLogIOReleaseSync (BFLogIOSync hSync)
{
    if (!hSync)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        delete BFLogIOSyncClass::from(hSync);
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Determine if the sync is valid and connected. Return BFLOGIO_OK if the sync is good,
// BFLOGIO_ERR_SYNC_IS_BAD if not.
BFLOGIO_API BFLogIOSyncIsGood (BFLogIOSync hSync)
{
    if (!hSync)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        if (!BFLogIOSyncClass::from(hSync)->isGood())
            return BFLOGIO_ERR_SYNC_IS_BAD;
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Set the name of the sync server. ".", for the local machine.
BFLOGIO_API BFLogIOSetSyncServer (BFLogIOSync hSync, BFLOGIO_CCSTR serverName)
{
    if (!hSync)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!serverName)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        if (!BFLogIOSyncClass::from(hSync)->setServerName(serverName))
            return BFLOGIO_ERR_SYNC_IS_BAD;
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Retrieve the next BFLogIO message from the sync handle. On BFLOGIO_OK, Caller is responsible
// for freeing the returned message using BFLogIOReleaseMessage.
BFLOGIO_API BFLogIOWaitNextMessage (BFLogIOSync hSync, BFLogIOMessage *pMsg, BFLOGIO_UINT32 ms_timeout)
{
    if (!hSync)
        return BFLOGIO_ERR_BAD_HANDLE;
    if (!pMsg)
        return BFLOGIO_ERR_NULL_ARGUMENT;

    try
    {
        BFLogIOMessageClass msg;
        switch (BFLogIOSyncClass::from(hSync)->waitNextMessage(msg, ms_timeout))
        {
        case BFLogIOSyncClass::Wait::OK:
            break;
        case BFLogIOSyncClass::Wait::SyncIsBad:
            return BFLOGIO_ERR_SYNC_IS_BAD;
        case BFLogIOSyncClass::Wait::SourceConnectFailed:
            return BFLOGIO_ERR_SOURCE_CONNECT_FAILED;
        case BFLogIOSyncClass::Wait::Timeout:
            return BFLOGIO_ERR_TIMEOUT;
        case BFLogIOSyncClass::Wait::Canceled:
            return BFLOGIO_ERR_CANCELED;
        case BFLogIOSyncClass::Wait::SourceWaitFailed:
            return BFLOGIO_ERR_SOURCE_WAIT_FAILED;
        case BFLogIOSyncClass::Wait::SourceDisconnected:
            return BFLOGIO_ERR_SOURCE_DISCONNECTED;
        case BFLogIOSyncClass::Wait::StartReadFailed:
            return BFLOGIO_ERR_START_READ_FAILED;
        case BFLogIOSyncClass::Wait::ReadWaitFailed:
            return BFLOGIO_ERR_READ_WAIT_FAILED;
        case BFLogIOSyncClass::Wait::ReadFailed:
            return BFLOGIO_ERR_READ_FAILED;
        case BFLogIOSyncClass::Wait::MessageDeformed:
            return BFLOGIO_ERR_MESSAGE_DEFORMED;
        default:
            return BFLOGIO_ERR_UNKNOWN;
        }

        std::unique_ptr<BFLogIOMessageClass> msgCpy (new BFLogIOMessageClass(std::move(msg)));
        *pMsg = msgCpy->c_msg();
        msgCpy.release();
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}

// Cancel a call to BFLogIOWaitNextMessage. Cancel state will remain in effect until
// BFLogIOWaitNextMessage returns BFLOGIO_ERR_CANCELED, or the cancel is disabled.
BFLOGIO_API BFLogIOCancelWaitNextMessage (BFLogIOSync hSync, BFLOGIO_BOOL enabled)
{
    if (!hSync)
        return BFLOGIO_ERR_BAD_HANDLE;

    try
    {
        if (!BFLogIOSyncClass::from(hSync)->cancelWaitNextMessage(!!enabled))
            return BFLOGIO_ERR_CANCEL_STATE_CHANGE;
    }
    catch (...)
    {
        return BFLOGIO_ERR_UNKNOWN;
    }

    return BFLOGIO_OK;
}
