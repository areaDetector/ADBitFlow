/* FILE:        GenTLView.cpp
 * DATE:        1/5/2017
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * DESCRIPTION: Methods to run a simple or an advanced (interactive) acquisition from
 *              a GenTL producer, displaying each frame in an ImageView window.
 */

#if defined(_WIN32) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

#include "GenTLView.h"
#include "GenTLInterfaceExample.h"

// BitFlow ImageView library includes.
#include <ImageView/ImageViewApplication.h>
#include <ImageView/ImageViewWindow.h>
#include <ImageView/ImperativeAcquisitionInterface.h>

// Header file defining the GenTL_CTI class.
#include "../GenTLInterface/GenICamUtilities.h"

// GenICam header files we depend upon. These are also included by GenICamUtilities.h
#include <Base/GCUtilities.h>
#include <GenApi/GenApi.h>

#include <BFResolveGenTL.h>

// C++ Standard libraries.
#include <cstddef>
#include <cstdint>

#include <iostream>
#include <iomanip>

#include <algorithm>

#include <vector>
#include <deque>
#include <string>

#include <chrono>
#include <mutex>
#include <atomic>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#if defined(_WIN32)
typedef std::chrono::steady_clock std_chrono_steady_clock;
#elif defined(__GNUC__)
#   if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
typedef std::chrono::steady_clock std_chrono_steady_clock;
#   else
typedef std::chrono::monotonic_clock std_chrono_steady_clock;
#   endif
#else
#   error Native library support not implemented on this platform.
#endif

enum BFBitDepth_PFNC_Codes
{
    RAW_PFNC            = 0x80000000,
    BF8_Mono8_PFNC      = 0x01080001,
    BF10_Mono10_PFNC    = 0x01100003,
    BF10P_Mono10p_PFNC  = 0x010A0046,
    BF12_Mono12_PFNC    = 0x01100005,
    BF14_Mono14_PFNC    = 0x01100025,
    BF16_Mono16_PFNC    = 0x01100007,
    BF24_RGB8_PFNC      = 0x02180014,
    BF30_RGB10p32_PFNC  = 0x0220001D,
    BF32_RGBa8_PFNC     = 0x02200016,
    BF30_RGB10_PFNC     = 0x02300018,
    BF36_RGB12_PFNC     = 0x0230001A,
    BF42_RGB14_PFNC     = 0x0230005E,
    BF48_RGB16_PFNC     = 0x02300033
};

template<typename CharT>
inline static void clean (std::basic_istream<CharT> &strm)
{
    if (strm.rdbuf()->sungetc() != std::char_traits<CharT>::eof() && strm.get() != strm.widen('\n'))
    {
        strm.clear();
        strm.ignore(std::numeric_limits<std::streamsize>::max(), strm.widen('\n'));
    }
}

template <typename ClockT = std_chrono_steady_clock>
struct LapAverager
{
    typedef ClockT Clock;

private:
    // Members.
    std::vector<typename Clock::time_point> m_times;
    size_t m_laps;

    mutable std::mutex m_mutex;

    // Illegal.
    LapAverager   (LapAverager const&);
    LapAverager& operator= (LapAverager const&);

public:
    LapAverager (const size_t periodCnt = 127)
    {
        resize(periodCnt);
    }

    inline void resize (const size_t periodCnt)
    {
        const std::lock_guard<std::mutex> lock (m_mutex);

        m_laps = 0;
        m_times.clear();
        m_times.resize(1 + periodCnt, Clock::time_point::min());
    }

    // Return the rate average in Hz.
    inline double rate_average (void) const
    {
        const std::lock_guard<std::mutex> lock (m_mutex);

        if (m_laps <= 1)
            return 0.;

        const auto now = Clock::now();
        typename Clock::duration sum = std::chrono::seconds(0);
        size_t cnt = 0;

        const size_t timesCnt = std::min(m_times.size(), m_laps);
        const size_t oldestIndex = m_laps % timesCnt;

        auto prevTime = m_times[oldestIndex];
        for (size_t i = 1; i < timesCnt; i++)
        {
            const size_t thisIndex = (oldestIndex + i) % timesCnt;
            const auto thisTime = m_times[thisIndex];

            if (now - thisTime < std::chrono::seconds(5))
            {
                sum += (thisTime - prevTime);
                cnt++;
            }

            prevTime = thisTime;
        }

        if (0 == cnt)
            return 0.;

        const double denom = double(std::chrono::duration_cast<std::chrono::nanoseconds>(sum).count());
        const double numer = double(cnt) * 1000000000.;
        return numer / denom;
    }

    inline size_t lap (void)
    {
        const std::lock_guard<std::mutex> lock (m_mutex);

        m_times[m_laps % m_times.size()] = Clock::now();

        return ++m_laps;
    }
};

class GenTLView IMAGEVIEW_FINAL : public ImageView::ImperativeAcquisitionInterface
{
private:
    // Members.
    GenTL_CTI                           &m_cti;
    const DEV_HANDLE                    m_hDev;
    GenTL_CTI::NodeMapPtr const&        m_remDevNodeMap;

    std::atomic<bool>                   m_keepGoing;
    boost::thread                       m_commandThread;
    boost::promise<bool>                m_commandPromise;

    bool                                m_autoRequeue;
    bool                                m_messagesEnabled;

    DS_HANDLE                           m_hDS;
    EVENT_HANDLE                        m_eventNewBuffer;

    ImageView::Format                   m_bayerFormat;

    LapAverager<>                       m_lapAverager;

    std::vector<BUFFER_HANDLE>          m_hBuffers;
    mutable std::deque<BUFFER_HANDLE>   m_dequeued;

    mutable std::mutex                  m_poolMutex;
    boost::thread                       m_acqThread;

    bool                                m_hasViewer;

    // Illegal.
    GenTLView (void);
    GenTLView (GenTLView const&);
    GenTLView& operator= (GenTLView const&);

public:
    inline GenTLView (GenTL_CTI &a_cti, const DEV_HANDLE a_hDev, GenTL_CTI::NodeMapPtr const& a_remDevNodeMap);
    inline ~GenTLView (void) IMAGEVIEW_OVERRIDE;

