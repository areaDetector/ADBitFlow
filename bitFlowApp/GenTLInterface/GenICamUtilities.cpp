/* FILE:        GenICamUtilities.cpp
 * DATE:        6/19/2012, 3/6/2013
 * AUTHOR:      Jeremy Greene
 * COPYRIGHT:   BitFlow, Inc., 2012-2013
 * DESCRIPTION: Implementation of the GenTL_CTI class.
 */

#if defined(_WIN32) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

#define DO_BF_GENICAM_UTILTIES_EXPORT

#include "GenICamUtilities.h"
#include "GenICamUtilities_Private.h"
#include "SubFileIterator.h"

#if defined(_WIN32)
#   include <Windows.h>
#elif defined(__GNUC__)
#   include <dlfcn.h>
#   include <BFLogClientApi.h>
#else
#   error Platform implementation missing.
#endif

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <regex>
#include <cstdio>

#include "Port.h"

GenTL_CTI::PrivateData::PrivateData (GenTL_CTI_string const& gentlPath)
    : hCti                      (NULL)

    , GCGetInfo                 (0)
    , GCGetLastError            (0)
    , GCInitLib                 (0)
    , GCCloseLib                (0)
    , GCReadPort                (0)
    , GCWritePort               (0)
    , GCGetPortURL              (0)
    , GCGetPortInfo             (0)

    , GCRegisterEvent           (0)
    , GCUnregisterEvent         (0)
    , EventGetData              (0)
    , EventGetDataInfo          (0)
    , EventGetInfo              (0)
    , EventFlush                (0)
    , EventKill                 (0)
    , TLOpen                    (0)
    , TLClose                   (0)
    , TLGetInfo                 (0)
    , TLGetNumInterfaces        (0)
    , TLGetInterfaceID          (0)
    , TLGetInterfaceInfo        (0)
    , TLOpenInterface           (0)
    , TLUpdateInterfaceList     (0)
    , IFClose                   (0)
    , IFGetInfo                 (0)
    , IFGetNumDevices           (0)
    , IFGetDeviceID             (0)
    , IFUpdateDeviceList        (0)
    , IFGetDeviceInfo           (0)
    , IFOpenDevice              (0)
    , DevGetPort                (0)
    , DevGetNumDataStreams      (0)
    , DevGetDataStreamID        (0)
    , DevOpenDataStream         (0)
    , DevGetInfo                (0)
    , DevClose                  (0)
    , DSAnnounceBuffer          (0)
    , DSAllocAndAnnounceBuffer  (0)
    , DSFlushQueue              (0)
    , DSStartAcquisition        (0)
    , DSStopAcquisition         (0)
    , DSGetInfo                 (0)
    , DSGetBufferID             (0)
    , DSClose                   (0)
    , DSRevokeBuffer            (0)
    , DSQueueBuffer             (0)
    , DSGetBufferInfo           (0)

#if GENTL_H_AT_LEAST_V(1,1,0)
    , GCGetNumPortURLs          (0)
    , GCGetPortURLInfo          (0)
    , GCReadPortStacked         (0)
    , GCWritePortStacked        (0)
#endif

#if GENTL_H_AT_LEAST_V(1,3,0)
    , DSGetBufferChunkData      (0)
#endif

#if GENTL_H_AT_LEAST_V(1,4,0)
    , IFGetParentTL             (0)
    , DevGetParentIF            (0)
    , DSGetParentDev            (0)
#endif

#if GENTL_H_AT_LEAST_V(1,5,0)
    , DSGetNumBufferParts       (0)
    , DSGetBufferPartInfo       (0)
#endif

    , lastErrorCode             (GC_ERR_SUCCESS)
    , recordErrors              (true)

    , blockPortReads            (false)
    , blockPortWrites           (false)
    , cancelNext                (false)

    , portReadDebugLogEnabled   (false)
    , portReadLog               (new std::ofstream)
    , portWriteDebugLogEnabled  (false)
    , portWriteLog              (new std::ofstream)

#if USE_BFOUTPOUTDEBUGSTRINGEX
    , bfOutputDebugStringEx     (nullptr)
#endif
{
#if defined(_WIN32)

    hCti = LoadLibrary(gentlPath.c_str());
    if (!hCti)
        throw std::runtime_error("Unable to load the specified GenTL file.");

#   if USE_BFOUTPOUTDEBUGSTRINGEX

    if (HMODULE hBFD = LoadLibraryA("BFD.dll"))
    {
        GCUErr("Found BFD!");
        if (bfOutputDebugStringEx = (PBFOutputDebugStringEx)GetProcAddress(hBFD, "BFOutputDebugStringEx"))
            GCUErr("Found BFOutputDebugStringEx!");
    }

#   endif

#elif defined(__GNUC__)

    hCti = dlopen(gentlPath.c_str(), RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE);
    if (!hCti)
        throw std::runtime_error(std::string("Unable to load the specified GenTL file: ").append(dlerror()));

#   if USE_BFOUTPOUTDEBUGSTRINGEX
    bfOutputDebugStringEx = &BFLogClientMessagePush;
#   endif

#else
#   error Platform implementation missing.
#endif
}

GenTL_CTI::PrivateData::~PrivateData (void)
{
    if (hCti)
    {
#if defined(_WIN32)
        FreeLibrary(hCti);
#elif defined(__GNUC__)
        dlclose(hCti);
#else
#   error Platform implementation missing.
#endif

        hCti = nullptr;
    }

    const std::lock_guard<std::mutex> portLogLock (portLogMutex);
    put_file_closed(*portReadLog, portReadFileLogPath, "READ");
    put_file_closed(*portWriteLog, portWriteFileLogPath, "WRITE");
}

std::string GenTL_CTI::PrivateData::cdata_format (std::string const& cdata)
{
    std::string stringFormatted = cdata;
    const std::string cdataSub ("]]]]><![CDATA[>");
    for ( size_t cdataAt = stringFormatted.find("]]>")
        ; std::string::npos != cdataAt
        ; cdataAt = stringFormatted.find("]]>", cdataSub.length() + cdataAt) )
        stringFormatted.replace(cdataAt, 3, cdataSub);

    return "<![CDATA[" + stringFormatted + "]]>";
}

char GenTL_CTI::PrivateData::byte_to_char (const std::uint8_t val)
{
    const std::uint8_t byteVal = val & 0xF;
    if (0x0A <= byteVal)
        return 'A' + byteVal - 0x0A;
    return '0' + byteVal;
}

