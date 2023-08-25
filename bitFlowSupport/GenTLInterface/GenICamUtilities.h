/* FILE:        GenICamUtilities.h
 * DATE:        6/19/2012, 3/6/2013
 * AUTHOR:      Jeremy Greene
 * COPYRIGHT:   BitFlow, Inc., 2012-2013
 * DESCRIPTION: Header file declaring the GenTL_CTI class, which may be used to locate and load GenTL
 *              producer CTI files, call the standard GenTL.h functions of the producer, and perform
 *              a few other common operations. This should be significantly simpler than using the
 *              GenTL C-interface directly.
 */

#ifndef INCLUDED__GEN_I_CAM_UTILITIES_H
#define INCLUDED__GEN_I_CAM_UTILITIES_H

#include <string>
#include <vector>
#include <memory>

#include <Base/GCUtilities.h>
#include <GenApi/GenApi.h>

#define GENTL_H_AT_LEAST_V(MAJOR,MINOR,SUBMINOR) (GC_GENTL_HEADER_VERSION >= GC_GENTL_HEADER_VERSION_CODE(MAJOR,MINOR,SUBMINOR))

#include <BFResolveGenTL.h>

#if defined(_WIN32)

#   ifdef DO_BF_GENICAM_UTILTIES_EXPORT
#       define BF_GENICAM_UTILTIES_IMPORT_EXPORT __declspec(dllexport)
#   else
#       define BF_GENICAM_UTILTIES_IMPORT_EXPORT
#   endif
#   define BF_GENICAM_UTILTIES_HIDDEN

#   define BF_GENICAM_UTILTIES_TEXT(T) L##T
#   define BF_GENICAM_UTILTIES_PATH_SEP L"\\"
#   define BF_GENICAM_UTILTIES_CHAR wchar_t

typedef std::wstring GenTL_CTI_string;
typedef HMODULE GenTL_CTI_hSystem;

#elif defined(__GNUC__)

#   ifdef DO_BF_GENICAM_UTILTIES_EXPORT
#       define BF_GENICAM_UTILTIES_IMPORT_EXPORT __attribute__ ((visibility("default")))
#   else
#       define BF_GENICAM_UTILTIES_IMPORT_EXPORT
#   endif
#   define BF_GENICAM_UTILTIES_HIDDEN __attribute__ ((visibility("hidden")))

#   define BF_GENICAM_UTILTIES_TEXT(T) T
#   define BF_GENICAM_UTILTIES_PATH_SEP "/"
#   define BF_GENICAM_UTILTIES_CHAR char

typedef std::string GenTL_CTI_string;
typedef void* GenTL_CTI_hSystem;

#else
#   error Local platform implementation missing.
#endif

#if defined(_WIN32)
#   if _MSC_VER >= 1700
#       define BF_GENICAM_UTILITIES_OVERRIDE override
#       define BF_GENICAM_UTILITIES_FINAL final
#   else
#       define BF_GENICAM_UTILITIES_OVERRIDE
#       define BF_GENICAM_UTILITIES_FINAL
#   endif
#   if _MSC_VER >= 1900
#       define BF_GENICAM_UTILITIES_NOEXCEPT noexcept
#   else
#       define BF_GENICAM_UTILITIES_NOEXCEPT
#   endif
#elif defined(__GNUC__)
#   if __GNUC__ >= 5 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 7)
#       define BF_GENICAM_UTILITIES_OVERRIDE override
#       define BF_GENICAM_UTILITIES_FINAL final
#   else
#       define BF_GENICAM_UTILITIES_OVERRIDE
#       define BF_GENICAM_UTILITIES_FINAL
#   endif
#   if __GNUC__ >= 5 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#       define BF_GENICAM_UTILITIES_NOEXCEPT noexcept
#   else
#       define BF_GENICAM_UTILITIES_NOEXCEPT
#   endif
#else
#   error noexcept, final, and override support unknown for this platform.
#endif

/* The name of the GenTL environment variable, for the current platform. */
#define GENICAM_GENTL64_PATH_VAR_NAME "GENICAM_GENTL64_PATH"
#define GENICAM_GENTL32_PATH_VAR_NAME "GENICAM_GENTL32_PATH"

#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#   define GENICAM_GENTL_PATH_VAR_NAME GENICAM_GENTL64_PATH_VAR_NAME
#elif defined(_WIN32) || defined(__i386__)
#   define GENICAM_GENTL_PATH_VAR_NAME GENICAM_GENTL32_PATH_VAR_NAME
#else
#   error The GenTL environment variable name has not been defined for this platform.
#endif

