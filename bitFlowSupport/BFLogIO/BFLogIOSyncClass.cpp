/* FILE:        BFLogIOSyncClass.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Implementation class of a BFLogIO Sync server.
 */

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#   define BFLOGIO_INVALID_HANDLE (INVALID_HANDLE_VALUE)
#elif defined(__GNUC__)
#   include <time.h>
#   include <poll.h>
#   include <unistd.h>
#   include <limits.h>
#   include <paths.h>
#   include <sys/types.h>
#   include <sys/un.h>
#   include <sys/fcntl.h>
#   include <sys/stat.h>
#   define BFLOGIO_INVALID_HANDLE (-1)
#else
#   error Platform implementation missing.
#endif

#include "BFLogIOSyncClass.h"
#include "BFLogIOMessageClass.h"

#include "TimeoutHelper.h"

#include <iostream>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>

struct BFLogIOSyncClass::PrivateData
{
    enum CancelEnum : int
    {
        CancelOff,
        CancelSet,
        CancelNow
    };

    std::string m_server;

#if defined(_WIN32)

    HANDLE m_ioPipe;
    HANDLE m_hOverlappedEvent;
    HANDLE m_hCancelEvent;

#elif defined(__GNUC__)

    static const struct timespec nsleep1ms[1];
    static const struct timespec nsleep10ms[1];

    int m_lockFile;
    int m_ioPipe;
    std::string m_lockName;
    std::string m_ioPipeName;

    int m_cancelPipe[2];

#else
#   error Platform implementation missing.
#endif

    std::atomic<int> m_doCancel;
    std::vector<unsigned char> m_overflowArray;

#if defined(_WIN32)

    PrivateData (void)
        : m_ioPipe (BFLOGIO_INVALID_HANDLE)
        , m_hOverlappedEvent (NULL)
        , m_hCancelEvent (NULL)
        , m_doCancel (CancelOff)
    {
    }

#elif defined(__GNUC__)

    PrivateData (void)
        : m_lockFile (BFLOGIO_INVALID_HANDLE)
        , m_ioPipe (BFLOGIO_INVALID_HANDLE)
        , m_cancelPipe {BFLOGIO_INVALID_HANDLE, BFLOGIO_INVALID_HANDLE}
        , m_doCancel (CancelOff)
    {
        if (pipe(m_cancelPipe) == -1)
        {
            m_cancelPipe[0] = BFLOGIO_INVALID_HANDLE;
            m_cancelPipe[1] = BFLOGIO_INVALID_HANDLE;
        }
    }

    ~PrivateData (void)
    {
        for (const auto cPipe : m_cancelPipe)
        {
            if (BFLOGIO_INVALID_HANDLE != cPipe)
                close(cPipe);
        }
    }

#else
#   error Platform implementation missing.
#endif

    inline bool cancel (const bool a_doCancel)
    {
        // Attempt to get the desired state. If uncanceling, wait continuously
        // until we are not in the Set state. If canceling, succeed or fail
        // immediately.
        int expected;
        while (true)
        {
            expected = a_doCancel ? CancelOff : CancelNow;
            if (m_doCancel.compare_exchange_strong(expected, CancelSet))
                break;
            else if (CancelSet != expected)
            {
                // We're in the wrong state. Get out of here!
                return false;
            }
            else
            {
                // Try again.
#if defined(_WIN32)
                Sleep(0);
#elif defined(__GNUC__)
                nanosleep(nsleep1ms, NULL);
#else
#   error Platform implementation missing.
#endif
            }
        }

        bool success;

#if defined(_WIN32)
        if (a_doCancel)
            success = !!SetEvent(m_hCancelEvent);
        else
            success = !!ResetEvent(m_hCancelEvent);
#elif defined(__GNUC__)
        if (a_doCancel)
            success = write(m_cancelPipe[1], "C", 1) > 0;
        else
        {
            int tmp;
            success = read(m_cancelPipe[0], &tmp, sizeof(tmp)) > 0;
        }
#else
#   error Platform implementation missing.
#endif

        if (a_doCancel)
        {
            m_doCancel.store(success ? CancelNow : CancelOff);
            return success;
        }
        else
        {
            m_doCancel.store(CancelOff);
            return true;
        }
    }

    inline bool canceled (void)
    {
        return cancel(false);
    }
};

#if defined(__GNUC__)

const struct timespec BFLogIOSyncClass::PrivateData::nsleep1ms[] = {{0, 1000000L}};
const struct timespec BFLogIOSyncClass::PrivateData::nsleep10ms[] = {{0, 10000000L}};

