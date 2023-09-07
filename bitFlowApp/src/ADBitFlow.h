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

#define BFTimeStampModeString               "BF_TIME_STAMP_MODE"                // asynParamInt32, R/O
#define BFUniqueIdModeString                "BF_UNIQUE_ID_MODE"                 // asynParamInt32, R/O
#define BFBufferSizeString                  "BF_BUFFER_SIZE"                    // asynParamInt32, R/O
#define BFBufferQueueSizeString             "BF_BUFFER_QUEUE_SIZE"              // asynParamInt32, R/O
#define BFMessageQueueSizeString            "BF_MESSAGE_QUEUE_SIZE"             // asynParamInt32, R/O
#define BFMessageQueueFreeString            "BF_MESSAGE_QUEUE_FREE"             // asynParamInt32, R/O
#define BFProcessTotalTimeString            "BF_PROCESS_TOTAL_TIME"             // asynParamFloat64, R/O
#define BFProcessCopyTimeString             "BF_PROCESS_COPY_TIME"              // asynParamFloat64, R/O

/** Main driver class inherited from areaDetectors ADDriver class.
 * One instance of this class will control one camera.
 */
class ADBitFlow : public ADGenICam
{
public:
    ADBitFlow(const char *portName, int cameraId, int numSPBuffers, int numThreads,
              size_t maxMemory, int priority, int stackSize);

    // virtual methods to override from ADGenICam
    virtual asynStatus writeInt32( asynUser *pasynUser, epicsInt32 value);
    //virtual asynStatus writeFloat64( asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                                size_t nElements, size_t *nIn);
    void report(FILE *fp, int details);
    virtual GenICamFeature *createFeature(GenICamFeatureSet *set, 
                                          std::string const & asynName, asynParamType asynType, int asynIndex,
                                          std::string const & featureName, GCFeatureType_t featureType);
    
    BFGTLDev getBFGTLDev();
    /**< These should be private but are called from C callback functions, must be public. */
    void waitImageThread();
    void processImageThread();
    void shutdown();

private:
    int BFTimeStampMode;
#define FIRST_BF_PARAM BFTimeStampMode
    int BFUniqueIdMode;
    int BFBufferSize;
    int BFBufferQueueSize;
    int BFMessageQueueSize;
    int BFMessageQueueFree;
    int BFProcessTotalTime;
    int BFProcessCopyTime;

    /* Local methods to this class */
    asynStatus grabImage();
    asynStatus startCapture();
    asynStatus stopCapture();
    asynStatus connectCamera();
    asynStatus disconnectCamera();
    asynStatus setROI();
    void reportNode(FILE *fp, const char *nodeName, int level);

    /* Data */
    int boardNum_;
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
    epicsMessageQueue *pMsgQ_;
    int messageQueueSize_;
    int uniqueId_;
};

#endif