    // Acquisition interface methods.
    inline bool isSetup (void) const IMAGEVIEW_OVERRIDE;
    inline bool isRunning (void) const IMAGEVIEW_OVERRIDE;
    inline double frameRate (void) const IMAGEVIEW_OVERRIDE;

    // Basic mode initialization.
    inline bool init (const bool a_hasViewer = true);

    // Advanced mode initialization.
    inline bool init_adv (const bool a_hasViewer = true);

    // Wait until the command thread exits.
    inline bool wait_done (void);

private:
    // Local methods.
    inline void updateBayerFormat (void);
    inline bool setup (void);

    inline GenICam::gcstring safeNodeToStr (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& valueNodeName);

    inline bool safeNodeExec (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& commandNodeName);
    inline bool safeNodeFromStr (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& valueNodeName, GenICam::gcstring const& value);

    template<typename CNodePtrT, typename ValueT>
    inline bool safeNodeSet (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& valueNodeName, ValueT const& value);

    inline void genTLViewAcqThread (void);

    inline void commandThread (void);
    inline void commandThreadAdv (void);

    template<typename PtrT>
    inline bool gcCanWrite (PtrT const& ptr);
};

GenTLView::GenTLView (GenTL_CTI &a_cti, const DEV_HANDLE a_hDev, GenTL_CTI::NodeMapPtr const& a_remDevNodeMap)
    : m_cti             (a_cti)
    , m_hDev            (a_hDev)
    , m_remDevNodeMap   (a_remDevNodeMap)

    , m_keepGoing       (true)
    , m_autoRequeue     (true)
    , m_messagesEnabled (false)

    , m_hDS             (GENTL_INVALID_HANDLE)
    , m_eventNewBuffer  (GENTL_INVALID_HANDLE)

    , m_bayerFormat     (ImageView::Unencoded)

    , m_hasViewer       (true)
{
    // Enable the frame rate feature.
    setHasFrameRate(true);

    // Specify that our frames remain available throughout acquisition.
    setFramesLocked(true);
}

GenTLView::~GenTLView (void)
{
    GC_ERROR err;

    // Acquisition exit routine.
    std::cout << "\nExiting acquisition...\n";

    // Set the exit command.
    m_keepGoing.store(false);

    // Stop camera acquisition.
    safeNodeExec(m_remDevNodeMap, "AcquisitionStop");

    // Stop host acquisition.
    if (GENTL_INVALID_HANDLE != m_hDS)
    {
        std::cout << "  Acquisition stop... ";
        err = m_cti.DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_DEFAULT);
        if (GC_ERR_SUCCESS != err)
            std::cerr << "Failure! You may need to force quit the application.\n";
        else
            std::cout << "Success!\n";
    }

    // Abort the acquisition thread.
    if (GENTL_INVALID_HANDLE != m_eventNewBuffer)
    {
        std::cout << "  Killing the acquisition thread... ";
        err = m_cti.EventKill(m_eventNewBuffer);
        if (GC_ERR_SUCCESS != err)
            std::cerr << "Failure! You may need to force quit the application.\n";
        else
            std::cout << "Success!\n  Exiting control loop.\n";
    }

    // Wait for the acquisition loop thread to exit.
    if (m_acqThread.joinable())
        m_acqThread.join();

    // Wait for the control thread to exit.
    if (m_commandThread.joinable())
        m_commandThread.join();

    // Dequeue any queued buffers.
    if (GENTL_INVALID_HANDLE != m_hDS)
    {
        err = m_cti.DSFlushQueue(m_hDS, ACQ_QUEUE_ALL_DISCARD);
        if (GC_ERR_SUCCESS != err)
            std::cerr << "Encountered an error dequeuing all buffers.\n";

        // Free all buffers.
        for (BUFFER_HANDLE& hDelBuf : m_hBuffers)
        {
            err = m_cti.DSRevokeBuffer(m_hDS, hDelBuf, 0, 0);
            if (GC_ERR_SUCCESS != err)
                std::cerr << "Encountered an error dequeuing buffer 0x" << std::hex << size_t(hDelBuf) << ".\n" << std::dec;
        }

        // Close the DataStream.
        err = m_cti.DSClose(m_hDS);
        if (GC_ERR_SUCCESS != err)
            std::cerr << "Encountered an error closing the TLDataStream.\n";
    }

    // Disable TLParamsLocked.
    if (!safeNodeSet<CIntegerPtr>(m_remDevNodeMap, "TLParamsLocked", 0))
        std::cerr << "WARNING: Failed to disable TLParamsLocked.\n";

    printErrors(std::cerr, &m_cti);
}

// Returns true/false, whether or not the interface is setup for acquisition.
bool GenTLView::isSetup (void) const
{
    return true;
}

// Returns true/false, whether or not acquisition is running.
bool GenTLView::isRunning (void) const
{
    return true;
}

// Return the estimated frame rate.
double GenTLView::frameRate (void) const
{
    return m_lapAverager.rate_average();
}

