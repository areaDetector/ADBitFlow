/* FILE:        GenICamUtilities_Private.h
 * DATE:        1/5/2017
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2017, BitFlow, Inc.
 * DESCRIPTION: Private details of the GenTL_CTI class.
 */

#ifndef INCLUDED__GEN__I__CAM__UTILITIES_____PRIVATE__H
#define INCLUDED__GEN__I__CAM__UTILITIES_____PRIVATE__H

#include "GenICamUtilities.h"

#include <cstdint>
#include <atomic>
#include <mutex>

#include <string>
#include <fstream>
#include <memory>

#if defined(ENABLE_BFOUTPUTDEBUGSTRINGEX)
#   if defined(_WIN32)
#       define USE_BFOUTPOUTDEBUGSTRINGEX 1
#       include <BFApi.h>
        typedef decltype(&BFOutputDebugStringEx) PBFOutputDebugStringEx;
#   elif defined(__GNUC__)
#       define USE_BFOUTPOUTDEBUGSTRINGEX 1
#       include <BFLogClientApi.h>
        typedef decltype(BFLogClientMessagePush) *PBFOutputDebugStringEx;
#   else
#       error Platform implementation missing.
#   endif
#   else
#       define USE_BFOUTPOUTDEBUGSTRINGEX 0
#endif

using namespace GenApi;
using namespace GenICam;

#if GENTL_H_AT_LEAST_V(1,5,0)
using namespace GenTL;
#else
using namespace GenICam::Client;
#endif

struct GenTL_CTI::PrivateData
{
    typedef std::unique_ptr<std::ofstream> OFStreamPtr;

    PrivateData (GenTL_CTI_string const& gentlPath);
    ~PrivateData (void);
    
    // Helper methods.
    static std::string  cdata_format    (std::string const& cdata);
    static char         byte_to_char    (const std::uint8_t val);
    static void         put_time        (std::ostream &strm, const size_t indent);
    static void         put_reg_op      (std::ostream &strm, const size_t indent, const std::uint8_t *const pBuf, const int64_t Address, const int64_t Length);
    static void         put_file_opened (std::ostream &strm, std::string const& path, std::string const& portOpType);
    static void         put_file_closed (std::ostream &strm, std::string const& path, std::string const& portOpType);
    static void         put_port_op     (std::ostream &strm, std::string const& opType, std::string const& portName, std::string const& hint, const GC_ERROR err, const void *const pBuffer, const int64_t Address, const int64_t Length);
    void                log_port_read   (std::string const& portName, const GC_ERROR err, const void *const pBuffer, const int64_t Address, const int64_t Length);
    void                log_port_write  (std::string const& portName, const GC_ERROR err, const void *const pBuffer, const int64_t Address, const int64_t Length);

    // The CTI file handle;
    GenTL_CTI_hSystem           hCti;

    // GenTL function pointers.
    PGCGetInfo                  GCGetInfo;
    PGCGetLastError             GCGetLastError;
    PGCInitLib                  GCInitLib;
    PGCCloseLib                 GCCloseLib;
    PGCReadPort                 GCReadPort;
    PGCWritePort                GCWritePort;
    PGCGetPortURL               GCGetPortURL;
    PGCGetPortInfo              GCGetPortInfo;

    PGCRegisterEvent            GCRegisterEvent;
    PGCUnregisterEvent          GCUnregisterEvent;
    PEventGetData               EventGetData;
    PEventGetDataInfo           EventGetDataInfo;
    PEventGetInfo               EventGetInfo;
    PEventFlush                 EventFlush;
    PEventKill                  EventKill;
    PTLOpen                     TLOpen;
    PTLClose                    TLClose;
    PTLGetInfo                  TLGetInfo;
    PTLGetNumInterfaces         TLGetNumInterfaces;
    PTLGetInterfaceID           TLGetInterfaceID;
    PTLGetInterfaceInfo         TLGetInterfaceInfo;
    PTLOpenInterface            TLOpenInterface;
    PTLUpdateInterfaceList      TLUpdateInterfaceList;
    PIFClose                    IFClose;
    PIFGetInfo                  IFGetInfo;
    PIFGetNumDevices            IFGetNumDevices;
    PIFGetDeviceID              IFGetDeviceID;
    PIFUpdateDeviceList         IFUpdateDeviceList;
    PIFGetDeviceInfo            IFGetDeviceInfo;
    PIFOpenDevice               IFOpenDevice;
    PDevGetPort                 DevGetPort;
    PDevGetNumDataStreams       DevGetNumDataStreams;
    PDevGetDataStreamID         DevGetDataStreamID;
    PDevOpenDataStream          DevOpenDataStream;
    PDevGetInfo                 DevGetInfo;
    PDevClose                   DevClose;
    PDSAnnounceBuffer           DSAnnounceBuffer;
    PDSAllocAndAnnounceBuffer   DSAllocAndAnnounceBuffer;
    PDSFlushQueue               DSFlushQueue;
    PDSStartAcquisition         DSStartAcquisition;
    PDSStopAcquisition          DSStopAcquisition;
    PDSGetInfo                  DSGetInfo;
    PDSGetBufferID              DSGetBufferID;
    PDSClose                    DSClose;
    PDSRevokeBuffer             DSRevokeBuffer;
    PDSQueueBuffer              DSQueueBuffer;
    PDSGetBufferInfo            DSGetBufferInfo;
    
#if GENTL_H_AT_LEAST_V(1,1,0)
    PGCGetNumPortURLs           GCGetNumPortURLs;
    PGCGetPortURLInfo           GCGetPortURLInfo;
    PGCReadPortStacked          GCReadPortStacked;
    PGCWritePortStacked         GCWritePortStacked;
#endif
    
#if GENTL_H_AT_LEAST_V(1,3,0)
    PDSGetBufferChunkData       DSGetBufferChunkData;
#endif
    
#if GENTL_H_AT_LEAST_V(1,4,0)
    PIFGetParentTL              IFGetParentTL;
    PDevGetParentIF             DevGetParentIF;
    PDSGetParentDev             DSGetParentDev;
#endif
    
#if GENTL_H_AT_LEAST_V(1,5,0)
    PDSGetNumBufferParts        DSGetNumBufferParts;
    PDSGetBufferPartInfo        DSGetBufferPartInfo;
#endif

    // Additional variables.
    GC_ERROR                    lastErrorCode;
    bool                        recordErrors;
    std::mutex                  errorAccessMutex;

    std::atomic<bool>           blockPortReads;
    std::atomic<bool>           blockPortWrites;
    std::atomic<bool>           cancelNext;

    std::string                 portReadLogHint;
    std::atomic<bool>           portReadDebugLogEnabled;
    OFStreamPtr                 portReadLog;
    std::string                 portReadFileLogPath;

    std::string                 portWriteLogHint;
    std::atomic<bool>           portWriteDebugLogEnabled;
    OFStreamPtr                 portWriteLog;
    std::string                 portWriteFileLogPath;

#if USE_BFOUTPOUTDEBUGSTRINGEX
    PBFOutputDebugStringEx      bfOutputDebugStringEx;
#endif

    mutable std::mutex          portLogMutex;
};


#endif // INCLUDED__GEN__I__CAM__UTILITIES_____PRIVATE__H