void GenTL_CTI::PrivateData::put_time (std::ostream &strm, const size_t indent)
{
    const std::string indent1 = "\n" + std::string(indent, ' ');

    auto rawTime = time(nullptr);
    tm tmNow;
#if defined(_WIN32)
    localtime_s(&tmNow, &rawTime);
#elif defined(__GNUC__)
    localtime_r(&rawTime, &tmNow);
#else
#   error Platform implementation missing.
#endif
    
    char timeStr [128];
    if (strftime(timeStr, sizeof(timeStr), "%c", &tmNow))
        strm << indent1 << "<time>" << timeStr << "</time>";
}
void GenTL_CTI::PrivateData::put_reg_op (std::ostream &strm, const size_t indent, const std::uint8_t *const pBuf, const int64_t Address, const int64_t Length)
{
    const std::string indent1 = "\n" + std::string(indent, ' ');
    const std::string indent2 = "\n" + std::string(indent + 1, ' ');

    if (4 == Length)
    {
        const std::uint32_t wordVal
            = (std::uint32_t(pBuf[0]) << 24)
            | (std::uint32_t(pBuf[1]) << 16)
            | (std::uint32_t(pBuf[2]) << 8)
            | (std::uint32_t(pBuf[3]) << 0);

        strm
        << indent1 << "<reg>"
        << "<address>0x" << std::hex << std::setw(8) << std::uppercase << std::setfill('0') << Address << "</address>"
        << "<data>0x" << std::hex << std::setw(8) << std::uppercase << std::setfill('0') << wordVal << "</data>"
        << "</reg>";
    }
    else
    {
        strm
        << indent1 << "<reg>"
        << indent2 << "<address>0x" << std::hex << std::setw(8) << std::uppercase << std::setfill('0') << Address << "</address>"
        << indent2 << "<length>0x" << std::hex << std::setw(8) << std::uppercase << std::setfill('0') << Length << "</length>"
        << indent2 << "<string>" << cdata_format( std::string(reinterpret_cast<const char*>(pBuf), (size_t)Length) ) << "</string>";

        strm
        << indent2 << "<data>";

        auto *const bufBegin = pBuf;
        auto *const bufEnd = bufBegin + Length;
        for (auto bufIter = bufBegin; bufIter < bufEnd; ++bufIter)
        {
            strm.put( byte_to_char(*bufIter >> 4) );
            strm.put( byte_to_char(*bufIter) );
        }

        strm
        << "</data>"
        << indent1 << "</reg>";
    }
}

void GenTL_CTI::PrivateData::put_file_opened (std::ostream &strm, std::string const& path, std::string const& portOpType)
{
    if (!strm.good())
    {
        std::cerr << "Bad output stream for the port " + portOpType + " log.\n";
        return;
    }

    strm << "<file_op type=\"OPEN\">";
    put_time(strm, 1);
    strm << "\n <path>" << path << "</path>";
    strm << "\n <port_op type=\"" << portOpType << "\"/>";
    strm << "\n</file_op>\n";

    strm.flush();
}
void GenTL_CTI::PrivateData::put_file_closed (std::ostream &strm, std::string const& path, std::string const& portOpType)
{
    if (!strm.good())
        return;

    strm << "<file_op type=\"CLOSE\">";
    put_time(strm, 1);
    strm << "\n <path>" << path << "</path>";
    strm << "\n <port_op type=\"" << portOpType << "\"/>";
    strm << "\n</file_op>\n";

    strm.flush();
}

void GenTL_CTI::PrivateData::put_port_op (std::ostream &strm, std::string const& opType, std::string const& portName, std::string const& hint, const GC_ERROR err, const void *const pBuffer, const int64_t Address, const int64_t Length)
{
    if (!strm.good())
        return;
    
    strm << "<port_op type=\"" << opType << "\">";
    put_time(strm, 1);
    strm << "\n <port_id>" << portName << "</port_id>";

    if (hint.size())
        strm << "\n <hint>" << hint << "</hint>";

    put_reg_op(strm, 1, reinterpret_cast<const std::uint8_t*>(pBuffer), Address, Length);

    if (GC_ERR_SUCCESS != err)
        strm << "\n <error>" << GenTL_CTI::gentlError(err, opType + " ERROR") << "</error>";

    strm << "\n</port_op>\n";

    strm.flush();
}

void GenTL_CTI::PrivateData::log_port_read (std::string const& portName, const GC_ERROR err, const void *const pBuffer, const int64_t Address, const int64_t Length)
{
    const std::lock_guard<std::mutex> lock (portLogMutex);

    const bool doDebugLog = portReadDebugLogEnabled.load();
    const bool doFileLog = portReadLog->good();

    if (doDebugLog)
    {
        std::ostringstream tmpLogStrm;
        put_port_op(tmpLogStrm, "READ", portName, portReadLogHint, err, pBuffer, Address, Length);
        const std::string tmpLog = tmpLogStrm.str();

#if USE_BFOUTPOUTDEBUGSTRINGEX

        if (bfOutputDebugStringEx)
        {
            const std::string msgTitle = "READ " + portName;
            std::vector<char> msgTitleVec (msgTitle.begin(), msgTitle.end());
            msgTitleVec.push_back(0);

            std::vector<char> tmpLogVec (tmpLog.begin(), tmpLog.end());
            tmpLogVec.push_back(0);

            if (!!bfOutputDebugStringEx(GC_ERR_SUCCESS == err ? BFLogIODebugType : BFLogIOErrorType, BFLogIOGenTLSrc, msgTitleVec.data(), tmpLogVec.data()))
                GCUErr(tmpLog.c_str());
        }
        else
            GCUErr(tmpLog.c_str());

#else

        GCUErr(tmpLog.c_str());

#endif

        if (doFileLog)
        {
            portReadLog->write(tmpLog.c_str(), tmpLog.size());
            portReadLog->flush();
        }
    }
    else if (doFileLog)
        put_port_op(*portReadLog, "READ", portName, portReadLogHint, err, pBuffer, Address, Length);
}
void GenTL_CTI::PrivateData::log_port_write (std::string const& portName, const GC_ERROR err, const void *const pBuffer, const int64_t Address, const int64_t Length)
{
    const std::lock_guard<std::mutex> lock (portLogMutex);
    
    const bool doDebugLog = portWriteDebugLogEnabled.load();
    const bool doFileLog = portWriteLog->good();

    if (doDebugLog)
    {
        std::ostringstream tmpLogStrm;
        put_port_op(tmpLogStrm, "WRITE", portName, portWriteLogHint, err, pBuffer, Address, Length);
        const std::string tmpLog = tmpLogStrm.str();

#if USE_BFOUTPOUTDEBUGSTRINGEX

        if (bfOutputDebugStringEx)
        {
            const std::string msgTitle = "WRITE " + portName;
            std::vector<char> msgTitleVec (msgTitle.begin(), msgTitle.end());
            msgTitleVec.push_back(0);

            std::vector<char> tmpLogVec (tmpLog.begin(), tmpLog.end());
            tmpLogVec.push_back(0);

            if (!!bfOutputDebugStringEx(GC_ERR_SUCCESS == err ? BFLogIODebugType : BFLogIOErrorType, BFLogIOGenTLSrc, msgTitleVec.data(), tmpLogVec.data()))
                GCUErr(tmpLog.c_str());
        }
        else
            GCUErr(tmpLog.c_str());

#else

        GCUErr(tmpLog.c_str());

#endif

        if (doFileLog)
        {
            portWriteLog->write(tmpLog.c_str(), tmpLog.size());
            portWriteLog->flush();
        }
    }
    else if (doFileLog)
        put_port_op(*portWriteLog, "WRITE", portName, portWriteLogHint, err, pBuffer, Address, Length);
}