// Basic mode initialization.
bool GenTLView::init (const bool a_hasViewer)
{
    GC_ERROR err;

    m_hasViewer = a_hasViewer;
    m_messagesEnabled = !a_hasViewer;

    // Setup the GenTL acquisition.
    if (!setup())
        return false;

    // Enable buffer auto-requeue.
    m_autoRequeue = true;

    // Queue all buffers.
    std::cout << "\nQueuing all buffers... ";
    for (auto Iter = m_dequeued.begin(); m_dequeued.end() != Iter; )
    {
        err = m_cti.DSQueueBuffer(m_hDS, *Iter);
        if (GC_ERR_SUCCESS != err)
            Iter++;
        else
            Iter = m_dequeued.erase(Iter);
    }

    if (m_dequeued.size())
        std::cerr << "Queued only " << (m_hBuffers.size() - m_dequeued.size()) << " of " << m_hBuffers.size() << " buffers.\n";
    else
        std::cout << "Success!\n";

    // Start host acquisition.
    std::cout << "\nStarting host acquisition... ";
    err = m_cti.DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << "ERROR starting acquisition. Aborting.\n";

        for (BUFFER_HANDLE& hDelBuf : m_hBuffers)
        {
            if (GENTL_INVALID_HANDLE != hDelBuf)
                m_cti.DSRevokeBuffer(m_hDS, hDelBuf, 0, 0);
        }
        m_cti.DSClose(m_hDS);
        return false;
    }
    else
        std::cout << "Success!\n";

    // Start camera acquisition (may be unneccessary).

        // For good measure, make sure acquisition is stopped, and TLParamsLocked is disabled.

    std::cout << "\nClear camera acquisition...\n";

    safeNodeExec(m_remDevNodeMap, "AcquisitionStop");

    safeNodeSet<CIntegerPtr>(m_remDevNodeMap, "TLParamsLocked", 0);

        // Execute the standard routine for acquisition start.
    std::cout << "\n... starting continuous camera acquisition...\n";

    safeNodeFromStr(m_remDevNodeMap, "AcquisitionMode", "Continuous");

    safeNodeFromStr(m_remDevNodeMap, "TriggerSelector", "AcquisitionStart");

    safeNodeFromStr(m_remDevNodeMap, "TriggerMode", "Off");

    safeNodeSet<CIntegerPtr>(m_remDevNodeMap, "TLParamsLocked", 1);

    std::cout << "\n\tQuery Bayer Format\n";
    updateBayerFormat();

    safeNodeExec(m_remDevNodeMap, "AcquisitionStart");

    std::cout << "\n... completed camera acquisition start routine. \n";

    // Start the command thread.
    m_commandPromise = boost::promise<bool>();
    m_commandThread = boost::thread(&GenTLView::commandThread, this);

    return true;
}

// Advanced mode initialization.
bool GenTLView::init_adv (const bool a_hasViewer)
{
    m_hasViewer = a_hasViewer;
    m_messagesEnabled = !a_hasViewer;

    // Setup the GenTL acquisition.
    if (!setup())
        return false;

    // Set the camera to a default working state.

        // For good measure, make sure acquisition is stopped, and TLParamsLocked is disabled.

    std::cout << "\nClear camera acquisition...\n";

    safeNodeExec(m_remDevNodeMap, "AcquisitionStop");

    safeNodeSet<CIntegerPtr>(m_remDevNodeMap, "TLParamsLocked", 0);

        // Default to continuous acquisition mode.
    std::cout << "\n... setup camera for continuous acquisition...\n";

    safeNodeFromStr(m_remDevNodeMap, "AcquisitionMode", "Continuous");

    safeNodeFromStr(m_remDevNodeMap, "TriggerSelector", "AcquisitionStart");

    safeNodeFromStr(m_remDevNodeMap, "TriggerMode", "Off");

    std::cout << "\n... camera is ready. \n";

    // Start the command thread.
    m_commandPromise = boost::promise<bool>();
    m_commandThread = boost::thread(&GenTLView::commandThreadAdv, this);

    return true;
}
// Wait until the command thread exits.
bool GenTLView::wait_done (void)
{
    return m_keepGoing.load() && m_commandPromise.get_future().get();
}

void GenTLView::updateBayerFormat (void)
{
    // Record the pixel format Bayer pattern, if any.
    std::cout << "Checking PixelFormat for Bayer filter... ";

    const std::string pixelFormat = safeNodeToStr(m_remDevNodeMap, "PixelFormat").c_str();
    if (pixelFormat.compare(0, 7, "BayerRG") == 0)
    {
        std::cout << "found RGGB.\n";
        m_bayerFormat = ImageView::BayerRGGB;
    }
    else if (pixelFormat.compare(0, 7, "BayerGR") == 0)
    {
        std::cout << "found GRBG.\n";
        m_bayerFormat = ImageView::BayerGRBG;
    }
    else if (pixelFormat.compare(0, 7, "BayerBG") == 0)
    {
        std::cout << "found BGGR.\n";
        m_bayerFormat = ImageView::BayerBGGR;
    }
    else if (pixelFormat.compare(0, 7, "BayerGB") == 0)
    {
        std::cout << "found GBRG.\n";
        m_bayerFormat = ImageView::BayerGBRG;
    }
    else
    {
        std::cout << "no Bayer encoding.\n";
        m_bayerFormat = ImageView::Unencoded;
    }
}

