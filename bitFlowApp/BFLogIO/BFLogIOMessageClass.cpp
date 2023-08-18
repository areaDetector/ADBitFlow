/* FILE:        BFLogIOMessageClass.cpp
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Class implementing a BFLogIO message object.
 */

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#elif defined(__GNUC__)
#   include <sys/types.h>
#   include <unistd.h>
#   include <limits.h>
#else
#   error Platform implementation missing.
#endif

#include "BFLogIOMessageClass.h"

#include <sstream>
#include <algorithm>
#include <ctime>

#if defined(_WIN32)
const size_t BFLogIOMessageClass::ByteArrayLengthMax = BFLOGIO_STRLENMAX * 4;
#elif defined(__GNUC__)
const size_t BFLogIOMessageClass::ByteArrayLengthMax = PIPE_BUF - 1;
#else
#   error Platform implementation missing.
#endif

static const std::string TimestampSentPrefix = "timestampSent=";
static const std::string TypePrefix = "type=";
static const std::string SourcePrefix = "source=";
static const std::string TitlePrefix = "title=";
static const std::string TextPrefix = "text=";
static const std::string ApplicationPrefix = "application=";
static const std::string PIDPrefix = "pid=";

static const char EOFToken[] = ";";
static const size_t EOFTokenLen = sizeof(EOFToken) / sizeof(char);

static const struct
{
    BFLogIOMsgType type;
    std::string name;
} MsgTypeStrs[] = {
    { BFLogIOUnknownType, "Unknown" },
    { BFLogIODebugType, "Debug" },
    { BFLogIONotificationType, "Notification" },
    { BFLogIOWarningType, "Warning" },
    { BFLogIOErrorType, "Error" },
    { BFLogIOFatalErrorType, "FatalError" } };

static const struct
{
    BFLogIOMsgSource source;
    std::string name;
} MsgSourceStrs[] = {
    { BFLogIOUnknownSrc, "Unknown" },
    { BFLogIOUserSrc, "User" },
    { BFLogIODriverSrc, "Driver" },
    { BFLogIOCLSerialSrc, "CLSerial" },
    { BFLogIOCxpRegSrc, "CxpReg" },
    { BFLogIOBufInSrc, "BufIn" },
    { BFLogIOGenTLSrc, "GenTL" } };

struct BFLogIOMessageClass::PrivateData
{
    std::time_t m_timestampSent;
    BFLogIOMsgType m_type;
    BFLogIOMsgSource m_source;
    std::string m_title;
    std::string m_text;
    std::string m_application;
    std::uint64_t m_pid;

    PrivateData (void)
    {
        clear();

#if defined(_WIN32)
        m_pid = GetCurrentProcessId();
#elif defined(__GNUC__)
        m_pid = getpid();
#else
#   error Platform implementation missing.
#endif
    }

    PrivateData (PrivateData const& to_copy)
        : m_timestampSent (to_copy.m_timestampSent)
        , m_type (to_copy.m_type)
        , m_source (to_copy.m_source)
        , m_title (to_copy.m_title)
        , m_text (to_copy.m_text)
        , m_application (to_copy.m_application)
        , m_pid (to_copy.m_pid)
    {
    }

    inline void clear (void)
    {
        m_timestampSent = 0;
        m_type = BFLogIOUnknownType;
        m_source = BFLogIOUnknownSrc;
        m_title.clear();
        m_text.clear();
        m_application.clear();
        m_pid = 0;
    }