std::string GenTL_CTI::gentlError (const int status, std::string const& custom_msg)
{
    switch (status)
    {
    case GC_ERR_SUCCESS:
        return custom_msg + " -- GenICam success (GC_ERR_SUCCESS).";
    case GC_ERR_NOT_INITIALIZED:
        return custom_msg + " -- GenICam feature has not been initialized (GC_ERR_NOT_INITIALIZED).";
    case GC_ERR_NOT_IMPLEMENTED:
        return custom_msg + " -- GenICam feature not implemented (GC_ERR_NOT_IMPLEMENTED).";
    case GC_ERR_RESOURCE_IN_USE:
        return custom_msg + " -- GenICam resource in use (GC_ERR_RESOURCE_IN_USE).";
    case GC_ERR_ACCESS_DENIED:
        return custom_msg + " -- GenICam access denied (GC_ERR_ACCESS_DENIED).";
    case GC_ERR_INVALID_HANDLE:
        return custom_msg + " -- GenICam received an invalid handle (GC_ERR_INVALID_HANDLE).";
    case GC_ERR_INVALID_ID:
        return custom_msg + " -- GenICam received an invalid ID (GC_ERR_INVALID_ID).";
    case GC_ERR_NO_DATA:
        return custom_msg + " -- GenICam has no data (GC_ERR_NO_DATA).";
    case GC_ERR_INVALID_PARAMETER:
        return custom_msg + " -- GenICam received an invalid parameter (GC_ERR_INVALID_PARAMETER).";
    case GC_ERR_IO:
        return custom_msg + " -- GenICam encountere an IO error (GC_ERR_IO).";
    case GC_ERR_TIMEOUT:
        return custom_msg + " -- GenICam timed out (GC_ERR_TIMEOUT).";
    case GC_ERR_ABORT:
        return custom_msg + " -- GenICam received an abort message (GC_ERR_ABORT).";
    case GC_ERR_INVALID_BUFFER:
        return custom_msg + " -- GenICam was provided an invalid buffer (GC_ERR_INVALID_BUFFER).";
    case GC_ERR_NOT_AVAILABLE:
        return custom_msg + " -- GenICam feature not available (GC_ERR_NOT_AVAILABLE).";
    case GC_ERR_INVALID_ADDRESS:
        return custom_msg + " -- GenICam received an invalid address (GC_ERR_INVALID_ADDRESS).";
    case GC_ERR_BUFFER_TOO_SMALL:
        return custom_msg + " -- GenICam could not return the requested data because the provided buffer is too small (GC_ERR_BUFFER_TOO_SMALL).";
    case GC_ERR_INVALID_INDEX:
        return custom_msg + " -- The specified index does not exist (GC_ERR_INVALID_INDEX).";
    case GC_ERR_PARSING_CHUNK_DATA:
        return custom_msg + " -- The chunk data is badly formatted and couldn't be parsed (GC_ERR_PARSING_CHUNK_DATA).";
    case GC_ERR_INVALID_VALUE:
        return custom_msg + " -- The value specified is unknown or invalid (GC_ERR_INVALID_VALUE).";
    case GC_ERR_RESOURCE_EXHAUSTED:
        return custom_msg + " -- The requested resource has been exhausted (GC_ERR_RESOURCE_EXHAUSTED).";
    case GC_ERR_OUT_OF_MEMORY:
        return custom_msg + " -- The system has insufficient memory to complete the operation (GC_ERR_OUT_OF_MEMORY).";
    case GC_ERR_BUSY:
        return custom_msg + " -- The resource is busy. Try again later (GC_ERR_BUSY).";
    case GC_ERR_ERROR:
        return custom_msg + " -- GenICam error (GC_ERR_ERROR).";
    }

    return custom_msg + " -- Unknown or non-GenICam error.";
}

GenTL_CTI::UrlScheme GenTL_CTI::processUrl (GenTL_CTI_string &url, const bool stripLocale)
{
    // Generate a lowercase copy of the locale eligible portion of the URL string.
    GenTL_CTI_string url_lowered = url.substr(0, sizeof "local:///" - 1);
    std::transform(url_lowered.begin(), url_lowered.end(), url_lowered.begin(), towlower);

    // Determine the scheme, and decode the URL, if necessary.
    UrlScheme scheme = UnknownScheme;
    size_t schemeLength = 0;
    if (      url_lowered.compare(0, 5, BF_GENICAM_UTILTIES_TEXT("file:")) == 0 )    // "file:(///)?({A-Z](:|\|))?"
    {
        url = gcstr2ctistr(GenICam::UrlDecode( gcstring(url.c_str()) ));
        scheme = GenTL_CTI::FileScheme;
        schemeLength = url_lowered.compare(5, 3, BF_GENICAM_UTILTIES_TEXT("///")) == 0 ? 8 : 5;

        // If necessary, convert drive letter pipe to colon, ie. C| -> C:
        if ( url.length() >= schemeLength + 2 && iswalpha(url[schemeLength]) && BF_GENICAM_UTILTIES_TEXT('|') == url[schemeLength + 1] )
            url[schemeLength + 1] = BF_GENICAM_UTILTIES_TEXT(':');
    }
    else if ( url_lowered.compare(0, 7, BF_GENICAM_UTILTIES_TEXT("http://")) == 0 )  // "http://"
    {
        url = gcstr2ctistr(GenICam::UrlDecode( gcstring(url.c_str()) ));
        scheme = GenTL_CTI::HttpScheme;
        schemeLength = sizeof "http://" - 1;
    }
    else if ( url_lowered.compare(0, 6, BF_GENICAM_UTILTIES_TEXT("local:")) == 0 )   // "local:[///]"
    {
        scheme = GenTL_CTI::LocalScheme;
        schemeLength = url_lowered.compare(6, 3, BF_GENICAM_UTILTIES_TEXT("///")) == 0 ? 9 : 6;
    }
    else
        throw std::runtime_error( gcstring("Received a URL with an unrecognized or ill-formated scheme. URL: \"").append(gcstring(url_lowered.c_str())).append("\"").c_str() );

    // Remove the locale prefix, if specified.
    if (stripLocale)
        url = url.substr(schemeLength);

    return scheme;
}