bool GenTLView::setup (void)
{
    GC_ERROR        err;
    INFO_DATATYPE   iType;
    size_t          iSize;

    // Ask the user how many buffers to allocate.
    uint32_t iNumDSs;
    err = m_cti.DevGetNumDataStreams(m_hDev, &iNumDSs);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << "Unable to load the determine the TLDevice DataStream count.\n";
        return false;
    }

    if (!iNumDSs)
    {
        std::cerr << "The TLDevice has no DataStreams, so there is nothing to acquire from.\n";
        return false;
    }

    // Printout the list of available datastreams.
    std::vector<std::string> dsIDs (iNumDSs);
    std::cout << "\nAvailable TLDataStreams:\n";
    for (uint32_t i = 0; i < iNumDSs; i++)
    {
        err = m_cti.DevGetDataStreamID(m_hDev, i, GENTL_INVALID_HANDLE, &iSize);
        if (GC_ERR_SUCCESS != err)
            std::cerr << "ERROR: Could not determine the ID string length of TLDataStream " << i << "!\n";
        else
        {
            std::vector<char> dsID (iSize, 0);
            err = m_cti.DevGetDataStreamID(m_hDev, i, dsID.data(), &iSize);
            if (GC_ERR_SUCCESS != err)
                std::cerr << "ERROR: Failed to retrieve the ID string of TLDataStream " << i << "!\n";
            else
            {
                std::cout << " (" << i << ") " << dsID.data() << "\n";
                dsIDs[i] = dsID.data();
            }
        }
    }

    // The user should select which DataStream to open.
    uint32_t acqDS;
    do
    {
        std::cout << "Which to open (0 to " << (iNumDSs - 1) << ")? ";
        std::cin >> acqDS;
    }
    while (acqDS >= iNumDSs);

    // Open the DataStream.
    std::cout << "\nOpening the TLDataStream... ";

    err = m_cti.DevOpenDataStream(m_hDev, dsIDs[acqDS].c_str(), &m_hDS);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << "ERROR opening TLDataStream \"" << dsIDs[acqDS] << "\".\n";
        return false;
    }

    std::cout << "Success!\n\n";

    // Should we use High Frame Rate acquisition mode?
    bool hfrYes = false, hfrNo = false;
    do
    {
        std::cout << "Use High Frame Rate acquisition (Y/N)? ";

        std::string ans;
        std::cin >> ans;

        hfrYes = ("Y" == ans || "y" == ans);
        hfrNo = ("N" == ans || "n" == ans);
    }
    while (!hfrYes && !hfrNo);

    // Apply the user setting for High Frame Rate mode.
    GenTL_CTI::PortPtr dsPort;
    const auto dsNodeMap = m_cti.getNodeMap(m_hDS, dsPort, "", 0);
    if (dsNodeMap.get())
    {
        const CBooleanPtr hfrBoolPtr (dsNodeMap->_GetNode("GrabberHighFrameRate"));
        if (hfrBoolPtr)
        {
            try
            {
                hfrBoolPtr->SetValue(hfrYes);
            }
            catch (std::exception const& err)
            {
                std::cerr << "High Frame Rate mode could not be enabled/disabled: " << err.what() << "\n";
            }
        }
        else
            std::cerr << "Could not find the GrabberHighFrameRate node!\n";
    }
    else
        std::cerr << "Unable to open TLDataStream node map for setup!\n";

    // Register the new-buffer event.
    std::cout << "Registering the TLDataStream EVENT_NEW_BUFFER event... ";

    err = m_cti.GCRegisterEvent(m_hDS, EVENT_NEW_BUFFER, &m_eventNewBuffer);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << "ERROR!\n";
        m_cti.DSClose(m_hDS);
        return false;
    }
    std::cout << "Success!\n\n";

    // Determine the DataStream minimum buffer count, and buffer byte size.
    std::cout << "Retrieving TLDataStream parameters.\n";

    size_t minimumBufferCnt;
    iSize = sizeof(minimumBufferCnt);
    err = m_cti.DSGetInfo(m_hDS, STREAM_INFO_BUF_ANNOUNCE_MIN, &iType, &minimumBufferCnt, &iSize);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << " -- ERROR: Could not determine the STREAM_INFO_BUF_ANNOUNCE_MIN.\n";
        m_cti.DSClose(m_hDS);
        return false;
    }
    std::cout << " -- STREAM_INFO_BUF_ANNOUNCE_MIN = " << minimumBufferCnt << "\n";

    size_t bufferSize;
    iSize = sizeof(bufferSize);
    err = m_cti.DSGetInfo(m_hDS, STREAM_INFO_PAYLOAD_SIZE, &iType, &bufferSize, &iSize);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << " -- ERROR: Could not determine the STREAM_INFO_PAYLOAD_SIZE.\n";
        m_cti.DSClose(m_hDS);
        return false;
    }
    std::cout << " -- STREAM_INFO_PAYLOAD_SIZE = " << bufferSize << "\n";

    // The user should select the number of buffers to allocate, and we should allocate them.
    uint32_t acqBufCnt = 0;

    while (acqBufCnt < minimumBufferCnt)
    {
        std::cout << "How many buffers to allocate ( >= " << minimumBufferCnt << ")? ";
        std::cin >> acqBufCnt;
        if (acqBufCnt < minimumBufferCnt)
            continue;

        try
        {
            m_hBuffers.resize(acqBufCnt, GENTL_INVALID_HANDLE);
        }
        catch (std::bad_alloc const& err)
        {
            std::cerr << "Buffer allocation failed: " << err.what() << "\n";
            acqBufCnt = 0;
            continue;
        }

        for (BUFFER_HANDLE& hBuf : m_hBuffers)
        {
            err = m_cti.DSAllocAndAnnounceBuffer(m_hDS, bufferSize, 0, &hBuf);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "ERROR during DSAllocAndAnnounceBuffer. Cleaning up announced buffers, and aborting.\n";
                for (BUFFER_HANDLE& hDelBuf : m_hBuffers)
                {
                    if (GENTL_INVALID_HANDLE != hDelBuf)
                        m_cti.DSRevokeBuffer(m_hDS, hDelBuf, 0, 0);
                }
                m_cti.DSClose(m_hDS);
                return false;
            }
        }

        std::cout << "Successfully allocated " << m_hBuffers.size() << " buffers.\n";
    }

    m_dequeued.assign(m_hBuffers.begin(), m_hBuffers.end());

    // Start the acquisition loop thread.
    m_acqThread = boost::thread(&GenTLView::genTLViewAcqThread, this);

    return true;
}

GenICam::gcstring GenTLView::safeNodeToStr (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& valueNodeName)
{
    GenICam::gcstring returnVal;

    if (!nodeMap.get())
        std::cerr << "WARNING: Cannot read from an IValueNode on a nullptr node map.\n";
    else
    {
        try
        {
            GenApi::CValuePtr valueNode = nodeMap->_GetNode(valueNodeName);
            if (valueNode)
                returnVal = valueNode->ToString(true, true);
            else
                std::cerr << "WARNING: Unable to find IValueNode \"" << valueNodeName.c_str() << "\"\n";
        }
        catch (GenICam::GenericException const& err)
        {
            std::cerr << "WARNING: Caught a GenICam exception while setting IValueNode \"" << valueNodeName.c_str() << "\" -- " << err.what() << "\n";
        }
        catch (std::exception const& err)
        {
            std::cerr << "WARNING: Caught a generic exception while setting IValueNode \"" << valueNodeName.c_str() << "\" -- " << err.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "WARNING: Caught an unknown exception while setting IValueNode \"" << valueNodeName.c_str() << "\"\n";
        }
    }

    return returnVal;
}