    inline static std::string toString (const BFLogIOMsgType type)
    {
        if (BFLogIOUnknownType == type)
            return MsgTypeStrs[0].name;

        std::string typeList;

        for (auto const& msgType : MsgTypeStrs)
        {
            if (msgType.type & type)
            {
                if (typeList.size())
                    typeList += "|";
                typeList += msgType.name;
            }
        }

        return typeList;
    }
    inline static BFLogIOMsgType typeFromString (std::string const& typeList)
    {
        BFLogIOMsgType type = BFLogIOUnknownType;

        std::istringstream typeListStrm (typeList);
        std::string typeStr;
        while (std::getline(typeListStrm, typeStr, '|'))
        {
            for (auto const& msgType : MsgTypeStrs)
            {
                if (msgType.name == typeStr)
                {
                    type = (BFLogIOMsgType)(msgType.type | type);
                    break;
                }
            }
        }

        return type;
    }

    inline static std::string toString (const BFLogIOMsgSource source)
    {
        if (BFLogIOUnknownSrc == source)
            return MsgSourceStrs[0].name;
        
        for (auto const& msgSource : MsgSourceStrs)
        {
            if (msgSource.source == source)
                return msgSource.name;
        }

        return "";
    }
    inline static BFLogIOMsgSource sourceFromString (std::string const& source)
    {
        for (auto const& msgSource : MsgSourceStrs)
        {
            if (msgSource.name == source)
                return msgSource.source;
        }

        return BFLogIOUnknownSrc;
    }

    template <typename IterT>
    inline static bool isEOF (IterT const& iBegin, IterT const& iEnd)
    {
        // EOFToken must have a NULL prefix.
        if (iBegin == iEnd || *iBegin != 0)
            return false;

        // Compare the rest to the EOFToken directly.
        IterT iIter = std::next(iBegin);
        auto eofIter = std::begin(EOFToken);
        while (std::end(EOFToken) != eofIter && iEnd != iIter)
        {
            if (*eofIter++ != *iIter++)
                return false;
        }

        // We have an EOF.
        return true;
    }

    template <typename IterT>
    inline static void byteAppend (std::vector<unsigned char> &byteArray, IterT const& Begin, IterT const& End)
    {
        byteArray.insert(byteArray.end(), Begin, End);
    }
    inline static void bytePack (std::vector<unsigned char> &byteArray, std::string const& dataStr)
    {
        byteAppend(byteArray, dataStr.begin(), dataStr.end());
        byteArray.push_back(0);
    }
    inline static void packEOF (std::vector<unsigned char> &byteArray)
    {
        // Messages terminate with our own EOF.
        if (byteArray.size() >= BFLogIOMessageClass::ByteArrayLengthMax - EOFTokenLen)
        {
            // Remove excess bytes. This condition shouldn't generally be possible.
            DBG_MSG("Source Message size (%u) exceeds max size (%u). Stripping excess bytes.\n", (unsigned int)byteArray.size(), (unsigned int)BFLogIOMessageClass::ByteArrayLengthMax);

            byteArray.resize(BFLogIOMessageClass::ByteArrayLengthMax);
        }
        else if (byteArray.empty() || byteArray.back() != 0)
            byteArray.resize(byteArray.size() + 3);
        else
            byteArray.resize(byteArray.size() + 2);

        auto Iter = byteArray.end() - (EOFTokenLen + 1);
        *Iter++ = 0;
        for (char c : EOFToken)
            *Iter++ = c;
    }

    inline static std::vector<std::string> byteSplit (std::vector<unsigned char> const& a_byteArray, std::vector<unsigned char> &a_overflowArray)
    {
        std::vector<std::string> tokenVec;

        auto tokenStart = a_byteArray.begin();
        auto tokenEnd = tokenStart;
        const auto dataEnd = a_byteArray.end();

        // Find null-delimited tokens until EOF or end-of-data.
        bool isFirstPass = true;
        while (dataEnd != tokenEnd)
        {
            // Get the next token start, if any.
            if (isFirstPass)
                isFirstPass = false;
            else
            {
                tokenStart = std::next(tokenEnd);
                if (dataEnd == tokenStart)
                    break;
            }

            // Find the token end.
            tokenEnd = std::find(tokenStart, dataEnd, 0);

            // Copy the token data.
            std::string token (tokenStart, tokenEnd);

            // An EOFToken is not required, but can delineate the message end.
            if (EOFToken == token)
                break;

            // Drop empty tokens.
            if (token.empty())
                DBG_MSG("Sync Message contained an empty token.");
            else
                tokenVec.push_back(std::move(token));
        }

        // Preserve all overflow data.
        if (dataEnd == tokenEnd)
            a_overflowArray.clear();
        else
            a_overflowArray.assign(std::next(tokenEnd), dataEnd);

        // Return the tokens.
        return tokenVec;
    }

