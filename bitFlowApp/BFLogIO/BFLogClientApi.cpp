/* FILE:        BFLogClientApi.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Simplified API to to send BFLogIO interprocess messages from a
 *              client application.
 */

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <Psapi.h>
#elif defined(__GNUC__)
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <unistd.h>
#   include <vector>
#else
#   error Platform implementation missing.
#endif

#include "BFLogClientApi.h"

#include "BFLogIOSourceApi.h"
#include "BFLogIOMessageApi.h"

// Attempt to connect to the local BFLogIO Sync server, and push a message with
// the specified parameters. All necessary resources are allocated and deallocated
// internally. In addition to the Client error codes, BFLOGIO_ERR_MISSING_SYNC may
// be returned.
BFLOGIO_API BFLogClientMessagePush (const BFLogIOMsgType Type, const BFLogIOMsgSource Source, BFLOGIO_CCSTR Title, BFLOGIO_CCSTR Text)
{
    BFLOGIORC rc;
    BFLogIOSource hSrc;
    BFLogIOMessage hMsg;

    // Acquire the source handle.
    rc = BFLogIOAcquireSource(&hSrc);
    if (BFLOGIO_OK != rc)
        return BFLOGIO_ERR_SOURCE_ACQUIRE_FAILED;

    // Connect the source to the sync server, if any.
    rc = BFLogIOConnect(hSrc, BFLOGIO_INFINITE_TIMEOUT);
    if (BFLOGIO_OK != rc)
    {
        BFLogIOReleaseSource(hSrc);
        if (BFLOGIO_ERR_MISSING_SYNC == rc)
            return BFLOGIO_ERR_MISSING_SYNC;
        return BFLOGIO_ERR_COULD_NOT_CONNECT;
    }

    // Create a message handle.
    rc = BFLogIOMakeMessage(&hMsg);
    if (BFLOGIO_OK != rc)
    {
        BFLogIODisconnect(hSrc);
        BFLogIOReleaseSource(hSrc);
        return BFLOGIO_ERR_MAKE_MESSAGE_FAILED;
    }

    // Set the message data.
    BFLogIOSetMsgType(hMsg, (BFLogIOMsgType)Type);
    BFLogIOSetMsgSource(hMsg, (BFLogIOMsgSource)Source);
    BFLogIOSetMsgTitle(hMsg, Title);
    BFLogIOSetMsgText(hMsg, Text);

    // Attempt to determine the client application programatically. If possible,
    // set this as the message application.
#if defined(_WIN32)

    BFCHAR procName [MAX_PATH] = {0};
    if (GetModuleFileNameExA(GetCurrentProcess(), NULL, procName, sizeof(procName)))
        BFLogIOSetMsgApp(hMsg, procName);

#elif defined(__GNUC__)

    // /proc/self/exe is a pseudo-link to the current executable file.
    const char procSelfPath[] = "/proc/self/exe";

    // Call readlink recursively, until we get the full exe file path. lstat
    // cannot be used to determine the path length on some platforms, because
    // /proc/self/exe is not a true link.
    std::vector<char> appName (128);
    do
    {
        const ssize_t pathLen = readlink(procSelfPath, appName.data(), appName.size());
        if (0 > pathLen || (ssize_t)appName.size() <= pathLen)
            appName.resize(appName.size() << 1);
        else
        {
            // !!! readlink does not null terminate !!!
            appName[pathLen] = '\0';
            BFLogIOSetMsgApp(hMsg, appName.data());
            break;
        }
    }
    while (appName.size() <= 65536);

#else
#   error Platform implementation missing.
#endif
    
    // Push the message to the waiting sync server.
    rc = BFLogIOPushMessage(hSrc, hMsg);

    // Cleanup.
    BFLogIOFreeMessage(hMsg);
    BFLogIODisconnect(hSrc);
    BFLogIOReleaseSource(hSrc);

    if (BFLOGIO_OK != rc)
        return BFLOGIO_ERR_MESSAGE_PUSH_FAILED;

    return BFLOGIO_OK;
}