bool GenTLView::safeNodeExec (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& commandNodeName)
{
    if (!nodeMap.get())
    {
        std::cerr << "WARNING: Cannot execute ICommandNode on a nullptr node map.\n";
        return false;
    }

    try
    {
        std::cout << "Execute " << commandNodeName << "\n";

        GenApi::CCommandPtr commandNode = nodeMap->_GetNode(commandNodeName);
        if (commandNode)
        {
            commandNode->Execute(true);
            return true;
        }

        std::cerr << "WARNING: Unable to find ICommandNode \"" << commandNodeName.c_str() << "\"\n";
    }
    catch (GenICam::GenericException const& err)
    {
        std::cerr << "WARNING: Caught a GenICam exception while executing ICommandNode \"" << commandNodeName.c_str() << "\" -- " << err.what() << "\n";
    }
    catch (std::exception const& err)
    {
        std::cerr << "WARNING: Caught a generic exception while executing ICommandNode \"" << commandNodeName.c_str() << "\" -- " << err.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "WARNING: Caught an unknown exception while executing ICommandNode \"" << commandNodeName.c_str() << "\"\n";
    }

    return false;
}
bool GenTLView::safeNodeFromStr (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& valueNodeName, GenICam::gcstring const& value)
{
    if (!nodeMap.get())
    {
        std::cerr << "WARNING: Cannot write to an IValueNode on a nullptr node map.\n";
        return false;
    }

    try
    {
        std::cout << "Set " << valueNodeName << " " << value << "\n";

        GenApi::CValuePtr valueNode = nodeMap->_GetNode(valueNodeName);
        if (valueNode)
        {
            valueNode->FromString(value, true);
            return true;
        }

        std::cerr << "WARNING: Unable to find IValueNode \"" << valueNodeName.c_str() << "\"\n";
    }
    catch (GenICam::GenericException const& err)
    {
        std::cerr << "WARNING: Caught a GenICam exception while setting IValueNode \"" << valueNodeName.c_str() << "\" -- " << err.what() << "\n";
    }
    catch (std::exception const& err)
    {
        std::cerr << "WARNING: Caught a generic exception while setting IValueNode \"" << valueNodeName.c_str() << "\" -- " << err.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "WARNING: Caught an unknown exception while setting IValueNode \"" << valueNodeName.c_str() << "\"\n";
    }

    return false;
}

template<typename CNodePtrT, typename ValueT>
bool GenTLView::safeNodeSet (GenTL_CTI::NodeMapPtr const& nodeMap, GenICam::gcstring const& valueNodeName, ValueT const& value)
{
    if (!nodeMap.get())
    {
        std::cerr << "WARNING: Cannot write to an IValueNode on a nullptr node map.\n";
        return false;
    }

    try
    {
        std::cout << "Set " << valueNodeName << " " << value << "\n";

        CNodePtrT valueNode = nodeMap->_GetNode(valueNodeName);
        if (valueNode)
        {
            valueNode->SetValue(value, true);
            return true;
        }

        std::cerr << "WARNING: Unable to find IValueNode \"" << valueNodeName.c_str() << "\"\n";
    }
    catch (GenICam::GenericException const& err)
    {
        std::cerr << "WARNING: Caught a GenICam exception while setting IValueNode \"" << valueNodeName.c_str() << "\" -- " << err.what() << "\n";
    }
    catch (std::exception const& err)
    {
        std::cerr << "WARNING: Caught a generic exception while setting IValueNode \"" << valueNodeName.c_str() << "\" -- " << err.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "WARNING: Caught an unknown exception while setting IValueNode \"" << valueNodeName.c_str() << "\"\n";
    }

    return false;
}