GenTL_CTI::PortPtr GenTL_CTI::makePort (PORT_HANDLE hPort)
{
    return PortPtr(new GCPort(hPort, this));
}

GenTL_CTI_string GenTL_CTI::retrieveXml (PORT_HANDLE hPort, NodeMapPtr const& pDeviceMap, const unsigned int xmlIndex)
{
    // Disable error recording, for this scope.
    const bool errs = recordErrors(false);

    GC_ERROR status;
    uint32_t iUrlCount = 0;
    size_t iUrlLength = 0;
    size_t iLength;
    uint64_t iAddr = 0;
    size_t iXMLSize = 0;
    gcstring gcstrXml;

#if defined(_WIN32)
    wchar_t *pEnd;
#elif defined(__GNUC__)
    uint64_t tmpUInt64;
#endif

    GenTL_CTI_string strFilename;
    iLength = 2048;

    iUrlLength = 0;

    // Validate the xmlIndex.
    status = GCGetNumPortURLs( hPort, &iUrlCount );
    if (GC_ERR_SUCCESS != status)
        return GenTL_CTI_string();

    if (xmlIndex >= iUrlCount)
        throw std::runtime_error("No XML file exists for the given index.");

    // Retrieve the XML URL.
    INFO_DATATYPE dt;
    status = GCGetPortURLInfo( hPort, xmlIndex, URL_INFO_URL, &dt, 0, &iUrlLength );
    if (GC_ERR_SUCCESS != status)
        return GenTL_CTI_string();

    std::vector<char> sURL (iUrlLength + 1, 0);
    status = GCGetPortURLInfo( hPort, xmlIndex, URL_INFO_URL, &dt, &sURL.at(0), &iUrlLength );
    if (GC_ERR_SUCCESS != status)
        return GenTL_CTI_string();

    // Read XML Address
    GenTL_CTI_string strXMLAddress = gcstr2ctistr(&sURL.at(0));
    switch (processUrl(strXMLAddress))
    {
    case LocalScheme:
        {
            // Retrieve the file name string.
            size_t iOffset = 6;
            iLength = (uint32_t)strXMLAddress.find(BF_GENICAM_UTILTIES_TEXT(";"), iOffset);
            strFilename = strXMLAddress.substr(iOffset, iLength - iOffset);

            // Retrieve the XML address.
            iOffset = iLength+1;
            iLength = strXMLAddress.find(BF_GENICAM_UTILTIES_TEXT(";"), iOffset);
            const auto addrStr = strXMLAddress.substr(iOffset, iLength-iOffset);

#if defined(_WIN32)
            iAddr = _wcstoui64(addrStr.c_str(), &pEnd, 16);
#elif defined(__GNUC__)
            if (std::sscanf(addrStr.c_str(), "%" SCNx64, &tmpUInt64) == 1)
                iAddr = tmpUInt64;
#else
#   error Platform implementation missing.
#endif

            // Retrieve the XML size.
            iOffset = iLength+1;
            iLength = strXMLAddress.size();
            const auto sizeStr = strXMLAddress.substr(iOffset, iLength-iOffset);

#if defined(_WIN32)
            iXMLSize = (size_t)_wcstoui64(sizeStr.c_str(), &pEnd, 16);
#elif defined(__GNUC__)
            if (std::sscanf(sizeStr.c_str(), "%" SCNx64, &tmpUInt64) == 1)
                iXMLSize = (size_t)tmpUInt64;
#else
#   error Platform implementation missing.
#endif

            // Attempt to retrieve the XML file in one gulp.
            std::vector<char> pXML (iXMLSize);
            if (GCReadPort(hPort, iAddr, pXML.data(), &iXMLSize) < 0)
                throw std::runtime_error("Error while retrieving port XML");

            pXML.back() = '\0';
            gcstrXml = pXML.data();

            // If the XML file is empty, throw an error.
            if (gcstrXml.empty())
                throw std::runtime_error("Unable to retrieve XML");

            // Open the XML string from the port.
            pDeviceMap->_LoadXMLFromString(gcstrXml);
        }
        break;

    case FileScheme:
        pDeviceMap->_LoadXMLFromFile( gcstring(strXMLAddress.substr(0, strXMLAddress.rfind(BF_GENICAM_UTILTIES_TEXT(".xml")) + 4).c_str()) );
        break;

    case HttpScheme:
        throw std::runtime_error(gcstring(( BF_GENICAM_UTILTIES_TEXT("BitFlow GenTL consumer interface does not support download of XML files from HTTP. See URL: \"http://") + strXMLAddress + BF_GENICAM_UTILTIES_TEXT("\"") ).c_str()).c_str());

    default:
        throw std::runtime_error(gcstring(( BF_GENICAM_UTILTIES_TEXT("The XML file URL has no prefix, or an unknown prefix (") + strXMLAddress + BF_GENICAM_UTILTIES_TEXT(").")).c_str() ).c_str());
    }

    recordErrors(errs);
    return gcstr2ctistr(gcstrXml);
}

void GenTL_CTI::getNodeMap (PORT_HANDLE hPort, NodeMapPtr const& pDeviceMap, GenApi::IPort *const portPtr, const unsigned int xmlIndex)
{
    // Disable error recording, for this scope.
    const bool errs = recordErrors(false);

    try
    {
        retrieveXml ( hPort, pDeviceMap, xmlIndex );
    }
    catch (GenICam::GenericException const& err)
    {
        throw std::runtime_error(err.what());
    }

    try
    {
        const std::string name = pDeviceMap->_GetDeviceName().c_str();
        if ( !pDeviceMap->_Connect(portPtr, name.c_str()) )
            throw std::runtime_error("Failed to connect the port to the device.");
    }
    catch(GenICam::GenericException const& err)
    {
        recordErrors(errs);
        throw std::runtime_error(err.what());
    }
    catch (...)
    {
        recordErrors(errs);
        throw;
    }

    recordErrors(errs);
}

