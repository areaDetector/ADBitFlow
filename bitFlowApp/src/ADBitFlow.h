#ifndef ADBITFLOW_H
#define ADBITFLOW_H

#include <epicsEvent.h>

#include <ADGenICam.h>
#ifdef _WIN32
  #include "CircularInterface.h"
  using namespace BufferAcquisition;
#else
  #include "BFciLib.h"
#endif

#define SPConvertPixelFormatString          "SP_CONVERT_PIXEL_FORMAT"           // asynParamInt32, R/W
#define SPStartedFrameCountString           "SP_STARTED_FRAME_COUNT"            // asynParamInt32, R/O
#define SPDeliveredFrameCountString         "SP_DELIVERED_FRAME_COUNT"          // asynParamInt32, R/O
#define SPReceivedFrameCountString          "SP_RECEIVED_FRAME_COUNT"           // asynParamInt32, R/O
#define SPIncompleteFrameCountString        "SP_INCOMPLETE_FRAME_COUNT"         // asynParamInt32, R/O
#define SPLostFrameCountString              "SP_LOST_FRAME_COUNT"               // asynParamInt32, R/O
#define SPDroppedFrameCountString           "SP_DROPPED_FRAME_COUNT"            // asynParamInt32, R/O
#define SPInputBufferCountString            "SP_INPUT_BUFFER_COUNT"             // asynParamInt32, R/O
#define SPOutputBufferCountString           "SP_OUTPUT_BUFFER_COUNT"            // asynParamInt32, R/O
#define SPReceivedPacketCountString         "SP_RECEIVED_PACKET_COUNT"          // asynParamInt32, R/O
#define SPMissedPacketCountString           "SP_MISSED_PACKET_COUNT"            // asynParamInt32, R/O
#define SPResendRequestedPacketCountString  "SP_RESEND_REQUESTED_PACKET_COUNT"  // asynParamInt32, R/O
#define SPResendReceivedPacketCountString   "SP_RESEND_RECEIVED_PACKET_COUNT"   // asynParamInt32, R/O
#define SPTimeStampModeString               "SP_TIME_STAMP_MODE"                // asynParamInt32, R/O
#define SPUniqueIdModeString                "SP_UNIQUE_ID_MODE"                 // asynParamInt32, R/O


/** Main driver class inherited from areaDetectors ADDriver class.
 * One instance of this class will control one camera.
 */
class ADBitFlow : public ADGenICam
{
public:
    ADBitFlow(const char *portName, int cameraId, int numSPBuffers,
                size_t maxMemory, int priority, int stackSize);

    // virtual methods to override from ADGenICam
    //virtual asynStatus writeInt32( asynUser *pasynUser, epicsInt32 value);
    //virtual asynStatus writeFloat64( asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                                size_t nElements, size_t *nIn);
    void report(FILE *fp, int details);
    virtual GenICamFeature *createFeature(GenICamFeatureSet *set, 
                                          std::string const & asynName, asynParamType asynType, int asynIndex,
                                          std::string const & featureName, GCFeatureType_t featureType);
    
    BFGTLDev getBFGTLDev();
    /**< These should be private but are called from C callback functions, must be public. */
    void imageGrabTask();
    void shutdown();

private:
    int SPConvertPixelFormat;
#define FIRST_SP_PARAM SPConvertPixelFormat
    int SPStartedFrameCount;
    int SPDeliveredFrameCount;
    int SPReceivedFrameCount;
    int SPIncompleteFrameCount;
    int SPLostFrameCount;
    int SPDroppedFrameCount;
    int SPInputBufferCount;
    int SPOutputBufferCount;
    int SPReceivedPacketCount;
    int SPMissedPacketCount;
    int SPResendRequestedPacketCount;
    int SPResendReceivedPacketCount;
    int SPTimeStampMode;
    int SPUniqueIdMode;
    int SPFrameRateEnable;

    /* Local methods to this class */
    asynStatus grabImage();
    asynStatus startCapture();
    asynStatus stopCapture();
    asynStatus connectCamera();
    asynStatus disconnectCamera();
    void reportNode(FILE *fp, const char *nodeName, int level);

    /* Data */
    int boardId_;
    #ifdef _WIN32
      Bd hBoard_;
      CircularInterface *pBoard_;
    #else
      tCIp hBoard_;
    #endif
    BFGTLDev hDevice_;
    int numBFBuffers_;
    int exiting_;
    epicsEventId startEventId_;
    epicsMessageQueue *pCallbackMsgQ_;
    NDArray *pRaw_;
    int uniqueId_;
};

#endif