void GenTLView::genTLViewAcqThread (void)
{
    GC_ERROR        err;
    INFO_DATATYPE   iType;
    size_t          iSize;

    while (m_keepGoing.load())
    {
        EVENT_NEW_BUFFER_DATA newBufferData;
        iSize = sizeof(newBufferData);

        //err = m_cti.EventGetData(m_eventNewBuffer, &newBufferData, &iSize, 1000 / 60);
        err = m_cti.EventGetData(m_eventNewBuffer, &newBufferData, &iSize, GENTL_INFINITE);
        std::lock_guard<std::mutex> lock (m_poolMutex);

        // Framerate averaging.
        m_lapAverager.lap();

        if (GC_ERR_TIMEOUT == err)
        {
            if (m_messagesEnabled)
                std::cout << "<- Timeout received after 2s\n";
        }
        else if (GC_ERR_ABORT == err)
        {
            if (m_messagesEnabled)
                std::cout << "<- Received Abort\n";
        }
        else if (GC_ERR_SUCCESS != err)
        {
            std::cerr << "<- Unexpected ERROR from EventGetData\n";
            continue;
        }
        else
        {
            if (m_messagesEnabled)
                std::cout << "<- Dequeued buffer 0x" << std::hex << size_t(newBufferData.BufferHandle) << "\n" << std::dec;

            void *bufferBase;
            iSize = sizeof(bufferBase);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_BASE, &iType, &bufferBase, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_BASE\n";
                bufferBase = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_BASE = " << bufferBase << "\n";
            }

            size_t bufferSize;
            iSize = sizeof(bufferSize);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_SIZE, &iType, &bufferSize, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_SIZE\n";
                bufferSize = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_SIZE = " << bufferSize << "\n";
            }

            std::uint64_t timestamp_ns;
            iSize = sizeof(timestamp_ns);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_TIMESTAMP_NS, &iType, &timestamp_ns, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_TIMESTAMP_NS\n";
                timestamp_ns = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_TIMESTAMP_NS = " << timestamp_ns << "ns\n";
            }

            size_t imageWidth;
            iSize = sizeof(imageWidth);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_WIDTH, &iType, &imageWidth, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_WIDTH\n";
                imageWidth = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_WIDTH = " << imageWidth << "\n";
            }

            size_t imageHeight;
            iSize = sizeof(imageHeight);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_HEIGHT, &iType, &imageHeight, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_HEIGHT\n";
                imageHeight = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_HEIGHT = " << imageHeight << "\n";
            }

            uint64_t pixelFormatNamespace;
            iSize = sizeof(pixelFormatNamespace);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_PIXELFORMAT_NAMESPACE, &iType, &pixelFormatNamespace, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_PIXELFORMAT\n";
                pixelFormatNamespace = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_PIXELFORMAT_NAMESPACE = " << std::hex << pixelFormatNamespace << "\n";
            }

            uint64_t pixelFormat;
            iSize = sizeof(pixelFormat);
            err = m_cti.DSGetBufferInfo(m_hDS, newBufferData.BufferHandle, BUFFER_INFO_PIXELFORMAT, &iType, &pixelFormat, &iSize);
            if (GC_ERR_SUCCESS != err)
            {
                std::cerr << "<- ERROR retrieving the BUFFER_INFO_PIXELFORMAT\n";
                pixelFormat = 0;
            }
            else
            {
                if (m_messagesEnabled)
                    std::cout << "<- BUFFER_INFO_PIXELFORMAT = " << std::hex << pixelFormat << "\n";
            }

            // Set the latest buffer for display, if valid.
            if (bufferBase && bufferSize > 0 && imageWidth > 0 && imageHeight > 0 && pixelFormat > 0)
            {
                // Convert the PFNC pixel format to the ImageView pixel format.
                ImageView::Format ivFormat = ImageView::Gray8;
                if (PIXELFORMAT_NAMESPACE_PFNC_32BIT != pixelFormatNamespace)
                    std::cerr << "<- WARNING: Unsupported pixel format namespace. Default to Mono8\n";
                else
                {
                    switch (pixelFormat)
                    {
                    default:
                        std::cerr << "<- WARNING: Unknown BUFFER_INFO_PIXELFORMAT. Default to Mono8\n";
                    case BF8_Mono8_PFNC:        ivFormat = ImageView::Gray8; break;
                    case BF10_Mono10_PFNC:      ivFormat = ImageView::Gray10; break;
                    case BF10P_Mono10p_PFNC:    ivFormat = ImageView::Format(ImageView::Gray10 | ImageView::Packed); break;
                    case BF12_Mono12_PFNC:      ivFormat = ImageView::Gray12; break;
                    case BF14_Mono14_PFNC:      ivFormat = ImageView::Gray14; break;
                    case BF16_Mono16_PFNC:      ivFormat = ImageView::Gray16; break;
                    case BF24_RGB8_PFNC:        ivFormat = ImageView::Color24; break;
                    case BF30_RGB10p32_PFNC:    ivFormat = ImageView::Color30; break;
                    case BF32_RGBa8_PFNC:       ivFormat = ImageView::Color32; break;
                    case BF30_RGB10_PFNC:       ivFormat = ImageView::HiColor30; break;
                    case BF36_RGB12_PFNC:       ivFormat = ImageView::HiColor36; break;
                    case BF42_RGB14_PFNC:       ivFormat = ImageView::HiColor42; break;
                    case BF48_RGB16_PFNC:       ivFormat = ImageView::HiColor48; break;
                    }
                }

                if (m_hasViewer)
                {
                    // Apply the Bayer filter format, if any.
                    ivFormat = ImageView::Format(ivFormat | m_bayerFormat);

                    // Set the buffer. Just assume 8-bit, for current purposes.
                    setNextFrame((uint8_t*)bufferBase, imageWidth, imageHeight, ivFormat);
                }

                if (m_messagesEnabled)
                    std::cout << "<- Valid frame received.\n";
            }

            // Optionally, requeue the buffer.
            if (m_autoRequeue)
            {
                err = m_cti.DSQueueBuffer(m_hDS, newBufferData.BufferHandle);
                if (GC_ERR_SUCCESS != err)
                    std::cerr << "<- ERROR requeuing buffer 0x" << std::hex << size_t(newBufferData.BufferHandle) << "\n" << std::dec;
                else
                {
                    if (m_messagesEnabled)
                        std::cout << "<- Requeued buffer 0x" << std::hex << size_t(newBufferData.BufferHandle) << "\n" << std::dec;
                }
            }
            else
                m_dequeued.push_back(newBufferData.BufferHandle);
        }

        if (m_messagesEnabled)
            printErrors(std::cerr, &m_cti);

        if (!m_keepGoing.load())
        {
            if (m_messagesEnabled)
                std::cout << "<- ...Acquisition exiting\n";
        }
    }
}

void GenTLView::commandThread (void)
{
    clean(std::cin);

    std::string line;
    std::cout << "\nPress ENTER/RETURN to exit GenTL View...\n";
    std::getline(std::cin, line);

    m_commandPromise.set_value(m_keepGoing.exchange(false));

    if (m_hasViewer)
        ImageView::Application::global()->exit();
}