static char *lcl_secure_getenv (const char *const var)
{
    if ((getuid() != geteuid()) || (getgid() != getegid()))
        return NULL;
    
#ifdef HAVE_SECURE_GETENV
    return secure_getenv(var);
#else
    return getenv(var);
#endif
}

#endif

// Return the default local server path.
std::string BFLogIOSyncClass::localServerDefault (void)
{
#if defined(_WIN32)

    return ".";

#elif defined(__GNUC__)

    std::string localSrvr;
    
    const char *const tmpdirEnv = lcl_secure_getenv("TMPDIR");

    if (tmpdirEnv)
    {
        struct stat s;
        if (stat(tmpdirEnv, &s) == 0)
        {
            if (S_ISDIR(s.st_mode))
            {
                char cleanPath [PATH_MAX + 1];
                localSrvr = realpath(tmpdirEnv, cleanPath);
            }
            else
                DBG_MSG("TMPDIR is not an accessible directory.\n");
        }
    }

    {
#if defined(P_tmpdir)
        localSrvr = P_tmpdir;
#elif defined(_PATH_TMP)
        localSrvr = _PATH_TMP;
#else
        localSrvr = "/tmp";
#endif
    }

    return localSrvr;

#else
#   error Platform implmentation missing.
#endif
}

// Constructor and deconstructor.
BFLogIOSyncClass::BFLogIOSyncClass (void)
    : m_pd (new PrivateData)
{
#if defined(_WIN32)
    m_pd->m_hOverlappedEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    m_pd->m_hCancelEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
#endif

    setLocal();
}

BFLogIOSyncClass::~BFLogIOSyncClass (void)
{
    if (BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe)
    {
#if defined(_WIN32)
        CloseHandle(m_pd->m_ioPipe);
        m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;
#elif defined(__GNUC__)
        remove(m_pd->m_ioPipeName.c_str());
        m_pd->m_ioPipeName.clear();
        close(m_pd->m_ioPipe);
        m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;
        
        remove(m_pd->m_lockName.c_str());
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
#else
#   error Platform implementation missing.
#endif
    }

#if defined(_WIN32)
    if (m_pd->m_hOverlappedEvent)
    {
        CloseHandle(m_pd->m_hOverlappedEvent);
        m_pd->m_hOverlappedEvent = NULL;
    }

    if (m_pd->m_hCancelEvent)
    {
        CloseHandle(m_pd->m_hCancelEvent);
        m_pd->m_hCancelEvent = NULL;
    }
#endif

    delete m_pd;
}

// Handle rangling.
BFLogIOSync BFLogIOSyncClass::c_sync (void)
{
    return reinterpret_cast<BFLogIOSync>(this);
}
BFLogIOSyncClass* BFLogIOSyncClass::from (BFLogIOSync a_sync)
{
    return reinterpret_cast<BFLogIOSyncClass*>(a_sync);
}

// Sync state.
bool BFLogIOSyncClass::isGood (void) const
{
    bool good = BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe;

#if defined(_WIN32)
    good = good && m_pd->m_hOverlappedEvent && m_pd->m_hCancelEvent;
#elif defined(__GNUC__)
    good = good && BFLOGIO_INVALID_HANDLE != m_pd->m_cancelPipe[0] && BFLOGIO_INVALID_HANDLE != m_pd->m_cancelPipe[1];
#else
#   error Platform implementation missing.
#endif

    return good;
}