#if defined(_WIN32)
#   define GENICAM_GENTL_PATH_SEP BF_GENICAM_UTILTIES_TEXT(';')
#   define GCUErr(msg) OutputDebugStringA(msg)
#elif defined(__GNUC__)
#   define GENICAM_GENTL_PATH_SEP BF_GENICAM_UTILTIES_TEXT(':')
#   define GCUErr(msg) std::cerr << msg << "\n"
#else
#   error Platform implementation missing.
#endif

// Helper method to convert from gcstring to std::string.
inline static std::string gcstr2str (GenICam::gcstring const& a_str)
{
    return a_str.c_str();
}

// Helper method to convert from gcstring to std::wstring.
inline static GenTL_CTI_string gcstr2ctistr (GenICam::gcstring const& a_str)
{
#if defined(_WIN32)
#   if GENICAM_VERSION_MAJOR >= 3
    return GenTL_CTI_string(a_str.w_str().c_str());
#   else
    return GenTL_CTI_string(a_str.w_str());
#   endif
#elif defined(__GNUC__)
    return a_str.c_str();
#else
#   error Local platform implementation missing.
#endif
}

// Helper method to convert from a GenTL_CTI_string to a std::string.
inline static std::string ctistr2str (GenTL_CTI_string const& a_str)
{
#if defined(_WIN32)
    const GenICam::gcstring gcstr (a_str.c_str());
    return gcstr.c_str();
#elif defined(__GNUC__)
    return a_str;
#else
#   error Local platform implementation missing.
#endif
}

//// Declaration of the GenTL_CTI class. ////
class BF_GENICAM_UTILTIES_IMPORT_EXPORT GenTL_CTI
{
    GenTL_CTI (void);
    GenTL_CTI (GenTL_CTI_string const& gentlPath);

    // Private error message handlers.
    GenTL::GC_ERROR setLastError   (const GenTL::GC_ERROR error);
    void clearError         (void);

    // Private data structure and instance pointer.
    struct PrivateData;
    PrivateData &m_pd;

public:
    // Custom implementation of the GenICam IPort interface.
    class GCPort;

    // Public interface types.
    typedef std::shared_ptr<GenTL_CTI>              Ptr;
    typedef std::shared_ptr<GenApi::CNodeMapRef>    NodeMapPtr;
    typedef std::shared_ptr<GCPort>                 PortPtr;

    enum FileOpenMode : unsigned int
    {
        FileOverwrite = 0,
        FileAppend = 1
    };

private:
    // Private helper methods.
    PortPtr             makePort            (GenTL::PORT_HANDLE hPort);
    GenTL_CTI_string    retrieveXml         (GenTL::PORT_HANDLE hPort, NodeMapPtr const& pDeviceMap, const unsigned int xmlIndex);
    void                getNodeMap          (GenTL::PORT_HANDLE hPort, NodeMapPtr const& pDeviceMap, GenApi::IPort *const portPtr, const unsigned int xmlIndex = 0);

public:
    // Exception thrown if an operation is canceled due to a call to "cancelRunning".
    struct OperationCanceled : public std::exception
    {
        const std::string m_what;

        OperationCanceled (const char *const reason) : m_what (reason) { }

        ~OperationCanceled (void) BF_GENICAM_UTILITIES_NOEXCEPT { }

        inline const char* what (void) const BF_GENICAM_UTILITIES_NOEXCEPT BF_GENICAM_UTILITIES_OVERRIDE { return m_what.c_str(); }
    };

    ~GenTL_CTI (void);

    // Return the GenTL system handle.
    GenTL_CTI_hSystem   system_handle               (void);

    // A "hint" string to printed with any portRead log.
    std::string         portReadLogHint             (std::string const& hint);
    std::string         portReadLogHint             (void) const;

    // A "hint" string to printed with any portWrite log.
    std::string         portWriteLogHint            (std::string const& hint);
    std::string         portWriteLogHint            (void) const;

    // Enable or disable port read debug logging.
    bool                portReadDebugLogEnabled     (const bool enabled);
    bool                portReadDebugLogEnabled     (void) const;

    // Enable or disable port write debug logging.
    bool                portWriteDebugLogEnabled    (const bool enabled);
    bool                portWriteDebugLogEnabled    (void) const;

    // Open/close the port read logger.
    bool                openPortReadFileLog         (std::string const& path, const FileOpenMode mode);
    void                closePortReadFileLog        (void);
    std::string         portReadFileLogPath         (void) const;