void GenTLView::commandThreadAdv (void)
{
    // Enter the acquisition control loop.
    const char *const helpStr =
        "\nEnter line sequences of the following commands (case insensitive):\n"
        "  [H] Print this help message\n"
        "  [A] Start TLDataStream acquisition to acquire infinite frames\n"
        "  [S] Start TLDataStream acquisition to acquire a fixed count of frames\n"
        "  [D] Stop TLDataStream acquisition\n"
        "  [F] Kill TLDataStream acquisition\n"
        "  [Z] Start remote-device acquisition\n"
        "  [X] Stop remote-device acquisition\n"
        "  [L] TLParamsLocked = %s\n"
        "  [B] Query/clear the Bayer mosaic pattern\n"
        "  [Q] Automatic requeueing = %s\n"
        "  [W] Queue a single buffer to the input queue\n"
        "  [K] Kill the EVENT_NEW_BUFFER event\n"
        "  [?C] Query a camera node value\n"
        "  [=C] Set a camera node value\n"
        "  [?D] Query a TLDataStream node value\n"
        "  [=D] Set a TLDataStream node value\n"
        "  [ ] (Space) Sleep for one second\n"
        "  [.] Exit the acquisition routine\n";

    GC_ERROR err;

    GenApi::CIntegerPtr tlParamsLocked;

    if (!m_remDevNodeMap.get())
        std::cerr << "WARNING -- No Remote Device node map available. Some features cannot function.\n";

    if (m_remDevNodeMap.get())
    {
        try
        {
            tlParamsLocked = m_remDevNodeMap->_GetNode("TLParamsLocked");
            if (!tlParamsLocked)
                std::cerr << "WARNING -- \"TLParamsLocked\" isn't a command node!\n";
        }
        catch (std::exception const& err)
        {
            std::cerr << "Caught an error retrieving \"TLParamsLocked\": " << err.what() << "\n";
        }
    }

    // Attempt to load the remote device's first GenTL node map.
    GenTL_CTI::PortPtr dsPort;
    const auto dsNodeMap = m_cti.getNodeMap(m_hDS, dsPort, "", 0);
    if (!dsNodeMap.get())
        std::cerr << "WARNING -- Unable to load the TLDataStream node-map. Some features cannot function.\n";

    printf(helpStr, tlParamsLocked ? tlParamsLocked->ToString().c_str() : "Not Implemented", m_autoRequeue ? "enabled" : "disabled");

    std::cout << "\nTry: albz" << std::string(m_hBuffers.size(), 'w') << "\n";

    bool weQuit = false;
    while (m_keepGoing.load())
    {
        clean(std::cin);

        std::string line;
        std::cout << "Next command sequence? ";
        std::getline(std::cin, line);

        std::unique_lock<std::mutex> lock (m_poolMutex);

        for (size_t i = 0; i < line.length(); i++)
        {
            const char c = line[i];

            std::cout << "-> ";
            std::cout.put((char)toupper(c));
            std::cout << ": ";

            switch (c)
            {
            case 'H': case 'h':
                printf(helpStr, tlParamsLocked ? tlParamsLocked->ToString().c_str() : "Not Implemented", m_autoRequeue ? "enabled" : "disabled");
                break;

            case 'A': case 'a':
                err = m_cti.DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE);
                if (GC_ERR_SUCCESS != err)
                    std::cerr << "ERROR on DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE)!\n";
                else
                    std::cout << "DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, GENTL_INFINITE)\n";
                break;

            case 'S': case 's':
                {
                    uint64_t acquisitionLength;
                    do
                    {
                        std::cout << "How many buffers to acquire? ";
                        std::cin >> acquisitionLength;
                    }
                    while (!acquisitionLength);

                    err = m_cti.DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, acquisitionLength);
                    if (GC_ERR_SUCCESS != err)
                        std::cerr << "ERROR on DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, " << acquisitionLength << ")!\n";
                    else
                        std::cout << "DSStartAcquisition(m_hDS, ACQ_START_FLAGS_DEFAULT, " << acquisitionLength << ")!\n";
                }
                break;

            case 'D': case 'd':
                err = m_cti.DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_DEFAULT);
                if (GC_ERR_SUCCESS != err)
                    std::cerr << "ERROR on DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_DEFAULT)!\n";
                else
                    std::cout << "DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_DEFAULT)\n";
                break;

            case 'F': case 'f':
                err = m_cti.DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_KILL);
                if (GC_ERR_SUCCESS != err)
                    std::cerr << "ERROR on DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_KILL)!\n";
                else
                    std::cout << "DSStopAcquisition(m_hDS, ACQ_STOP_FLAGS_KILL)\n";
                break;

            case 'Z': case 'z':
                if (m_remDevNodeMap.get())
                {
                    try
                    {
                        GenApi::CCommandPtr acqStart = m_remDevNodeMap->_GetNode("AcquisitionStart");
                        if (!acqStart)
                            std::cerr << "ERROR -- \"AcquisitionStart\" isn't a command node!\n";
                        else
                        {
                            acqStart->Execute();
                            std::cout << "\"AcquisitionStart\"\n";
                        }
                    }
                    catch (std::exception const& err)
                    {
                        std::cerr << "Caught an error retrieving \"AcquisitionStart\": " << err.what() << "\n";
                    }
                }
                else
                    std::cerr << "Cannot execute AcquisitionStart with no Remote Device port.\n";
                break;

            case 'X': case 'x':
                if (m_remDevNodeMap.get())
                {
                    try
                    {
                        GenApi::CCommandPtr acqStop = m_remDevNodeMap->_GetNode("AcquisitionStop");
                        if (!acqStop)
                            std::cerr << "ERROR -- \"AcquisitionStop\" isn't a command node!\n";
                        else
                        {
                            acqStop->Execute();
                            std::cout << "\"AcquisitionStop\"\n";
                        }
                    }
                    catch (std::exception const& err)
                    {
                        std::cerr << "Caught an error retrieving \"AcquisitionStop\": " << err.what() << "\n";
                    }
                }
                else
                    std::cerr << "Cannot execute AcquisitionStop with no Remote Device port.\n";
                break;

            case 'L': case 'l':
                try
                {
                    if (!tlParamsLocked)
                        std::cerr << "ERROR -- \"TLParamsLocked\" isn't a command node!\n";
                    else
                    {
                        if (tlParamsLocked->GetValue() == 0)
                            tlParamsLocked->SetValue(1);
                        else
                            tlParamsLocked->SetValue(0);
                        std::cout << "TLParamsLocked = " << tlParamsLocked->GetValue() << "\n";
                    }
                }
                catch (std::exception const& err)
                {
                    std::cerr << "Caught an error retrieving \"TLParamsLocked\": " << err.what() << "\n";
                }
                break;

            case 'B': case 'b':
                if (ImageView::Unencoded == m_bayerFormat)
                    updateBayerFormat();
                else
                {
                    m_bayerFormat = ImageView::Unencoded;
                    std::cout << "Cleared Bayer mosaic pattern.\n";
                }
                break;

            case 'Q': case 'q':
                std::cout << "Auto-requeue = " << ((m_autoRequeue = !m_autoRequeue) ? "enabled" : "disabled") << "\n";
                break;

            case 'W': case 'w':
                if (m_dequeued.empty())
                    std::cerr << "ERROR No buffer available for queuing\n";
                else
                {
                    const BUFFER_HANDLE qBuf = m_dequeued.front();
                    err = m_cti.DSQueueBuffer(m_hDS, qBuf);
                    if (GC_ERR_SUCCESS != err)
                        std::cerr << "ERROR queuing a buffer\n";
                    else
                    {
                        m_dequeued.pop_front();
                        std::cout << "Queued buffer 0x" << std::hex << size_t(qBuf) << "\n" << std::dec;
                    }
                }
                std::cout << "queued " << (m_hBuffers.size() - m_dequeued.size()) << " of " << m_hBuffers.size() << "\n";
                break;

            case '?':
                if (line.length() > i + 1)
                {
                    lock.unlock();

                    GenTL_CTI::NodeMapPtr nodeMap;

                    switch (line[++i])
                    {
                    case 'c': case 'C':
                        nodeMap = m_remDevNodeMap;
                        break;
                    case 'd': case 'D':
                        nodeMap = dsNodeMap;
                        break;

                    default:
                        std::cerr << "ERROR bad target '" << line[i] << "'\n";
                        break;
                    }

                    if (nodeMap.get())
                    {
                        std::cout << "Node name? ";
                        std::string nodeName;
                        std::cin >> nodeName;
                        std::cout << nodeName << " = " << safeNodeToStr(nodeMap, nodeName.c_str()) << "\n";
                    }

                    lock.lock();
                }
                else
                    std::cerr << "ERROR missing query target (C or D)\n";

                break;

            case '=':
                if (line.length() > i + 1)
                {
                    lock.unlock();

                    GenTL_CTI::NodeMapPtr nodeMap;

                    switch (line[++i])
                    {
                    case 'c': case 'C':
                        nodeMap = m_remDevNodeMap;
                        break;
                    case 'd': case 'D':
                        nodeMap = dsNodeMap;
                        break;

                    default:
                        std::cerr << "ERROR bad target '" << line[i] << "'\n";
                        break;
                    }

                    if (nodeMap.get())
                    {
                        std::cout << "Node name and value (space separated)? ";
                        std::string nodeName, nodeValue;
                        std::cin >> nodeName >> nodeValue;
                        if (!safeNodeFromStr(nodeMap, nodeName.c_str(), nodeValue.c_str()))
                            std::cerr << "ERROR setting the node value.\n";
                    }

                    lock.lock();
                }
                else
                    std::cerr << "ERROR missing target (C or D)\n";

                break;

            case ' ':
                lock.unlock();

                std::cout << "Sleeping one second... ";
                std::cout.flush();
                boost::this_thread::sleep_for(boost::chrono::seconds(1));
                std::cout << "Awake!\n";

                lock.lock();
                break;

            case '.':
                std::cout << "Acquisition exit at command sequence end.\n";
                weQuit = m_keepGoing.exchange(false);
                break;

            case 'K': case 'k':
                err = m_cti.EventKill(m_eventNewBuffer);
                if (GC_ERR_SUCCESS != err)
                    std::cerr << "ERROR on EventKill(m_eventNewBuffer)!\n";
                else
                    std::cout << "EventKill(m_eventNewBuffer)\n";
                break;

            default:
                std::cerr << "Unknown command\n";
                break;
            }
        }
    }

    m_commandPromise.set_value(weQuit);

    if (m_hasViewer)
        ImageView::Application::global()->exit();
}

