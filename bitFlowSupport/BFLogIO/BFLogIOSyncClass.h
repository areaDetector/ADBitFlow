#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SYNC__CLASS__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SYNC__CLASS__H

/* FILE:        BFLogIOSyncClass.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Implementation class of a BFLogIO Sync server.
 */

#include "BFLogIODef.h"

#include "BFLogIOMessageClass.h"

class BFLogIOSyncClass
{
public:
    enum class Wait
    {
        OK = 0,
        SyncIsBad = 1,
        Timeout = 2,
        Canceled = 3,
        SourceConnectFailed = 4,
        SourceWaitFailed = 5,
        SourceDisconnected = 6,
        StartReadFailed = 7,
        ReadWaitFailed = 8,
        ReadFailed = 9,
        MessageDeformed = 10
    };

private:
    struct PrivateData;
    PrivateData *m_pd;

    // Illegal.
    BFLogIOSyncClass (BFLogIOSyncClass const&);
    BFLogIOSyncClass& operator= (BFLogIOSyncClass const&);

public:
    // Return the default local server path.
    static std::string localServerDefault (void);

    // Constructor and deconstructor.
    BFLogIOSyncClass (void);
    ~BFLogIOSyncClass (void);

    // Handle rangling.
    BFLogIOSync c_sync (void);
    static BFLogIOSyncClass* from (BFLogIOSync a_sync);

    // Sync state.
    bool isGood (void) const;

    // Get/set the sync server. Remote server name may be used on Windows.
    // Specifies an IPV6 port, on Linux.
    std::string serverName (void);
    bool setServerName (std::string const& a_serverName);

    // Set the effective server to the local machine, with no remote access.
    // Equivalent to setServerName(".") on Windows, or setServerName("") on
    // Linux.
    bool setLocal (void);

    // Wait for the next log message.
    Wait waitNextMessage (BFLogIOMessageClass &a_msg, const unsigned int ms_timeout);

    // Cancel a pending call to waitNextMessage. Cancel state will remain in
    // effect until waitNextMessage returns Canceled, or a cancelWaitNextMessage
    // is disabled.
    bool cancelWaitNextMessage (const bool a_setCancel = true);
};

#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SYNC__CLASS__H