GenTL_CTI::NodeMapPtr GenTL_CTI::getNodeMap (PORT_HANDLE hPort, GenApi::IPort *const portPtr, std::string gcstrPortName, const unsigned int xmlIndex)
{
    if (gcstrPortName.empty())
    {
        size_t iSize = 0;
        INFO_DATATYPE dt;
        if (GCGetPortInfo(hPort, PORT_INFO_PORTNAME, &dt, 0, &iSize) < 0 || !iSize)
            throw std::runtime_error("Unable to retrieve the GenICam PORT_INFO_PORTNAME value.");

        std::vector<char> portName (iSize + 1, 0);
        if (GCGetPortInfo(hPort, PORT_INFO_PORTNAME, &dt, &portName.at(0), &iSize) < 0)
            throw std::runtime_error("Unable to retrieve the GenICam PORT_INFO_PORTNAME value.");

        portName.back() = '\0';
        gcstrPortName = &portName.at(0);
    }

    const NodeMapPtr pDeviceMap ( new CNodeMapRef (gcstrPortName.c_str()) );
    getNodeMap(hPort, pDeviceMap, portPtr, xmlIndex);
    return pDeviceMap;
}
GenTL_CTI::NodeMapPtr GenTL_CTI::getNodeMap (PORT_HANDLE hPort, PortPtr &portPtr, std::string gcstrPortName, const unsigned int xmlIndex)
{
    if (!portPtr.get())
        portPtr = makePort(hPort);
    return getNodeMap(hPort, portPtr.get(), gcstrPortName, xmlIndex);
}

// Take a semicolon list of paths (pathsList), and search each path for the file named by
// fileName. If a matching file is found, return its complete path as a string. Otherwise,
// return an empty string.
GenTL_CTI_string GenTL_CTI::findFileForPaths (GenTL_CTI_string const& pathsList, GenTL_CTI_string const& fileName)
{
    for ( GenTL_CTI_string::size_type start = 0, end = pathsList.find(GENICAM_GENTL_PATH_SEP);
          start != GenTL_CTI_string::npos;
          start = (end == GenTL_CTI_string::npos) ? end : end + 1,
          end = pathsList.find(GENICAM_GENTL_PATH_SEP, start) )
    {
        // Only proceed with this entry if it is conceivably valid.
        if (end - start < 2)
            continue;

        const GenTL_CTI_string rawPath = pathsList.substr(start, end == GenTL_CTI_string::npos ? end : end - start) + BF_GENICAM_UTILTIES_PATH_SEP + fileName;
        const GenTL_CTI_string cleanPath = SubFileIterator::cleanPath(rawPath);
        const std::ifstream pathStrm (cleanPath);
        if (pathStrm.good())
            return cleanPath;
    }

    // Return an empty string, upon failure.
    return BF_GENICAM_UTILTIES_TEXT("");
}

// Search all paths in the provided paths list (semicolon separated) for files whos names
// match the provided pattern, which should be an ECMA-script style regular expression.
// Fill fileList with each matching path, and return the number of paths found. */
size_t GenTL_CTI::findAllMatchingFiles (GenTL_CTI_string const& pathsList, GenTL_CTI_string const& pattern, std::vector<GenTL_CTI_string> &fileList)
{
#if defined(_WIN32)
    const std::wregex patternRegex (pattern, std::regex_constants::ECMAScript);
#elif defined(__GNUC__)
    const std::regex patternRegex (pattern, std::regex_constants::ECMAScript);
#else
#   error Platform implementation missing.
#endif

    fileList.clear();

    for ( GenTL_CTI_string::size_type start = 0, end = pathsList.find(GENICAM_GENTL_PATH_SEP);
          start != GenTL_CTI_string::npos;
          start = (end == GenTL_CTI_string::npos) ? end : end + 1,
          end = pathsList.find(GENICAM_GENTL_PATH_SEP, start) )
    {
        // Only proceed with this entry if it is conceivably valid.
        if (end - start < 2)
            continue;

        // Check each file in the directory.
        const GenTL_CTI_string basePath = pathsList.substr(start, end == GenTL_CTI_string::npos ? end : end - start);
        for (SubFileIterator fileIter (basePath, BF_GENICAM_UTILTIES_TEXT("*")); fileIter; ++fileIter)
        {
            const auto candidate = fileIter.file_path();
            if (fileIter.is_file() && std::regex_match(candidate, patternRegex))
                fileList.push_back(candidate);
        }
    }

    return fileList.size();
}

/* Fill the provided list with paths to all available GenTL CTI files on this machine. */
size_t GenTL_CTI::findCtiFiles(std::vector<GenTL_CTI_string> &fileList)
{
    // Find all CTI files in the standard environment variable location.
    gcstring TLPath = GenICam::GetValueOfEnvironmentVariable(GENICAM_GENTL_PATH_VAR_NAME);
    findAllMatchingFiles(gcstr2ctistr(TLPath), BF_GENICAM_UTILTIES_TEXT(".*\\.cti"), fileList);
    return fileList.size();
}

/* Return a GenTL instance for the specified cti file path, null upon failure. */
GenTL_CTI::Ptr GenTL_CTI::openCtiFilePath(GenTL_CTI_string const& ctiFilePath)
{
    // Attempt to open the GenTL CTI object.
    Ptr ptr;
    try   
    {
        ptr.reset(new GenTL_CTI(ctiFilePath));
    }
    catch (std::runtime_error const& err)
    {
        GCUErr(err.what());
    }
    catch (...)
    {
    }

    return ptr;
}

/* Find and return the path to the specified CTI file. If multiple matches are available,
 * only the first is returned. Returns an empty string, on failure. */
GenTL_CTI_string GenTL_CTI::findCtiFile(GenTL_CTI_string const& ctiFileName)
{
    // Find the file, in the standard environment varialbe location.
    gcstring TLPath = GenICam::GetValueOfEnvironmentVariable(GENICAM_GENTL_PATH_VAR_NAME);
    return findFileForPaths(gcstr2ctistr(TLPath), ctiFileName);
}

/* Return a GenTL instance for the specified cti file name, null upon failure.
 * If multiple files with this name are present, only the first found will be loaded. */
GenTL_CTI::Ptr GenTL_CTI::openCtiFile(GenTL_CTI_string const& ctiFileName)
{
    // Open the CTI file.
    return openCtiFilePath (findCtiFile(ctiFileName));
}

GenTL_CTI::GenTL_CTI (GenTL_CTI_string const& gentlPath)
    : m_pd ( *(new PrivateData(gentlPath)) )
{ }

GenTL_CTI::~GenTL_CTI (void)
{
    delete &m_pd;
}

// Return the GenTL system handle.
GenTL_CTI_hSystem GenTL_CTI::system_handle (void)
{
    return m_pd.hCti;
}

// A "hint" string to printed with any portRead log.
std::string GenTL_CTI::portReadLogHint (std::string const& hint)
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);

    std::string oldHint = m_pd.portReadLogHint;
    m_pd.portReadLogHint = hint;
    return oldHint;
}
std::string GenTL_CTI::portReadLogHint (void) const
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);
    return m_pd.portReadLogHint;
}