    // Open/close the port write logger.
    bool                openPortWriteFileLog        (std::string const& path, const FileOpenMode mode);
    void                closePortWriteFileLog       (void);
    std::string         portWriteFileLogPath        (void) const;

    // Block/unblock subsequent read operations, not interrupting ongoing operations.
    // Return the block status before the call.
    bool                blockPortReads              (const bool block);
    bool                blockPortReads              (void) const;
    
    // Block/unblock subsequent write operations, not interrupting ongoing operations.
    // Return the block status before the call.
    bool                blockPortWrites             (const bool block);
    bool                blockPortWrites             (void) const;

    // Thread-safe raise of the Cancel-Next flag. With the flag raised (true), the next
    // time a GenTL_CTI GenTL wrapper function is invoked, it will throw an
    // OperationCanceled exception, rather than executing the function. Invoking with
    // cancel = false will revoke the cancelation, if any. Returns the prior state of
    // the cancelation flag. When a cancel has been executed, the cancel flag is
    // automatically lowered (set false).
    bool                cancelNext                  (const bool cancel);
    bool                cancelNext                  (void) const;
    
    // Cancel now, if the Cancel-Next flag is raised. On cancel, lower the Cancel-Now
    // flag, and throw an OperationCanceled flag. NoOp, if Cancel-Now isn't raised.
    void                cancelNow                   (std::string const& canceledAt);

    // Enable/disable internal GenICam error recording. Return the prior state.
    bool                recordErrors                (bool recordErrors);

    GenTL::GC_ERROR     lastError                   (void) const;
    GenTL::GC_ERROR     popLastError                (void);

    // Take a semicolon list of paths (pathsList), and search each path for the file named by
    // fileName. If a matching file is found, return its complete path as a string. Otherwise,
    // return an empty string.
    static GenTL_CTI_string findFileForPaths(GenTL_CTI_string const& pathsList, GenTL_CTI_string const& fileName);

    // Search all paths in the provided paths list (semicolon separated) for files whos names
    // match the provided pattern, which should be an ECMA-script style regular expression.
    // Fill fileList with each matching path, and return the number of paths found. */
    static size_t       findAllMatchingFiles(GenTL_CTI_string const& pathsList, GenTL_CTI_string const& pattern, std::vector<GenTL_CTI_string> &fileList);

    // Fill the provided list with paths to all available GenTL CTI files on this machine.
    static size_t       findCtiFiles                (std::vector<GenTL_CTI_string> &fileList);

    // Return a GenTL instance for the specified cti file path, null upon failure.
    static Ptr          openCtiFilePath             (GenTL_CTI_string const& ctiFilePath);

    // Find and return the path to the specified CTI file. If multiple matches are available,
    // only the first is returned. Returns an empty string, on failure.
    static GenTL_CTI_string findCtiFile             (GenTL_CTI_string const& ctiFileName);

    // Return a GenTL instance for the specified cti file name, null upon failure.
    // If multiple files with this name are present, only the first found will be loaded.
    static Ptr          openCtiFile                 (GenTL_CTI_string const& ctiFileName);

