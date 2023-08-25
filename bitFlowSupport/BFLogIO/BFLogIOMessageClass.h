#ifndef INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__MESSAGE__CLASS__H
#define INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__MESSAGE__CLASS__H

/* FILE:        BFLogIOMessageClass.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Class implementing a BFLogIO message object.
 */

#include "BFLogIODef.h"

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#if defined(_GLIBCXX_DEBUG_ONLY) || defined(_DEBUG)
#   include <cstdio>
#   if defined(_WIN32)
#       define DBG_MSG(FMT, ...) fprintf(stderr, __FUNCTION__ "[%d]: " FMT, __LINE__, __VA_ARGS__)
#   elif defined(__GNUC__)
#       define DBG_MSG(FMT, ...) fprintf(stderr, "%s[%d]: " FMT, __func__, __LINE__, ##__VA_ARGS__)
#   else
#       error Platform implementation missing.
#   endif
#else
#   define DBG_MSG(...) ((void)0 /* NoOp */)
#endif

class BFLogIOMessageClass
{
private:
    struct PrivateData;
    PrivateData *m_pd;

public:
    static const size_t ByteArrayLengthMax;

    BFLogIOMessageClass (void);
    ~BFLogIOMessageClass (void);

    BFLogIOMessageClass (BFLogIOMessageClass const& to_copy);
    BFLogIOMessageClass (BFLogIOMessageClass &&to_take);

    BFLogIOMessageClass& operator= (BFLogIOMessageClass const& to_copy);
    BFLogIOMessageClass& operator= (BFLogIOMessageClass &&to_take);

    // Handle wrangling.
    BFLogIOMessage c_msg (void);
    static BFLogIOMessageClass* from (BFLogIOMessage a_msg);

    // Message I/O.
    std::vector<unsigned char> toByteArray (void) const;

    static bool containsMessageArray (std::vector<unsigned char> const& a_byteArray);
    bool fromByteArray (std::vector<unsigned char> const& a_byteArray, std::vector<unsigned char> &a_overflowArray);

    // Field accessors/modifiers.
    bool hasBeenSent (void) const;

    std::time_t timestampSent (void) const;

    BFLogIOMsgType type (void) const;
    void setType (const BFLogIOMsgType a_type) const;

    BFLogIOMsgSource source (void) const;
    void setSource (const BFLogIOMsgSource a_source);

    std::string title (void) const;
    void setTitle (std::string const& a_title);

    std::string text (void) const;
    void setText (std::string const& a_text);

    std::string application (void) const;
    void setApplication (std::string const& a_application);

    std::uint64_t pid (void) const;
};

#endif // INCLUDED__BIT_FLOW__B_F_LOG_I_O__B_F_LOG__I_O__MESSAGE__CLASS__H