// A "hint" string to printed with any portWrite log.
std::string GenTL_CTI::portWriteLogHint (std::string const& hint)
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);

    std::string oldHint = m_pd.portWriteLogHint;
    m_pd.portWriteLogHint = hint;
    return oldHint;
}
std::string GenTL_CTI::portWriteLogHint (void) const
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);
    return m_pd.portWriteLogHint;
}

// Enable or disable port read debug logging.
bool GenTL_CTI::portReadDebugLogEnabled (const bool enabled)
{
    return m_pd.portReadDebugLogEnabled.exchange(enabled);
}
bool GenTL_CTI::portReadDebugLogEnabled (void) const
{
    return m_pd.portReadDebugLogEnabled.load();
}

// Enable or disable port write debug logging.
bool GenTL_CTI::portWriteDebugLogEnabled (const bool enabled)
{
    return m_pd.portWriteDebugLogEnabled.exchange(enabled);
}
bool GenTL_CTI::portWriteDebugLogEnabled (void) const
{
    return m_pd.portWriteDebugLogEnabled.load();
}

// Open/close the port read logger.
bool GenTL_CTI::openPortReadFileLog (std::string const& path, const FileOpenMode mode)
{
    std::ios::openmode iosMode = std::ios::out;
    switch (mode)
    {
    case FileAppend:
        iosMode |= std::ios::app;
        break;
    case FileOverwrite:
        iosMode |= std::ios::trunc;
        break;
    default:
        return false;
    }

    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);

    if (!m_pd.portReadLog->good() || path != m_pd.portReadFileLogPath || FileOverwrite == mode)
    {
        PrivateData::put_file_closed(*m_pd.portReadLog, m_pd.portReadFileLogPath, "READ");
        m_pd.portReadLog.reset(new std::ofstream(path, iosMode));
        PrivateData::put_file_opened(*m_pd.portReadLog, path, "READ");
    }

    if (m_pd.portReadLog->good())
        m_pd.portReadFileLogPath = path;
    else
        m_pd.portReadFileLogPath.clear();

    return m_pd.portReadLog->good();
}
void GenTL_CTI::closePortReadFileLog (void)
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);
    
    PrivateData::put_file_closed(*m_pd.portReadLog, m_pd.portReadFileLogPath, "READ");

    m_pd.portReadLog->close();
    m_pd.portReadFileLogPath.clear();
}
std::string GenTL_CTI::portReadFileLogPath (void) const
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);
    return m_pd.portReadFileLogPath;
}

// Open/close the port write logger.
bool GenTL_CTI::openPortWriteFileLog (std::string const& path, const FileOpenMode mode)
{
    std::ios::openmode iosMode = std::ios::out;
    switch (mode)
    {
    case FileAppend:
        iosMode |= std::ios::app;
        break;
    case FileOverwrite:
        iosMode |= std::ios::trunc;
        break;
    default:
        return false;
    }

    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);

    if (!m_pd.portWriteLog->good() || path != m_pd.portWriteFileLogPath || FileOverwrite == mode)
    {
        PrivateData::put_file_closed(*m_pd.portWriteLog, m_pd.portWriteFileLogPath, "WRITE");
        m_pd.portWriteLog.reset(new std::ofstream(path, iosMode));
        PrivateData::put_file_opened(*m_pd.portWriteLog, path, "WRITE");
    }

    if (m_pd.portWriteLog->good())
        m_pd.portWriteFileLogPath = path;
    else
        m_pd.portWriteFileLogPath.clear();

    return m_pd.portWriteLog->good();
}
void GenTL_CTI::closePortWriteFileLog (void)
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);
    
    PrivateData::put_file_closed(*m_pd.portWriteLog, m_pd.portWriteFileLogPath, "WRITE");

    m_pd.portWriteLog->close();
    m_pd.portWriteFileLogPath.clear();
}
std::string GenTL_CTI::portWriteFileLogPath (void) const
{
    const std::lock_guard<std::mutex> lock (m_pd.portLogMutex);
    return m_pd.portWriteFileLogPath;
}

// Block/unblock subsequent read operations, not interrupting ongoing operations. Return the
// status before the call.
bool GenTL_CTI::blockPortReads (const bool block)
{
    return m_pd.blockPortReads.exchange(block);
}
bool GenTL_CTI::blockPortReads (void) const
{
    return m_pd.blockPortReads.load();
}
    
// Block/unblock subsequent write operations, not interrupting ongoing operations. Return the
// status before the call.
bool GenTL_CTI::blockPortWrites (const bool block)
{
    return m_pd.blockPortWrites.exchange(block);
}
bool GenTL_CTI::blockPortWrites (void) const
{
    return m_pd.blockPortWrites.load();
}

// Thread-safe raise of the Cancel-Next flag. With the flag raised (true), the next
// time a GenTL_CTI GenTL wrapper function is invoked, it will throw an
// OperationCanceled exception, rather than executing the function. Invoking with
// cancel = false will revoke the cancelation, if any. Returns the prior state of
// the cancelation flag. When a cancel has been executed, the cancel flag is
// automatically lowered (set false).
bool GenTL_CTI::cancelNext (const bool cancel)
{
    return m_pd.cancelNext.exchange(cancel);
}
bool GenTL_CTI::cancelNext (void) const
{
    return m_pd.cancelNext.load();
}

// Cancel now, if the Cancel-Next flag is raised. On cancel, lower the Cancel-Now
// flag, and throw an OperationCanceled flag. NoOp, if Cancel-Now isn't raised.
void GenTL_CTI::cancelNow (std::string const& canceledAt)
{
    bool cancelIfCanceled = true;
    if (m_pd.cancelNext.compare_exchange_strong(cancelIfCanceled, false))
        throw GenTL_CTI::OperationCanceled( ("Operation canceled at the invocation of " + canceledAt + ".").c_str() );
}

// Enable/disable internal GenICam error recording. Return the prior state.
bool GenTL_CTI::recordErrors (bool recordErrors)
{
    std::swap (m_pd.recordErrors, recordErrors);
    return recordErrors;
}

GC_ERROR GenTL_CTI::lastError (void) const
{   return m_pd.lastErrorCode; }
GC_ERROR GenTL_CTI::popLastError (void)
{
    const GC_ERROR err = m_pd.lastErrorCode;
    m_pd.lastErrorCode = GC_ERR_SUCCESS;
    return err;
}

