#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SOURCE__CLASS__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SOURCE__CLASS__H

/* FILE:        BFLogIOSourceClass.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Class implementing a BFLogIO Source object.
 */

#include "BFLogIODef.h"

#include "BFLogIOMessageClass.h"

class BFLogIOSourceClass
{
public:
    // Global Source connect blocking. Return previous state.
    static bool BlockAllSources (const bool block);
    static bool AllSourcesBlocked (void);

    // Error Codes.
    enum class Connect
    {
        OK = 0,
        Blocked = 1,
        AlreadyConnected = 2,
        MissingSync = 3,
        Timeout = 4,
        SyncWaitFailed = 5
    };

    enum class Disconnect
    {
        OK = 0,
        NotConnected = 1
    };

    enum class Push
    {
        OK = 0,
        NotConnected = 1,
        WriteFailed = 2
    };

private:
    struct PrivateData;
    PrivateData *m_pd;

    // Illegal.
    BFLogIOSourceClass (BFLogIOSourceClass const&);
    BFLogIOSourceClass& operator= (BFLogIOSourceClass const&);

public:
    // Constructors/Destructors.
    BFLogIOSourceClass (void);
    ~BFLogIOSourceClass (void);

    // Handle rangling.
    BFLogIOSource c_src (void);
    static BFLogIOSourceClass* from (BFLogIOSource a_src);

    // Source status.
    bool isConnected (void) const;

    // Connect to the Sync.
    Connect connect (const unsigned int ms_timeout);

    // Disconnect from the Sync.
    Disconnect disconnect (void);

    // Push a message to all active BFLogIO syncs on this system.
    Push push (BFLogIOMessageClass const& a_msg);
};

#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__SOURCE__CLASS__H