// Source properties.
std::string BFLogIOSyncClass::serverName (void)
{
    return m_pd->m_server;
}
bool BFLogIOSyncClass::setServerName (std::string const& a_serverName)
{
    m_pd->m_server = a_serverName;

    // Cleanup any prior connection.
    if (BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe)
    {
        m_pd->cancel(false);

#if defined(_WIN32)
        CloseHandle(m_pd->m_ioPipe);
        m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;
#elif defined(__GNUC__)
        remove(m_pd->m_ioPipeName.c_str());
        m_pd->m_ioPipeName.clear();
        close(m_pd->m_ioPipe);
        m_pd->m_ioPipe = BFLOGIO_INVALID_HANDLE;

        remove(m_pd->m_lockName.c_str());
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
#else
#   error Platform implementation missing.
#endif

        m_pd->m_overflowArray.clear();
    }

#if defined(_WIN32)

    // Create the new pipe.
    m_pd->m_ioPipe = CreateNamedPipeA(("\\\\" + serverName() + "\\pipe\\" + BFLOGIO_PIPE_NAME).c_str()
        , PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_MESSAGE | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE
        , PIPE_WAIT
        , 1
        , (DWORD)BFLogIOMessageClass::ByteArrayLengthMax
        , (DWORD)BFLogIOMessageClass::ByteArrayLengthMax
        , NMPWAIT_USE_DEFAULT_WAIT
        , NULL);

#elif defined(__GNUC__)

    const std::string server = serverName();

    // Derive the lock name.
    m_pd->m_lockName = server + "/" BFLOGIO_PIPE_NAME ".pid";
    
    // Open the lock file.
    m_pd->m_lockFile = open(m_pd->m_lockName.c_str(), O_CREAT | O_RDWR, 0666);
    if (BFLOGIO_INVALID_HANDLE == m_pd->m_lockFile)
    {
        m_pd->m_lockName.clear();
        return false;
    }
    
    // Lock the lock file.
    struct flock lock, setLock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    setLock = lock;
    fcntl(m_pd->m_lockFile, F_GETLK, &lock);
    if (F_UNLCK != lock.l_type || fcntl(m_pd->m_lockFile, F_SETLK, &setLock) == -1)
    {
        // Something prevents our lock. Give up!
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
        return false;
    }
    
    // Write our PID to the lock file.
    const std::string pidStr = std::to_string(getpid()) + "\n";
    if (ftruncate(m_pd->m_lockFile, 0) != 0)
    {
        // We can't seem to modify the PID file.
        remove(m_pd->m_lockName.c_str());
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
        return false;
    }

    if (0 >= write(m_pd->m_lockFile, pidStr.c_str(), pidStr.length()))
    {
        // We can't seem to modify the PID file.
        remove(m_pd->m_lockName.c_str());
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
        return false;
    }

    // Derive the pipe name.
    m_pd->m_ioPipeName = server + "/" BFLOGIO_PIPE_NAME;

    // Remove any existing pipe. Allow this to fail silently.
    remove(m_pd->m_ioPipeName.c_str());

    // Create the FIFO object, with universal RW access.
    if (mkfifo(m_pd->m_ioPipeName.c_str(), 0666) == -1)
    {
        m_pd->m_ioPipeName.clear();
        
        remove(m_pd->m_lockName.c_str());
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
        
        return false;
    }

    // Open the FIFO object.
    m_pd->m_ioPipe = open(m_pd->m_ioPipeName.c_str(), O_RDWR | O_NONBLOCK);
    if (BFLOGIO_INVALID_HANDLE == m_pd->m_ioPipe)
    {
        remove(m_pd->m_ioPipeName.c_str());
        m_pd->m_ioPipeName.clear();
        
        remove(m_pd->m_lockName.c_str());
        m_pd->m_lockName.clear();
        close(m_pd->m_lockFile);
        m_pd->m_lockFile = BFLOGIO_INVALID_HANDLE;
        
        return false;
    }

#else
#   error Platform implementation missing.
#endif

    return BFLOGIO_INVALID_HANDLE != m_pd->m_ioPipe;
}
bool BFLogIOSyncClass::setLocal (void)
{
    return setServerName(localServerDefault());
}

// Wait for the next log message.
BFLogIOSyncClass::Wait BFLogIOSyncClass::waitNextMessage (BFLogIOMessageClass &a_msg, unsigned int ms_timeout)
{
    TIMEOUT_START(ms_timeout, BFLOGIO_INFINITE_TIMEOUT);

    if (!isGood())
        return Wait::SyncIsBad;
    else if (m_pd->canceled())
        return Wait::Canceled;

    std::vector<unsigned char> byteArray = std::move(m_pd->m_overflowArray);

    if (!BFLogIOMessageClass::containsMessageArray(byteArray))
    {
#if defined(_WIN32)

        HANDLE eventHandles[] = { m_pd->m_hOverlappedEvent, m_pd->m_hCancelEvent };
        DWORD winErr;

        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = m_pd->m_hOverlappedEvent;

        ConnectNamedPipe(m_pd->m_ioPipe, &overlapped);
        winErr = GetLastError();
        if (winErr != ERROR_IO_PENDING && winErr != ERROR_PIPE_CONNECTED)
            return Wait::SourceConnectFailed;

        winErr = WaitForMultipleObjects(2, eventHandles, FALSE, TIMEOUT_REMAINING(INFINITE));
        if (WAIT_TIMEOUT == winErr)
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::Timeout;
        }
        else if (WAIT_OBJECT_0 + 1 == winErr)
        {
            if (m_pd->canceled())
            {
                DisconnectNamedPipe(m_pd->m_ioPipe);
                return Wait::Canceled;
            }
        }
        else if (WAIT_OBJECT_0 != winErr)
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::SourceWaitFailed;
        }

        if (!HasOverlappedIoCompleted(&overlapped))
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::SourceDisconnected;
        }

        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = m_pd->m_hOverlappedEvent;

        byteArray.resize(BFLogIOMessageClass::ByteArrayLengthMax);
        ReadFile(m_pd->m_ioPipe, byteArray.data(), (DWORD)byteArray.size(), NULL, &overlapped);
        winErr = GetLastError();
        if (winErr != NO_ERROR && winErr != ERROR_IO_PENDING)
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::StartReadFailed;
        }

        winErr = WaitForMultipleObjects(2, eventHandles, FALSE, TIMEOUT_REMAINING(INFINITE));
        if (WAIT_TIMEOUT == winErr)
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::Timeout;
        }
        else if (WAIT_OBJECT_0 + 1 == winErr)
        {
            if (m_pd->canceled())
            {
                DisconnectNamedPipe(m_pd->m_ioPipe);
                return Wait::Canceled;
            }
        }
        else if (WAIT_OBJECT_0 != winErr)
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::ReadWaitFailed;
        }

        DWORD bytesRead;
        if (!GetOverlappedResult(m_pd->m_ioPipe, &overlapped, &bytesRead, TRUE))
        {
            DisconnectNamedPipe(m_pd->m_ioPipe);
            return Wait::ReadFailed;
        }

        DisconnectNamedPipe(m_pd->m_ioPipe);

        byteArray.resize(bytesRead);

