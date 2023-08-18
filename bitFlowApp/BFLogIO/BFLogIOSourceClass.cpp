/* FILE:        BFLogIOSourceClass.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Class implementing a BFLogIO Source object.
 */

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#   define BFLOGIO_INVALID_HANDLE (INVALID_HANDLE_VALUE)
#elif defined(__GNUC__)
#   include <sys/types.h>
#   include <sys/un.h>
#   include <sys/fcntl.h>
#   include <poll.h>
#   include <unistd.h>
#   define BFLOGIO_INVALID_HANDLE (-1)
#else
#   error Platform implementation missing.
#endif

#include "BFLogIOSourceClass.h"
#include "BFLogIOSyncClass.h"
#include "TimeoutHelper.h"

#include <atomic>

struct BFLogIOSourceClass::PrivateData
{
#if defined(_WIN32)
    HANDLE m_ioPipe;
#elif defined(__GNUC__)
    int m_ioPipe;
#else
#   error Platform implementation missing.
#endif

    PrivateData (void)
        : m_ioPipe (BFLOGIO_INVALID_HANDLE)
    {
    }
};

// Global Source enable/disable. Return previous state.
static std::atomic<bool> s_AllSourcesBlocked (false);

bool BFLogIOSourceClass::BlockAllSources (const bool block)
{
    return s_AllSourcesBlocked.exchange(block);
}
bool BFLogIOSourceClass::AllSourcesBlocked (void)
{
    return s_AllSourcesBlocked.load();
}

// Constructors/Destructors.
BFLogIOSourceClass::BFLogIOSourceClass (void)
    : m_pd (new PrivateData)
{
}
BFLogIOSourceClass::~BFLogIOSourceClass (void)
{
    disconnect();
    delete m_pd;
}

// Handle rangling.
BFLogIOSource BFLogIOSourceClass::c_src (void)
{
    return reinterpret_cast<BFLogIOSource>(this);
}
BFLogIOSourceClass* BFLogIOSourceClass::from (BFLogIOSource a_src)
{
    return reinterpret_cast<BFLogIOSourceClass*>(a_src);
}

// Source status.
bool BFLogIOSourceClass::isConnected (void) const
{
    return BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe;
}

// Connect to the Sync.
BFLogIOSourceClass::Connect BFLogIOSourceClass::connect (const unsigned int ms_timeout)
{
    if (s_AllSourcesBlocked.load())
        return Connect::Blocked;

    if (BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe)
        return Connect::AlreadyConnected;

    TIMEOUT_START(ms_timeout, BFLOGIO_INFINITE_TIMEOUT);

#if defined(_WIN32)

    const std::string pipeName = "\\\\" + BFLogIOSyncClass::localServerDefault() + "\\pipe\\" BFLOGIO_PIPE_NAME;

    while (true)
    {
        m_pd->m_ioPipe = CreateFileA(pipeName.c_str()
            , GENERIC_READ | GENERIC_WRITE
            , 0
            , NULL
            , OPEN_EXISTING
            , 0
            , NULL);

        if (BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe)
            break;

        if (ERROR_PIPE_BUSY != GetLastError())
            return Connect::MissingSync;

        if (ms_timeout == 0)
            return Connect::Timeout;

        if (!WaitNamedPipeA(pipeName.c_str(), TIMEOUT_REMAINING(NMPWAIT_WAIT_FOREVER)))
        {
            if (ERROR_SEM_TIMEOUT == GetLastError())
                return Connect::Timeout;
            return Connect::SyncWaitFailed;
        }
    }

#elif defined(__GNUC__)

    const std::string pipePath = BFLogIOSyncClass::localServerDefault() + "/" BFLOGIO_PIPE_NAME;

    m_pd->m_ioPipe = open(pipePath.c_str(), O_WRONLY | O_NONBLOCK);
    if (BFLOGIO_INVALID_HANDLE == m_pd->m_ioPipe)
        return Connect::MissingSync;

    pollfd pfd;

    while (true)
    {
        memset(&pfd, 0, sizeof(pfd));
        pfd.fd = m_pd->m_ioPipe;
        pfd.events = POLLOUT;

        const int prc = poll(&pfd, 1, TIMEOUT_REMAINING(-1));
        if (0 == prc)
        {
            close(m_pd->m_ioPipe);
            m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;
            return Connect::Timeout;
        }
        else if (0 > prc)
        {
            close(m_pd->m_ioPipe);
            m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;
            return Connect::SyncWaitFailed;
        }
        else if (POLLOUT == pfd.revents)
            break;
    }

#else
#   error Platform implementation missing.
#endif

    return Connect::OK;
}

// Disconnect from the Sync.
BFLogIOSourceClass::Disconnect BFLogIOSourceClass::disconnect (void)
{
    if (BFLOGIO_INVALID_HANDLE == m_pd->m_ioPipe)
        return Disconnect::NotConnected;

#if defined(_WIN32)
    CloseHandle(m_pd->m_ioPipe);
#elif defined(__GNUC__)
    close(m_pd->m_ioPipe);
#else
#   error Platform implementation missing.
#endif

    m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;

    return Disconnect::OK;
}

// Push a message to all active BFLogIO syncs on this system.
BFLogIOSourceClass::Push BFLogIOSourceClass::push (BFLogIOMessageClass const& a_msg)
{
    if (BFLOGIO_INVALID_HANDLE == m_pd->m_ioPipe)
        return Push::NotConnected;

    const auto msgByteArray = a_msg.toByteArray();

#if defined(_WIN32)

    DWORD bytesWritten;
    if (!WriteFile(m_pd->m_ioPipe, msgByteArray.data(), (DWORD)msgByteArray.size(), &bytesWritten, NULL))
        return Push::WriteFailed;

#elif defined(__GNUC__)

    if (write(m_pd->m_ioPipe, msgByteArray.data(), msgByteArray.size()) == -1)
        return Push::WriteFailed;

#else
#   error Platform implementation missing.
#endif

    return Push::OK;
}