// Private error message handlers.
GC_ERROR GenTL_CTI::setLastError (const GC_ERROR error)
{
    if (m_pd.recordErrors && error < 0)
    {
        std::lock_guard<std::mutex> lock (m_pd.errorAccessMutex);
        m_pd.lastErrorCode = error;
    }
    return error;
}
void GenTL_CTI::clearError (void)
{
    std::lock_guard<std::mutex> lock (m_pd.errorAccessMutex);
    m_pd.lastErrorCode = GC_ERR_SUCCESS;
}

// Macro to automate implementation of the GenTL function loaders.
#if defined(_WIN32)

#   define LOAD_AND_CALL_GENTL_FUNCTION(FUNC,ARGLIST) \
        cancelNow("the " TOSTRING(FUNC) " GenTL function"); \
        if (!m_pd.FUNC) { \
        m_pd.FUNC = (P##FUNC)GetProcAddress(m_pd.hCti, TOSTRING(FUNC)); \
            if (!m_pd.FUNC) \
                throw std::runtime_error ("Unable to load the " TOSTRING(FUNC) " GenTL function from the GenTL library."); \
        } \
        return setLastError( m_pd.FUNC ARGLIST )

#elif defined(__GNUC__)

#   define LOAD_AND_CALL_GENTL_FUNCTION(FUNC,ARGLIST) \
        cancelNow("the " TOSTRING(FUNC) " GenTL function"); \
        if (!m_pd.FUNC) { \
        m_pd.FUNC = (P##FUNC)dlsym(m_pd.hCti, TOSTRING(FUNC)); \
            if (!m_pd.FUNC) \
                throw std::runtime_error(std::string("Unable to load the " TOSTRING(FUNC) " GenTL function from the GenTL library: ").append(dlerror())); \
        } \
        return setLastError( m_pd.FUNC ARGLIST )

#else
#   error Platform implementation missing.
#endif

/* GenTL function wrappers. The underlying GenTL functions will be loaded dynamically, on-demand.
 * If the function cannot be loaded, a std::runtime_error will be thrown. */
GC_ERROR GenTL_CTI::GCGetInfo ( TL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCGetInfo, (iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::GCGetLastError ( GC_ERROR *piErrorCode, char *sErrText, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCGetLastError, (piErrorCode, sErrText, piSize) );
}
GC_ERROR GenTL_CTI::GCInitLib ( void )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCInitLib, () );
}
GC_ERROR GenTL_CTI::GCCloseLib ( void )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCCloseLib, () );
}
GC_ERROR GenTL_CTI::GCReadPort ( PORT_HANDLE hPort, uint64_t iAddress, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCReadPort, (hPort, iAddress, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::GCWritePort ( PORT_HANDLE hPort, uint64_t iAddress, const void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCWritePort, (hPort, iAddress, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::GCGetPortURL ( PORT_HANDLE hPort, char *sURL, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCGetPortURL, (hPort, sURL, piSize) );
}
GC_ERROR GenTL_CTI::GCGetPortInfo ( PORT_HANDLE hPort, PORT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCGetPortInfo, (hPort, iInfoCmd, piType, pBuffer, piSize) );
}

GC_ERROR GenTL_CTI::GCRegisterEvent ( EVENTSRC_HANDLE hEventSrc, EVENT_TYPE iEventID, EVENT_HANDLE *phEvent )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCRegisterEvent, (hEventSrc, iEventID, phEvent) );
}
GC_ERROR GenTL_CTI::GCUnregisterEvent ( EVENTSRC_HANDLE hEventSrc, EVENT_TYPE iEventID )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCUnregisterEvent, (hEventSrc, iEventID) );
}
GC_ERROR GenTL_CTI::EventGetData ( EVENT_HANDLE hEvent, void *pBuffer, size_t *piSize, uint64_t iTimeout )
{
    LOAD_AND_CALL_GENTL_FUNCTION( EventGetData, (hEvent, pBuffer, piSize, iTimeout) );
}
GC_ERROR GenTL_CTI::EventGetDataInfo ( EVENT_HANDLE hEvent, const void *pInBuffer, size_t iInSize, EVENT_DATA_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pOutBuffer, size_t *piOutSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( EventGetDataInfo, (hEvent, pInBuffer, iInSize, iInfoCmd, piType, pOutBuffer, piOutSize) );
}
GC_ERROR GenTL_CTI::EventGetInfo ( EVENT_HANDLE hEvent, EVENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( EventGetInfo, (hEvent, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::EventFlush ( EVENT_HANDLE hEvent )
{
    LOAD_AND_CALL_GENTL_FUNCTION( EventFlush, (hEvent) );
}
GC_ERROR GenTL_CTI::EventKill ( EVENT_HANDLE hEvent )
{
    LOAD_AND_CALL_GENTL_FUNCTION( EventKill, (hEvent) );
}
GC_ERROR GenTL_CTI::TLOpen ( TL_HANDLE *phTL )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLOpen, (phTL) );
}
GC_ERROR GenTL_CTI::TLClose ( TL_HANDLE hTL )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLClose, (hTL) );
}
GC_ERROR GenTL_CTI::TLGetInfo ( TL_HANDLE hTL, TL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLGetInfo, (hTL, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::TLGetNumInterfaces ( TL_HANDLE hTL, uint32_t *piNumIfaces )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLGetNumInterfaces, (hTL, piNumIfaces) );
}
GC_ERROR GenTL_CTI::TLGetInterfaceID ( TL_HANDLE hTL, uint32_t iIndex, char *sID, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLGetInterfaceID, (hTL, iIndex, sID, piSize) );
}
GC_ERROR GenTL_CTI::TLGetInterfaceInfo ( TL_HANDLE hTL, const char *sIfaceID, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLGetInterfaceInfo, (hTL, sIfaceID, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::TLOpenInterface ( TL_HANDLE hTL, const char *sIfaceID, IF_HANDLE *phIface )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLOpenInterface, (hTL, sIfaceID, phIface) );
}
GC_ERROR GenTL_CTI::TLUpdateInterfaceList ( TL_HANDLE hTL, bool8_t *pbChanged, uint64_t iTimeout )
{
    LOAD_AND_CALL_GENTL_FUNCTION( TLUpdateInterfaceList, (hTL, pbChanged, iTimeout) );
}
GC_ERROR GenTL_CTI::IFClose ( IF_HANDLE hIface )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFClose, (hIface) );
}
GC_ERROR GenTL_CTI::IFGetInfo ( IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFGetInfo, (hIface, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::IFGetNumDevices ( IF_HANDLE hIface, uint32_t *piNumDevices )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFGetNumDevices, (hIface, piNumDevices) );
}
GC_ERROR GenTL_CTI::IFGetDeviceID ( IF_HANDLE hIface, uint32_t iIndex, char *sIDeviceID, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFGetDeviceID, (hIface, iIndex, sIDeviceID, piSize) );
}
GC_ERROR GenTL_CTI::IFUpdateDeviceList ( IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFUpdateDeviceList, (hIface, pbChanged, iTimeout) );
}
GC_ERROR GenTL_CTI::IFGetDeviceInfo ( IF_HANDLE hIface, const char *sDeviceID, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFGetDeviceInfo, (hIface, sDeviceID, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::IFOpenDevice ( IF_HANDLE hIface, const char *sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlags, DEV_HANDLE *phDevice )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFOpenDevice, (hIface, sDeviceID, iOpenFlags, phDevice) );
}
GC_ERROR GenTL_CTI::DevGetPort ( DEV_HANDLE hDevice, PORT_HANDLE *phRemoteDevice )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevGetPort, (hDevice, phRemoteDevice) );
}
GC_ERROR GenTL_CTI::DevGetNumDataStreams ( DEV_HANDLE hDevice, uint32_t *piNumDataStreams )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevGetNumDataStreams, (hDevice, piNumDataStreams) );
}
GC_ERROR GenTL_CTI::DevGetDataStreamID ( DEV_HANDLE hDevice, uint32_t iIndex, char *sDataStreamID, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevGetDataStreamID, (hDevice, iIndex, sDataStreamID, piSize) );
}
GC_ERROR GenTL_CTI::DevOpenDataStream ( DEV_HANDLE hDevice, const char *sDataStreamID, DS_HANDLE *phDataStream )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevOpenDataStream, (hDevice, sDataStreamID, phDataStream) );
}
GC_ERROR GenTL_CTI::DevGetInfo ( DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevGetInfo, (hDevice, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::DevClose ( DEV_HANDLE hDevice )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevClose, (hDevice) );
}
GC_ERROR GenTL_CTI::DSAnnounceBuffer ( DS_HANDLE hDataStream, void *pBuffer, size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSAnnounceBuffer, (hDataStream, pBuffer, iSize, pPrivate, phBuffer) );
}
GC_ERROR GenTL_CTI::DSAllocAndAnnounceBuffer ( DS_HANDLE hDataStream, size_t iSize, void *pPrivate, BUFFER_HANDLE *phBuffer )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSAllocAndAnnounceBuffer, (hDataStream, iSize, pPrivate, phBuffer) );
}
GC_ERROR GenTL_CTI::DSFlushQueue ( DS_HANDLE hDataStream, ACQ_QUEUE_TYPE iOperation )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSFlushQueue, (hDataStream, iOperation) );
}
GC_ERROR GenTL_CTI::DSStartAcquisition ( DS_HANDLE hDataStream, ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSStartAcquisition, (hDataStream, iStartFlags, iNumToAcquire) );
}
GC_ERROR GenTL_CTI::DSStopAcquisition ( DS_HANDLE hDataStream, ACQ_STOP_FLAGS iStopFlags )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSStopAcquisition, (hDataStream, iStopFlags) );
}
GC_ERROR GenTL_CTI::DSGetInfo ( DS_HANDLE hDataStream, STREAM_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetInfo, (hDataStream, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::DSGetBufferID ( DS_HANDLE hDataStream, uint32_t iIndex, BUFFER_HANDLE *phBuffer )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetBufferID, (hDataStream, iIndex, phBuffer) );
}
GC_ERROR GenTL_CTI::DSClose ( DS_HANDLE hDataStream )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSClose, (hDataStream) );
}
GC_ERROR GenTL_CTI::DSRevokeBuffer ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, void **pBuffer, void **pPrivate )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSRevokeBuffer, (hDataStream, hBuffer, pBuffer, pPrivate) );
}
GC_ERROR GenTL_CTI::DSQueueBuffer ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSQueueBuffer, (hDataStream, hBuffer) );
}
GC_ERROR GenTL_CTI::DSGetBufferInfo ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, BUFFER_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetBufferInfo, (hDataStream, hBuffer, iInfoCmd, piType, pBuffer, piSize) );
}