    inline bool unpackToken (std::string const& token, std::string const& prefix, std::string &value)
    {
        if (token.compare(0, prefix.size(), prefix) != 0)
            return false;

        value = token.substr(prefix.size());
        return true;
    }

    inline std::vector<unsigned char> toByteArray (void) const
    {
        std::vector<unsigned char> byteArray;

        time_t timestampSent = time(nullptr);

        // Pack all message componenets, in relative order of importance.
        bytePack(byteArray, PIDPrefix + std::to_string(m_pid));
        bytePack(byteArray, TimestampSentPrefix + std::to_string(timestampSent));
        bytePack(byteArray, TypePrefix + toString(m_type));
        bytePack(byteArray, SourcePrefix + toString(m_source));
        bytePack(byteArray, TitlePrefix + m_title);
        bytePack(byteArray, ApplicationPrefix + m_application);
        bytePack(byteArray, TextPrefix + m_text);
        packEOF(byteArray);

        return byteArray;
    }

    inline bool fromByteArray (std::vector<unsigned char> const& a_byteArray, std::vector<unsigned char> &a_overflowArray)
    {
        clear();

        for (auto const& token : byteSplit(a_byteArray, a_overflowArray))
        {
            std::string tmpStr;

            if (unpackToken(token, TimestampSentPrefix, tmpStr))
            {
                std::istringstream iStrm (tmpStr);
                iStrm >> m_timestampSent;
            }
            else if (unpackToken(token, TypePrefix, tmpStr))
                m_type = typeFromString(tmpStr);
            else if (unpackToken(token, SourcePrefix, tmpStr))
                m_source = sourceFromString(tmpStr);
            else if (unpackToken(token, TitlePrefix, m_title))
            { }
            else if (unpackToken(token, TextPrefix, m_text))
            { }
            else if (unpackToken(token, ApplicationPrefix, m_application))
            { }
            else if (unpackToken(token, PIDPrefix, tmpStr))
            {
                std::istringstream iStrm (tmpStr);
                iStrm >> m_pid;
            }
            else
                DBG_MSG("Sync Message has deformed token: \"%s\"\n", token.c_str());
        }

        return true;
    }
};

BFLogIOMessageClass::BFLogIOMessageClass (void)
    : m_pd (new PrivateData)
{
}
BFLogIOMessageClass::~BFLogIOMessageClass (void)
{
    delete m_pd;
}

BFLogIOMessageClass::BFLogIOMessageClass (BFLogIOMessageClass const& to_copy)
    : m_pd (new PrivateData(*to_copy.m_pd))
{
}
BFLogIOMessageClass::BFLogIOMessageClass (BFLogIOMessageClass &&to_take)
    : m_pd (to_take.m_pd)
{
    to_take.m_pd = nullptr;
}

BFLogIOMessageClass& BFLogIOMessageClass::operator= (BFLogIOMessageClass const& to_copy)
{
    if (this != &to_copy)
    {
        delete m_pd;
        m_pd = new PrivateData(*to_copy.m_pd);
    }
    return *this;
}
BFLogIOMessageClass& BFLogIOMessageClass::operator= (BFLogIOMessageClass &&to_take)
{
    if (this != &to_take)
    {
        delete m_pd;
        m_pd = to_take.m_pd;
        to_take.m_pd = nullptr;
    }
    return *this;
}