template<typename PtrT>
bool GenTLView::gcCanWrite (PtrT const& ptr)
{
    if (!ptr)
        return false;

    switch (ptr->GetAccessMode())
    {
    case GenApi::WO: case GenApi::RW:
        return true;
    default:
        break;
    }
    return false;
}

// Simple acquisition with no actual viewer.
void execGenTLAcq (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap)
{
    GenTLView gtlView (*cti, hDev, remDevNodeMap);

    if (gtlView.init(false))
        gtlView.wait_done();
}

// Advanced acquisition with no actual viewer.
void execGenTLAcqAdv (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap)
{
    GenTLView gtlView (*cti, hDev, remDevNodeMap);

    if (gtlView.init_adv(false))
        gtlView.wait_done();
}

void execGenTLView (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap)
{
    // Create the ImageView global Application object.
    const int ivArgc = 1;
    const char *const ivArgv[] = { "GenTLView" };
    ImageView::Application app (ivArgc, ivArgv);

    // Create the GenTLView acquisition interface object.
    auto *const gtlv = new GenTLView (*cti, hDev, remDevNodeMap);

    // Create an ImageView Window.
    ImageView::Window window (ImageView::make_scoped(gtlv));

    window.setCloseable(false);
    window.setTitle("GenTL View");

    // Initialize the GenTLView object.
    if (gtlv->init())
    {
        // Show our window and enter the application main loop.
        window.show();

        // Enter the ImageView main loop.
        app.exec();
    }
}

void execGenTLViewAdv (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap)
{
    // Create the ImageView global Application object.
    const int ivArgc = 1;
    const char *const ivArgv[] = { "GenTLView" };
    ImageView::Application app (ivArgc, ivArgv);

    // Create the GenTLView acquisition interface object.
    auto *const gtlv = new GenTLView (*cti, hDev, remDevNodeMap);

    // Create an ImageView Window.
    ImageView::Window window (ImageView::make_scoped(gtlv));

    window.setCloseable(false);
    window.setTitle("GenTL View Advanced");

    // Initialize the GenTLView object.
    if (gtlv->init_adv())
    {
        // Show our window and enter the application main loop.
        window.show();

        // Enter the ImageView main loop.
        app.exec();
    }
}