    //// GenTL function wrappers. The underlying GenTL functions will be loaded dynamically, on-demand. ////
    //// If the function cannot be loaded, a std::runtime_error will be thrown.                         ////
    GenTL::GC_ERROR     GCGetInfo                   (GenTL::TL_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     GCGetLastError              ( GenTL::GC_ERROR *piErrorCode, char *sErrText, size_t *piSize );
    GenTL::GC_ERROR     GCInitLib                   ( void );
    GenTL::GC_ERROR     GCCloseLib                  ( void );
    GenTL::GC_ERROR     GCReadPort                  ( GenTL::PORT_HANDLE hPort, uint64_t iAddress, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     GCWritePort                 ( GenTL::PORT_HANDLE hPort, uint64_t iAddress, const void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     GCGetPortURL                ( GenTL::PORT_HANDLE hPort, char *sURL, size_t *piSize );
    GenTL::GC_ERROR     GCGetPortInfo               ( GenTL::PORT_HANDLE hPort, GenTL::PORT_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );

    GenTL::GC_ERROR     GCRegisterEvent             ( GenTL::EVENTSRC_HANDLE hEventSrc, GenTL::EVENT_TYPE iEventID, GenTL::EVENT_HANDLE *phEvent );
    GenTL::GC_ERROR     GCUnregisterEvent           ( GenTL::EVENTSRC_HANDLE hEventSrc, GenTL::EVENT_TYPE iEventID );
    GenTL::GC_ERROR     EventGetData                ( GenTL::EVENT_HANDLE hEvent, void *pBuffer, size_t *piSize, uint64_t iTimeout );
    GenTL::GC_ERROR     EventGetDataInfo            ( GenTL::EVENT_HANDLE hEvent, const void *pInBuffer, size_t iInSize, GenTL::EVENT_DATA_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pOutBuffer, size_t *piOutSize );
    GenTL::GC_ERROR     EventGetInfo                ( GenTL::EVENT_HANDLE hEvent, GenTL::EVENT_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     EventFlush                  ( GenTL::EVENT_HANDLE hEvent );
    GenTL::GC_ERROR     EventKill                   ( GenTL::EVENT_HANDLE hEvent );
    GenTL::GC_ERROR     TLOpen                      ( GenTL::TL_HANDLE *phTL );
    GenTL::GC_ERROR     TLClose                     ( GenTL::TL_HANDLE hTL );
    GenTL::GC_ERROR     TLGetInfo                   ( GenTL::TL_HANDLE hTL, GenTL::TL_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     TLGetNumInterfaces          ( GenTL::TL_HANDLE hTL, uint32_t *piNumIfaces );
    GenTL::GC_ERROR     TLGetInterfaceID            ( GenTL::TL_HANDLE hTL, uint32_t iIndex, char *sID, size_t *piSize );
    GenTL::GC_ERROR     TLGetInterfaceInfo          ( GenTL::TL_HANDLE hTL, const char *sIfaceID, GenTL::INTERFACE_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     TLOpenInterface             ( GenTL::TL_HANDLE hTL, const char *sIfaceID, GenTL::IF_HANDLE *phIface );
    GenTL::GC_ERROR     TLUpdateInterfaceList       ( GenTL::TL_HANDLE hTL, bool8_t *pbChanged, uint64_t iTimeout );
    GenTL::GC_ERROR     IFClose                     ( GenTL::IF_HANDLE hIface );
    GenTL::GC_ERROR     IFGetInfo                   ( GenTL::IF_HANDLE hIface, GenTL::INTERFACE_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     IFGetNumDevices             ( GenTL::IF_HANDLE hIface, uint32_t *piNumDevices );
    GenTL::GC_ERROR     IFGetDeviceID               ( GenTL::IF_HANDLE hIface, uint32_t iIndex, char *sIDeviceID, size_t *piSize );
    GenTL::GC_ERROR     IFUpdateDeviceList          ( GenTL::IF_HANDLE hIface, bool8_t *pbChanged, uint64_t iTimeout );
    GenTL::GC_ERROR     IFGetDeviceInfo             ( GenTL::IF_HANDLE hIface, const char *sDeviceID, GenTL::DEVICE_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     IFOpenDevice                ( GenTL::IF_HANDLE hIface, const char *sDeviceID, GenTL::DEVICE_ACCESS_FLAGS iOpenFlags, GenTL::DEV_HANDLE *phDevice );
    GenTL::GC_ERROR     DevGetPort                  ( GenTL::DEV_HANDLE hDevice, GenTL::PORT_HANDLE *phRemoteDevice );
    GenTL::GC_ERROR     DevGetNumDataStreams        ( GenTL::DEV_HANDLE hDevice, uint32_t *piNumDataStreams );
    GenTL::GC_ERROR     DevGetDataStreamID          ( GenTL::DEV_HANDLE hDevice, uint32_t iIndex, char *sDataStreamID, size_t *piSize );
    GenTL::GC_ERROR     DevOpenDataStream           ( GenTL::DEV_HANDLE hDevice, const char *sDataStreamID, GenTL::DS_HANDLE *phDataStream );
    GenTL::GC_ERROR     DevGetInfo                  ( GenTL::DEV_HANDLE hDevice, GenTL::DEVICE_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     DevClose                    ( GenTL::DEV_HANDLE hDevice );
    GenTL::GC_ERROR     DSAnnounceBuffer            ( GenTL::DS_HANDLE hDataStream, void *pBuffer, size_t iSize, void *pPrivate, GenTL::BUFFER_HANDLE *phBuffer );
    GenTL::GC_ERROR     DSAllocAndAnnounceBuffer    ( GenTL::DS_HANDLE hDataStream, size_t iSize, void *pPrivate, GenTL::BUFFER_HANDLE *phBuffer );
    GenTL::GC_ERROR     DSFlushQueue                ( GenTL::DS_HANDLE hDataStream, GenTL::ACQ_QUEUE_TYPE iOperation );
    GenTL::GC_ERROR     DSStartAcquisition          ( GenTL::DS_HANDLE hDataStream, GenTL::ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire );
    GenTL::GC_ERROR     DSStopAcquisition           ( GenTL::DS_HANDLE hDataStream, GenTL::ACQ_STOP_FLAGS iStopFlags );
    GenTL::GC_ERROR     DSGetInfo                   ( GenTL::DS_HANDLE hDataStream, GenTL::STREAM_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     DSGetBufferID               ( GenTL::DS_HANDLE hDataStream, uint32_t iIndex, GenTL::BUFFER_HANDLE *phBuffer );
    GenTL::GC_ERROR     DSClose                     ( GenTL::DS_HANDLE hDataStream );
    GenTL::GC_ERROR     DSRevokeBuffer              ( GenTL::DS_HANDLE hDataStream, GenTL::BUFFER_HANDLE hBuffer, void **pBuffer, void **pPrivate );
    GenTL::GC_ERROR     DSQueueBuffer               ( GenTL::DS_HANDLE hDataStream, GenTL::BUFFER_HANDLE hBuffer );
    GenTL::GC_ERROR     DSGetBufferInfo             ( GenTL::DS_HANDLE hDataStream, GenTL::BUFFER_HANDLE hBuffer, GenTL::BUFFER_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );

#if GENTL_H_AT_LEAST_V(1,1,0)
    GenTL::GC_ERROR     GCGetNumPortURLs            ( GenTL::PORT_HANDLE hPort, uint32_t *iNumURLs );
    GenTL::GC_ERROR     GCGetPortURLInfo            ( GenTL::PORT_HANDLE hPort, uint32_t iURLIndex, GenTL::URL_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
    GenTL::GC_ERROR     GCReadPortStacked           ( GenTL::PORT_HANDLE hPort, GenTL::PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries );
    GenTL::GC_ERROR     GCWritePortStacked          ( GenTL::PORT_HANDLE hPort, GenTL::PORT_REGISTER_STACK_ENTRY *pEntries, size_t *piNumEntries );
#endif

#if GENTL_H_AT_LEAST_V(1,3,0)
    GenTL::GC_ERROR     DSGetBufferChunkData        (GenTL::DS_HANDLE hDataStream, GenTL::BUFFER_HANDLE hBuffer, GenTL::SINGLE_CHUNK_DATA *pChunkData, size_t *piNumChunks );
#endif

#if GENTL_H_AT_LEAST_V(1,4,0)
    GenTL::GC_ERROR     IFGetParentTL               ( GenTL::IF_HANDLE hIface, GenTL::TL_HANDLE *phSystem );
    GenTL::GC_ERROR     DevGetParentIF              ( GenTL::DEV_HANDLE hDevice, GenTL::IF_HANDLE *phIface );
    GenTL::GC_ERROR     DSGetParentDev              ( GenTL::DS_HANDLE hDataStream, GenTL::DEV_HANDLE *phDevice );
#endif
    
#if GENTL_H_AT_LEAST_V(1,5,0)
    GenTL::GC_ERROR     DSGetNumBufferParts         ( GenTL::DS_HANDLE hDataStream, GenTL::BUFFER_HANDLE hBuffer, uint32_t *piNumParts );
    GenTL::GC_ERROR     DSGetBufferPartInfo         ( GenTL::DS_HANDLE hDataStream, GenTL::BUFFER_HANDLE hBuffer, uint32_t iPartIndex, GenTL::BUFFER_PART_INFO_CMD iInfoCmd, GenTL::INFO_DATATYPE *piType, void *pBuffer, size_t *piSize );
#endif

    //// Additional helper functions, etc. ////
    static std::string  gentlError                  (const int status, std::string const& custom_msg);

    // Optionally strip the URL of any locale prefix, perform URL decoding as appropriate, and return
    // a constant corresponding to the detected locale.
    enum UrlScheme
    {
        LocalScheme     = 0,
        FileScheme      = 1,
        HttpScheme      = 2,

        UnknownScheme   = ~0
    };

    static UrlScheme    processUrl                  (GenTL_CTI_string &url, const bool stripLocale = true);

    // Convenience functions to retrieve node maps and remote XMLs (as strings).
    NodeMapPtr          getNodeMap                  (GenTL::PORT_HANDLE hPort, GenApi::IPort *const portPtr, std::string gcstrPortName = "Device", const unsigned int xmlIndex = 0);
    NodeMapPtr          getNodeMap                  (GenTL::PORT_HANDLE hPort, PortPtr &portPtr, std::string gcstrPortName = "Device", const unsigned int xmlIndex = 0);
};

#endif