#if GENTL_H_AT_LEAST_V(1,1,0)

GC_ERROR GenTL_CTI::GCGetNumPortURLs ( PORT_HANDLE hPort, uint32_t *iNumURLs )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCGetNumPortURLs, (hPort, iNumURLs) );
}
GC_ERROR GenTL_CTI::GCGetPortURLInfo ( PORT_HANDLE hPort, uint32_t iURLIndex, URL_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCGetPortURLInfo, (hPort, iURLIndex, iInfoCmd, piType, pBuffer, piSize) );
}
GC_ERROR GenTL_CTI::GCReadPortStacked ( PORT_HANDLE hPort, GenTL::PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCReadPortStacked, (hPort, pEntries, piNumEntries) );
}
GC_ERROR GenTL_CTI::GCWritePortStacked ( PORT_HANDLE hPort, GenTL::PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries )
{
    LOAD_AND_CALL_GENTL_FUNCTION( GCWritePortStacked, (hPort, pEntries, piNumEntries) );
}

#endif

#if GENTL_H_AT_LEAST_V(1,3,0)

GC_ERROR GenTL_CTI::DSGetBufferChunkData ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, SINGLE_CHUNK_DATA *pChunkData, size_t *piNumChunks )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetBufferChunkData, (hDataStream, hBuffer, pChunkData, piNumChunks) );
}

#endif

#if GENTL_H_AT_LEAST_V(1,4,0)

GC_ERROR GenTL_CTI::IFGetParentTL ( IF_HANDLE hIface, TL_HANDLE *phSystem )
{
    LOAD_AND_CALL_GENTL_FUNCTION( IFGetParentTL, (hIface, phSystem) );
}
GC_ERROR GenTL_CTI::DevGetParentIF ( DEV_HANDLE hDevice, IF_HANDLE *phIface )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DevGetParentIF, (hDevice, phIface) );
}
GC_ERROR GenTL_CTI::DSGetParentDev ( DS_HANDLE hDataStream, DEV_HANDLE *phDevice )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetParentDev, (hDataStream, phDevice) );
}

#endif

#if GENTL_H_AT_LEAST_V(1,5,0)

GC_ERROR GenTL_CTI::DSGetNumBufferParts ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumParts )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetNumBufferParts, (hDataStream, hBuffer, piNumParts) );
}
GC_ERROR GenTL_CTI::DSGetBufferPartInfo ( DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iPartIndex, BUFFER_PART_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize )
{
    LOAD_AND_CALL_GENTL_FUNCTION( DSGetBufferPartInfo, (hDataStream, hBuffer, iPartIndex, iInfoCmd, piType, pBuffer, piSize) );
}

#endif