#elif defined(__GNUC__)

        pollfd pfd[2];

        std::vector<unsigned char> tmpTxt (BFLogIOMessageClass::ByteArrayLengthMax);

        while (true)
        {
            memset(pfd, 0, sizeof(pfd));

            pfd[0].fd = m_pd->m_cancelPipe[0];
            pfd[0].events = POLLIN;

            pfd[1].fd = m_pd->m_ioPipe;
            pfd[1].events = POLLIN;

            const int prc = poll(pfd, 2, TIMEOUT_REMAINING(-1));

            if (0 == prc)
                return Wait::Timeout;
            else if (0 > prc)
                return Wait::SourceWaitFailed;

            if (POLLIN & pfd[0].revents)
            {
                // See if we're actually in a cancel state.
                if (m_pd->canceled())
                    return Wait::Canceled;
            }
            else if (POLLIN & pfd[1].revents)
            {
                // Read message data until we have a complete message.
                while (!BFLogIOMessageClass::containsMessageArray(byteArray))
                {
                    // Don't accumulate an absurd length of data.
                    if (byteArray.size() > BFLogIOMessageClass::ByteArrayLengthMax)
                    {
                        std::cerr << "BFLogIO Sync: Received excess data without a valid message. Trimming leading data.\n";
                        byteArray.erase(byteArray.begin(), byteArray.end() - BFLogIOMessageClass::ByteArrayLengthMax);
                    }

                    // Read as much as a whole message worth of data at a time.
                    int readLen = read(m_pd->m_ioPipe, tmpTxt.data(), tmpTxt.size());
                    if (0 < readLen)
                        byteArray.insert(byteArray.end(), tmpTxt.begin(), tmpTxt.begin() + readLen);
                    else if (0 == readLen)
                        break;
                    else if (0 > readLen)
                    {
                        if (EAGAIN == errno)
                            break;
                        else if (EINTR != errno)
                            return Wait::ReadFailed;
                    }

                    // We can timeout.
                    if (TIMEOUT_PASSED())
                        return Wait::Timeout;
                }

                break;
            }
            else if (POLLHUP & pfd[1].revents)
            {
                // The source hung up. Pause here for a millisecond, so we
                // don't hog system resources.
                nanosleep(PrivateData::nsleep10ms, NULL);
            }
            else
            {
                // Something else happened that we don't know about. Print a
                // message about it, then pause for a millisecond.
                fprintf(stderr, "BFLogIO Sync: Unknown poll event with Cancel = %d, I/O Pipe = %d\n", pfd[0].revents, pfd[1].revents);
                nanosleep(PrivateData::nsleep10ms, NULL);
            }

            if (TIMEOUT_PASSED())
                return Wait::Timeout;
        }

#else
#   error Platform implementation missing.
#endif
    }

    const bool gotMsg = a_msg.fromByteArray(byteArray, m_pd->m_overflowArray);

    return gotMsg ? Wait::OK : Wait::MessageDeformed;
}

// Cancel a pending call to waitNextMessage. Cancel state will remain in effect
// until waitNextMessage returns Canceled, or cancelWaitNextMessage is disabled.
bool BFLogIOSyncClass::cancelWaitNextMessage (const bool a_cancel)
{
    return isGood() && m_pd->cancel(a_cancel);
}