// Handle wrangling.
BFLogIOMessage BFLogIOMessageClass::c_msg (void)
{
    return reinterpret_cast<BFLogIOMessage>(this);
}
BFLogIOMessageClass* BFLogIOMessageClass::from (BFLogIOMessage a_msg)
{
    return reinterpret_cast<BFLogIOMessageClass*>(a_msg);
}

// Message I/O.
std::vector<unsigned char> BFLogIOMessageClass::toByteArray (void) const
{
    return m_pd->toByteArray();
}


bool BFLogIOMessageClass::containsMessageArray (std::vector<unsigned char> const& a_byteArray)
{
    // Perform a reverse search, seeking a valid EOFToken. The EOFToken is all
    // we will consider, for now.
    if (a_byteArray.size() >= EOFTokenLen + 1)
    {
        const auto Finish = a_byteArray.data();
        const auto Start = Finish + a_byteArray.size() - EOFTokenLen - 1;

        for (auto Iter = Start; Finish <= Iter; --Iter)
        {
            if (PrivateData::isEOF(Iter, Iter + EOFTokenLen + 1))
                return true;
        }
    }

    return false;
}
bool BFLogIOMessageClass::fromByteArray (std::vector<unsigned char> const& a_byteArray, std::vector<unsigned char> &a_overflowArray)
{
    return m_pd->fromByteArray(a_byteArray, a_overflowArray);
}

// Field accessors/modifiers.
bool BFLogIOMessageClass::hasBeenSent (void) const
{
    return 0 != m_pd->m_timestampSent;
}

std::time_t BFLogIOMessageClass::timestampSent (void) const
{
    return m_pd->m_timestampSent;
}

BFLogIOMsgType BFLogIOMessageClass::type (void) const
{
    return m_pd->m_type;
}
void BFLogIOMessageClass::setType (const BFLogIOMsgType a_type) const
{
    m_pd->m_type = (BFLogIOMsgType)(a_type & BFLogIOTypeMask);
}

BFLogIOMsgSource BFLogIOMessageClass::source (void) const
{
    return m_pd->m_source;
}
void BFLogIOMessageClass::setSource (const BFLogIOMsgSource a_source)
{
    for (auto const& src : MsgSourceStrs)
    {
        if (src.source == a_source)
        {
            m_pd->m_source = a_source;
            break;
        }
    }
}

std::string BFLogIOMessageClass::title (void) const
{
    return m_pd->m_title;
}
void BFLogIOMessageClass::setTitle (std::string const& a_title)
{
    m_pd->m_title = a_title;

    // Strip trailing data to fit within BFLOGIO_STRLENMAX.
    if (m_pd->m_title.size() > BFLOGIO_STRLENMAX)
        m_pd->m_title.resize(BFLOGIO_STRLENMAX);
}

std::string BFLogIOMessageClass::text (void) const
{
    return m_pd->m_text;
}
void BFLogIOMessageClass::setText (std::string const& a_text)
{
    m_pd->m_text = a_text;

    // Strip trailing data to fit within BFLOGIO_STRLENMAX.
    if (m_pd->m_text.size() > BFLOGIO_STRLENMAX)
        m_pd->m_text.resize(BFLOGIO_STRLENMAX);
}

std::string BFLogIOMessageClass::application (void) const
{
    return m_pd->m_application;
}
void BFLogIOMessageClass::setApplication (std::string const& a_application)
{
    m_pd->m_application = a_application;

    // Strip leading data to fit within BFLOGIO_STRLENMAX.
    if (m_pd->m_application.size() > BFLOGIO_STRLENMAX)
    {
        const auto Begin = m_pd->m_application.begin();
        m_pd->m_application.erase(Begin, Begin + (m_pd->m_application.size() - BFLOGIO_STRLENMAX));
    }
}

std::uint64_t BFLogIOMessageClass::pid (void) const
{
    return m_pd->m_pid;
}
