/*
 * ADBitFlow.cpp
 *
 * This is a driver for BitFlow frame grabbers
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created: August 28, 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <set>
#include <string>

#include <epicsEvent.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <iocsh.h>
#include <cantProceed.h>
#include <epicsString.h>
#include <epicsExit.h>

#ifdef _WIN32
#include "CircularInterface.h"
#include "CiApi.h"
#include "BiApi.h"
#include "BFApi.h"
#include "BFErApi.h"
#include "DSApi.h"
#include "BFGTLUtilApi.h"
using namespace BufferAcquisition;

#else
#include "BFciLib.h"
#endif

using namespace std;

#include <ADGenICam.h>

#include <epicsExport.h>
#include "BFFeature.h"
#include "ADBitFlow.h"

#define DRIVER_VERSION      1
#define DRIVER_REVISION     0
#define DRIVER_MODIFICATION 0

static const char *driverName = "ADBitFlow";

typedef enum {
    TimeStampCamera,
    TimeStampEPICS
} BFTimeStamp_t;

typedef enum {
    UniqueIdCamera,
    UniqueIdDriver
} BFUniqueId_t;

/** Configuration function to configure one camera.
 *
 * This function need to be called once for each camera to be used by the IOC. A call to this
 * function instantiates one object from the ADBitFlow class.
 * \param[in] portName asyn port name to assign to the camera.
 * \param[in] boardNum The board number.  Default is 0.
 * \param[in] numBFBuffers The number of buffers to allocate in BitFlow driver.
 *            If set to 0 or omitted the default of 100 will be used.
 * \param(in) numThreads Number of image processing threads.  If set to 0 or omitted 2 will be used.
 * \param[in] maxMemory Maximum memory (in bytes) that this driver is allowed to allocate. 0=unlimited.
 * \param[in] priority The EPICS thread priority for this driver.  0=use asyn default.
 * \param[in] stackSize The size of the stack for the EPICS port thread. 0=use asyn default.
 */
extern "C" int ADBitFlowConfig(const char *portName, int boardNum, int numBFBuffers, int numThreads,
                               size_t maxMemory, int priority, int stackSize)
{
    new ADBitFlow(portName, boardNum, numBFBuffers, numThreads, maxMemory, priority, stackSize);
    return asynSuccess;
}

struct workerQueueElement {
    #ifdef _WIN32
      BiCirHandle cirHandle;
    #else
      tCIU32 frameID;
      tCIU8 *pFrame;
    #endif
    int uniqueId;
};

static void c_shutdown(void *arg)
{
   ADBitFlow *p = (ADBitFlow *)arg;
   p->shutdown();
}

static void waitImageThreadC(void *drvPvt)
{
    ADBitFlow *pPvt = (ADBitFlow *)drvPvt;

    pPvt->waitImageThread();
}

static void processImageThreadC(void *drvPvt)
{
    ADBitFlow *pPvt = (ADBitFlow *)drvPvt;

    pPvt->processImageThread();
}


/** Constructor for the ADBitFlow class
 * \param[in] portName asyn port name to assign to the camera.
 * \param[in] boardNum The board number.  Default is 0.
 * \param[in] numBFBuffers The number of buffers to allocate in BitFlow driver.
 *            If set to 0 or omitted the default of 100 will be used.
 * \param(in) numThreads Number of image processing threads.  If set to 0 or omitted 2 will be used.
 * \param[in] maxMemory Maximum memory (in bytes) that this driver is allowed to allocate. 0=unlimited.
 * \param[in] priority The EPICS thread priority for this driver.  0=use asyn default.
 * \param[in] stackSize The size of the stack for the EPICS port thread. 0=use asyn default.
 */
ADBitFlow::ADBitFlow(const char *portName, int boardNum, int numBFBuffers, int numThreads,
                         size_t maxMemory, int priority, int stackSize )
    : ADGenICam(portName, maxMemory, priority, stackSize),
    boardNum_(boardNum), hBoard_(0), pBoard_(0), hDevice_(0), numBFBuffers_(numBFBuffers), exiting_(0), uniqueId_(0)
{
    static const char *functionName = "ADBitFlow";
    asynStatus status;
    
    //pasynTrace->setTraceMask(pasynUserSelf, ASYN_TRACE_ERROR | ASYN_TRACE_WARNING | ASYN_TRACEIO_DRIVER);
    
    if (numBFBuffers_ == 0) numBFBuffers_ = 100;
    if (numBFBuffers_ < 10) numBFBuffers_ = 10;
    messageQueueSize_ = numBFBuffers;
    if (numThreads <= 0) numThreads = 2;

    status = connectCamera();
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s:  camera connection failed (%d)\n",
            driverName, functionName, status);
        // Call report() to get a list of available cameras
        report(stdout, 1);
        return;
    }

    createParam(BFTimeStampModeString,              asynParamInt32,   &BFTimeStampMode);
    createParam(BFUniqueIdModeString,               asynParamInt32,   &BFUniqueIdMode);
    createParam(BFBufferSizeString,                 asynParamInt32,   &BFBufferSize);
    createParam(BFBufferQueueSizeString,            asynParamInt32,   &BFBufferQueueSize);
    createParam(BFMessageQueueSizeString,           asynParamInt32,   &BFMessageQueueSize);
    createParam(BFMessageQueueFreeString,           asynParamInt32,   &BFMessageQueueFree);
    createParam(BFProcessTotalTimeString,         asynParamFloat64,   &BFProcessTotalTime);
    createParam(BFProcessCopyTimeString,          asynParamFloat64,   &BFProcessCopyTime);

    /* Set initial values of some parameters */
    setIntegerParam(BFBufferSize, numBFBuffers);
    setIntegerParam(BFBufferQueueSize, 0);
    setIntegerParam(BFMessageQueueSize, messageQueueSize_);
    setIntegerParam(BFMessageQueueFree, messageQueueSize_);
    setIntegerParam(NDDataType, NDUInt8);
    setIntegerParam(NDColorMode, NDColorModeMono);
    setIntegerParam(NDArraySizeZ, 0);
    setIntegerParam(ADMinX, 0);
    setIntegerParam(ADMinY, 0);
    setStringParam(ADStringToServer, "<not used by driver>");
    setStringParam(ADStringFromServer, "<not used by driver>");
    
    // Create the message queue to pass images to the worker threads
    // The number of queue elements is the number of buffers
    pMsgQ_ = new epicsMessageQueue(messageQueueSize_, sizeof(workerQueueElement));
    if (!pMsgQ_) {
        cantProceed("ADBitFlow::ADBitFlow epicsMessageQueueCreate failure\n");
    }

    startEventId_ = epicsEventCreate(epicsEventEmpty);

    // Launch the thread that waits for images
    epicsThreadCreate("ADBFWaitImageThread", 
                      epicsThreadPriorityHigh,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      waitImageThreadC, this);
        

    // Launch the threads that process images
    for (int i=0; i<numThreads; i++) {
        epicsThreadCreate("ADBFProcessImageThread", 
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          processImageThreadC, this);
    }

    // shutdown on exit
    epicsAtExit(c_shutdown, this);

    return;
}

BFGTLDev ADBitFlow::getBFGTLDev() {
    return hDevice_;
}

void ADBitFlow::shutdown(void)
{
    //static const char *functionName = "shutdown";
    
    lock();
    exiting_ = 1;
    stopCapture();
    #ifdef _WIN32
      delete pBoard_;
    #else
      CiVFGclose(hBoard_);
    #endif
    unlock();
}

GenICamFeature *ADBitFlow::createFeature(GenICamFeatureSet *set, 
                                           std::string const & asynName, asynParamType asynType, int asynIndex,
                                           std::string const & featureName, GCFeatureType_t featureType) {
    return new BFFeature(set, asynName, asynType, asynIndex, featureName, featureType);
}

asynStatus ADBitFlow::connectCamera(void)
{
    char SDKVersionString[256];
    char driverVersionString[256];
    static const char *functionName = "connectCamera";

#ifdef _WIN32
    // Open the board using default camera file
    BFU32 cirSetupOptions = 0;
    BFU32 errorMode = CirErStop;
    pBoard_ = new CircularInterface(boardNum_, numBFBuffers_, errorMode, cirSetupOptions);
    if (!pBoard_) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s error calling BiBrdOpen, could not open board.\n",
            driverName, functionName);
        return asynError;
    }
    hBoard_ = pBoard_->getBoardHandle();
    
    char modelStr[MAX_STRING];
    char familyStr[MAX_STRING];
    char camName[MAX_STRING];
    unsigned int familyIndex;
    unsigned int ciFamily;
    BFGetBoardStrings(hBoard_, modelStr, MAX_STRING, familyStr, MAX_STRING, &familyIndex, &ciFamily);
    printf("Board \"%s - %s\" has been opened.\n", familyStr, modelStr);
    printf("Attached camera list:\n");
    int i = 0;
    while (CiBrdCamGetFileName(hBoard_, i, camName, sizeof(camName)) == CI_OK) {
        printf("%d - %s\n", i, camName);
        i++;
    }
    printf("Board is acquiring from camera\n");
    int ready;
    CiConIsCameraReady(hBoard_, &ready);
    if (ready)
        printf("System reports camera is ready\n");
    else
        printf("System reports camera not ready\n");

    if (BFIsCXP(hBoard_)) {
        // BFGTLUtil open device
        if (BFGTLDevOpen(hBoard_, &hDevice_) != BF_OK) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s calling BFGTLDevOpen\n",
                driverName, functionName);
         }
         printf("%s::%s hBoard_=%p, hDevice_=%p\n", driverName, functionName, hBoard_, hDevice_);
     }
    unsigned int versionMajor, versionMinor;
    BiDVersion(&versionMajor, &versionMinor);
    epicsSnprintf(SDKVersionString, sizeof(SDKVersionString), "%d.%d", versionMajor, versionMinor);

#else
    tCIRC circ;
    if ((circ = CiVFGopen(0, kCIBO_writeAccess, &hBoard_))) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s error calling CiVFGopen, error=%s.\n",
            driverName, functionName, CiErrStr(circ));
        return asynError;
    }
    tCIU32 libVers, drvVers;
    CiSysGetVersions(&libVers, &drvVers);
    epicsSnprintf(SDKVersionString, sizeof(SDKVersionString), "%d.%d", libVers, drvVers);
    
#endif
    epicsSnprintf(driverVersionString, sizeof(driverVersionString), "%d.%d.%d", 
                  DRIVER_VERSION, DRIVER_REVISION, DRIVER_MODIFICATION);
    setStringParam(NDDriverVersion,driverVersionString);
 
    setStringParam(ADSDKVersion, SDKVersionString);

    return asynSuccess;
}


/** Task to grab images off the camera and send them up to areaDetector
 *
 */

void ADBitFlow::waitImageThread()
{
    int BFStatus, BFStatus1;
    int numImages;
    int imageMode;
    int imagesCollected;
    bool waitingForImages = false;
    static const char *functionName = "waitImageThread";

    lock();

    while (1) {
        if (!waitingForImages) {
            // Wait for a signal that tells this thread that acquisition
            // has started and we can start reading image buffers...
            asynPrint(pasynUserSelf, ASYN_TRACE_WARNING,
                "%s::%s waiting for acquire to start\n", 
                driverName, functionName);
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();
            // Release the lock while we wait for an event that says acquire has started, then lock again
            unlock();
            epicsEventWait(startEventId_);
            lock();
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s started!\n", 
                driverName, functionName);
            setIntegerParam(ADNumImagesCounter, 0);
            setIntegerParam(ADAcquire, 1);
            getIntegerParam(ADNumImages, &numImages);
            getIntegerParam(ADImageMode, &imageMode);
            imagesCollected = 0;
            waitingForImages = true;
        }

        // We are now waiting for an image
        setIntegerParam(ADStatus, ADStatusWaiting);

        // Call the callbacks to update any changes
        callParamCallbacks();

        asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s waiting for frame\n", driverName, functionName);
#ifdef _WIN32
        BiCirHandle cirHandle;
        unlock();
        BFStatus = pBoard_->waitDoneFrame(INFINITE, &cirHandle);
        asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s got frame status=%d, bufferNumber=%d\n", 
                  driverName, functionName, BFStatus, cirHandle.BufferNumber);
        lock();
        switch (BFStatus) {
          case BI_OK: {
              // Mark the buffer to hold
              asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s calling setBufferStatus for BIHOLD\n", driverName, functionName);
              BFStatus1 = pBoard_->setBufferStatus(cirHandle, BIHOLD);
              asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s setBufferStatus returned %d\n", driverName, functionName, BFStatus1);
              // Send a message to the processing thread
              struct workerQueueElement wqe{cirHandle, uniqueId_};
              uniqueId_++;
              asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s sending message\n", driverName, functionName);
              if (pMsgQ_->send(&wqe, sizeof(wqe)) != 0) {
                  asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error calling pMsgQ_->send()\n", driverName, functionName);
              }
              imagesCollected++;
              if ((imageMode == ADImageSingle) || ((imageMode == ADImageMultiple) && (imagesCollected >= numImages))) {
                  pBoard_->cirControl(BISTOP, BiAsync);
                  waitingForImages = false;
              }
            }
            break;
          case BI_CIR_STOPPED:
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s::%s Circular acquisition stopped\n",
                      driverName, functionName);
            waitingForImages = false;
            break;
          case BI_CIR_ABORTED:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Circular acquisition aborted\n",
                       driverName, functionName);
             setIntegerParam(ADStatus, ADStatusIdle);
             stopCapture();
             waitingForImages = false;
             break;
          case BI_ERROR_CIR_WAIT_TIMEOUT:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Circular wait timeout\n",
                       driverName, functionName);
             break;
          case BI_ERROR_CIR_WAIT_FAILED:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Circular wait failed\n",
                       driverName, functionName);
             break;
          case BI_ERROR_QEMPTY:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Circular queue was empty\n",
                       driverName, functionName);
             break;
          default:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Unknown status return from waitDoneFrame = %d\n",
                       driverName, functionName, BFStatus);
             break;
        }
#else
        tCIU32 frameID;
        tCIU8 *pFrame;
        BFStatus = CiGetOldestNotDeliveredFrame(hBoard_, &frameID, &pFrame);
        switch (BFStatus) {
          case kCIEnoErr: {
              // Mark the buffer to hold
              //stat = pBoard_->setBufferStatus(cirHandle, BIHOLD);
              // Send a message to the processing thread
              struct workerQueueElement wqe{frameID, pFrame, uniqueId_};
              uniqueId_++;
              asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s sending message\n", driverName, functionName);
              if (pMsgQ_->send(&wqe, sizeof(wqe)) != 0) {
                  asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error calling pMsgQ_->send()\n", driverName, functionName);
              }
              imagesCollected++;
              if ((imageMode == ADImageSingle) || ((imageMode == ADImageMultiple) && (imagesCollected >= numImages))) {
                  CiAqAbort(hBoard_);
                  waitingForImages = false;
              }
            }
            break;
          case kCIEnoNewData:
            BFStatus1 = CiWaitNextUndeliveredFrame(hBoard_, -1);
            if (BFStatus1 == kCIEnoErr) break;
            if (BFStatus1 != kCIEaqAbortedErr) {
              asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s::%s Unknown status return from CiGetNextUndeliveredFrame = %d\n",
                        driverName, functionName, BFStatus1);
              break;
            }
            // Fall through if aborted
          case kCIEaqAbortedErr:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Circular acquisition aborted\n",
                       driverName, functionName);
             setIntegerParam(ADStatus, ADStatusIdle);
             stopCapture();
             waitingForImages = false;
             break;
          default:
             asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s::%s Unknown status return from CiGetOldestNotDeliveredFrame = %d\n",
                       driverName, functionName, BFStatus);
             break;
        }
#endif
    }
}

void ADBitFlow::processImageThread()
{
    NDArray *pRaw = 0;
    size_t nRows, nCols;
    NDDataType_t dataType = NDUInt8;
    NDColorMode_t colorMode = NDColorModeMono;
    int timeStampMode;
    int uniqueIdMode;
    int numColors=1;
    size_t dims[3];
    int pixelSize;
    size_t dataSize;
    unsigned int frameSize;
    void *pData;
    int nDims;
    int numImages;
    int imageCounter;
    int numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    epicsTime t1, t2, t3, t4;
    struct workerQueueElement wqe;
    static const char *functionName = "processImageThread";

    while (true) {
        unlock();
        int recvSize = pMsgQ_->receive(&wqe, sizeof(wqe));
        t1=t2=t3=t4 = epicsTime::getCurrent();
        lock();
        if (recvSize != sizeof(wqe)) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s error receiving from message queue\n",
                    driverName, functionName);
            continue;
        }
        // If acquisition has stopped then ignore this frame
        //getIntegerParam(ADAcquire, &acquire);
        //if (!acquire) continue;

#ifdef _WIN32
        BiCirHandle cirHandle;
        cirHandle = wqe.cirHandle;
        pData = cirHandle.pBufData;
        
        // Get the image information
        nCols = pBoard_->getBrdInfo(BiCamInqXSize);
        nRows = pBoard_->getBrdInfo(BiCamInqYSize0);
        pixelSize = pBoard_->getBrdInfo(BiCamInqBytesPerPix);
        frameSize = pBoard_->getBrdInfo(BiCamInqFrameSize0);
        asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, 
                  "%s::%s nCols=%u, nRows=%u, pixelSize=%u, frameSize=%u, BufferNumber=%u, FrameCount=%u, NumItemsOnQueue=%u\n",
                  driverName, functionName, nCols, nRows, pixelSize, frameSize, cirHandle.BufferNumber, cirHandle.FrameCount, cirHandle.NumItemsOnQueue);
#else
        int iTemp;
        getIntegerParam(ADSizeX, &iTemp);
        nCols = iTemp;
        getIntegerParam(ADSizeY, &iTemp);
        nRows = iTemp;
        pixelSize = bitsPerPixel_/8;
        frameSize = nCols * nRows * pixelSize;
        pData = wqe.pFrame;
#endif

        if (numColors == 1) {
            nDims = 2;
            dims[0] = nCols;
            dims[1] = nRows;
        } else {
            nDims = 3;
            dims[0] = 3;
            dims[1] = nCols;
            dims[2] = nRows;
        }
        dataSize = dims[0] * dims[1] * pixelSize;
        if (nDims == 3) dataSize *= dims[2];
        if (dataSize != frameSize) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: data size mismatch: calculated=%lu, reported=%lu\n",
                driverName, functionName, (long)dataSize, (long)frameSize);
        }
        setIntegerParam(NDArraySizeX, (int)nCols);
        setIntegerParam(NDArraySizeY, (int)nRows);
        setIntegerParam(NDArraySize, (int)dataSize);
        setIntegerParam(NDDataType, dataType);
        if (nDims == 3) {
            colorMode = NDColorModeRGB1;
        } 
        setIntegerParam(NDColorMode, colorMode);
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        if (arrayCallbacks) {
            asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s allocating pRaw\n", driverName, functionName);
            pRaw = pNDArrayPool->alloc(nDims, dims, dataType, 0, NULL);
            if (!pRaw) {
                // If we didn't get a valid buffer from the NDArrayPool we must abort
                // the acquisition as we have nowhere to dump the data...
                setIntegerParam(ADStatus, ADStatusAborting);
                callParamCallbacks();
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s::%s [%s] ERROR: Serious problem: not enough buffers left! Aborting acquisition!\n",
                    driverName, functionName, portName);
                setIntegerParam(ADAcquire, 0);
                continue;
            }
            if (pData) {
                asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s copying data\n", driverName, functionName);
                unlock();
                t2 = epicsTime::getCurrent();
                memcpy(pRaw->pData, pData, dataSize);
                t3 = epicsTime::getCurrent();
                lock();
            } else {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s::%s [%s] ERROR: pData is NULL!\n",
                    driverName, functionName, portName);
                continue;
            }
        
            // Put the frame number into the buffer
            getIntegerParam(BFUniqueIdMode, &uniqueIdMode);
            if (uniqueIdMode == UniqueIdCamera) {
#ifdef _WIN32
                pRaw->uniqueId = cirHandle.FrameCount;
#else
                pRaw->uniqueId = wqe.frameID;
#endif         
            } else {
                pRaw->uniqueId = wqe.uniqueId;
            }
            updateTimeStamp(&pRaw->epicsTS);
            getIntegerParam(BFTimeStampMode, &timeStampMode);
            // Set the timestamps in the buffer
            if (timeStampMode == TimeStampCamera) {
#ifdef _WIN32
                // Should use cirHandle.HiResTimeStamp but its fields are all zero?
                pRaw->timeStamp = cirHandle.TimeStamp.hour*3600 + 
                                  cirHandle.TimeStamp.min*60 + 
                                  cirHandle.TimeStamp.sec +
                                  cirHandle.TimeStamp.msec/1000.;
#else
                tCIextraFrameInfo extraInfo;
                extraInfo.frameID = wqe.frameID;
                CiGetExtraFrameInfo(hBoard_, sizeof(extraInfo), &extraInfo);
                pRaw->timeStamp = extraInfo.timestamp;
#endif
            } else {
                pRaw->timeStamp = pRaw->epicsTS.secPastEpoch + pRaw->epicsTS.nsec/1e9;
            }
    
            // Get any attributes that have been defined for this driver        
            getAttributes(pRaw->pAttributeList);
            
            // Change the status to be readout...
            setIntegerParam(ADStatus, ADStatusReadout);
        
            pRaw->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
        }

        // Mark the buffer as available
        asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s marking buffer as available\n", driverName, functionName);
#ifdef _WIN32
        pBoard_->setBufferStatus(cirHandle, BIAVAILABLE);
#else
        unsigned int bufferID;
        CiGetBufferID(hBoard_, wqe.frameID, &bufferID);
        CiReleaseBuffer(hBoard_, bufferID);
#endif
        getIntegerParam(NDArrayCounter, &imageCounter);
        getIntegerParam(ADNumImages, &numImages);
        getIntegerParam(ADNumImagesCounter, &numImagesCounter);
        getIntegerParam(ADImageMode, &imageMode);
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        imageCounter++;
        numImagesCounter++;
        setIntegerParam(NDArrayCounter, imageCounter);
        setIntegerParam(ADNumImagesCounter, numImagesCounter);

        if (arrayCallbacks) {
            // Call the NDArray callback
            asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s calling doCallbacksGenericPointer\n", driverName, functionName);
            doCallbacksGenericPointer(pRaw, NDArrayData, 0);
            // Release the NDArray buffer now that we are done with it.
            // After the callback just above we don't need it anymore
            //asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s releasing pRaw\n", driverName, functionName);
            if (pRaw) pRaw->release();
            pRaw = NULL;
        }

        // See if acquisition is done if we are in single or multiple mode
        if ((imageMode == ADImageSingle) ||
            ((imageMode == ADImageMultiple) && (numImagesCounter >= numImages))) {
            setIntegerParam(ADStatus, ADStatusIdle);
            asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s calling stopCapture\n", driverName, functionName);
            stopCapture();
        }
        t4 = epicsTime::getCurrent();
        setDoubleParam(BFProcessTotalTime, (t4-t1)*1000.);
        setDoubleParam(BFProcessCopyTime, (t3-t2)*1000.);
        setIntegerParam(BFMessageQueueFree, messageQueueSize_ - pMsgQ_->pending());
#ifdef _WIN32
        setIntegerParam(BFBufferQueueSize, cirHandle.NumItemsOnQueue);
#else
        // Is this information available in Linux?
#endif
        callParamCallbacks();
    }
}

asynStatus ADBitFlow::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr;
    static const char *functionName = "writeInt32";
  
    printf("%s::%s function=%d, value=%d\n", driverName, functionName, function, value);
    this->getAddress(pasynUser, &addr);
    if ((function == ADSizeX) ||
        (function == ADSizeY) ||
        (function == ADMinX)  ||
        (function == ADMinY)) {

        setIntegerParam(addr, function, value);
        return this->setROI();
    }
    return ADGenICam::writeInt32(pasynUser, value);
}

asynStatus ADBitFlow::setROI() 
{
    int minX, minY, sizeX, sizeY;
    static const char *functionName = "setROI";

    getIntegerParam(ADMinX, &minX);
    getIntegerParam(ADMinY, &minY);
    getIntegerParam(ADSizeX, &sizeX);
    getIntegerParam(ADSizeY, &sizeY);
#ifdef _WIN32
    try {
        printf("%s::%s calling setAcqROI(%d, %d, %d, %d)\n", driverName, functionName, minX, minY, sizeX, sizeY);
        pBoard_->clearBuffers();
        pBoard_->setAcqROI(minX, minY, sizeX, sizeY);
    }
    catch (BFException e) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error calling setAcqROI error=%s\n",
                  driverName, functionName, e.showErrorMsg());
        return asynError;
    }
#else
    int BFStatus;
    BFStatus = CiDrvrBuffConfigure(hBoard_, 0, minX, sizeX, minY, sizeY);
    BFStatus = CiDrvrBuffConfigure(hBoard_, numBFBuffers_, minX, sizeX, minY, sizeY);
    if (BFStatus) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error calling CiDrvrBuffConfigure error=%d\n",
                  driverName, functionName, BFStatus);
        return asynError;
    }
    tCIU32	nFrames, bitsPerPix, hROIoffset, hROIsize, vROIoffset, vROIsize, stride;
    BFStatus = CiBufferInterrogate(hBoard_, &nFrames, &bitsPerPix, &hROIoffset, &hROIsize, &vROIoffset, &vROIsize, &stride);
    if (BFStatus) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error calling CiBufferInterrogate error=%d\n",
                  driverName, functionName, BFStatus);
        return asynError;
    }
    setIntegerParam(ADMinX, hROIoffset);
    setIntegerParam(ADMinY, vROIoffset);
    setIntegerParam(ADSizeX, hROIsize);    
    setIntegerParam(ADSizeY, vROIsize);
    bitsPerPixel_ = bitsPerPix;
#endif
    return asynSuccess;
}

asynStatus ADBitFlow::startCapture()
{
    static const char *functionName = "startCapture";
    
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s entry\n", driverName, functionName);
    
    GenICamFeature *acquisitionStart = mGCFeatureSet.getByName("AcquisitionStart");
    acquisitionStart->writeCommand();
#ifdef _WIN32
    pBoard_->cirControl(BISTART, BiAsync);
#else
    CiAqStart(hBoard_, -1);
#endif
    epicsEventSignal(startEventId_);
    return asynSuccess;
}

asynStatus ADBitFlow::stopCapture()
{
    static const char *functionName = "stopCapture";

    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s entry\n", driverName, functionName);
#ifdef _WIN32
    pBoard_->cirControl(BISTOP, BiAsync);
#else
    CiAqStop(hBoard_);
#endif
    epicsThreadSleep(1.0);
    GenICamFeature *acquisitionStop = mGCFeatureSet.getByName("AcquisitionStop");
    acquisitionStop->writeCommand();

    // Set ADAcquire=0 which will tell the imageGrabTask to stop
    setIntegerParam(ADAcquire, 0);
    setShutter(0);

    return asynSuccess;
}

/** Print out a report; calls ADGenICam::report to get base class report as well.
  * \param[in] fp File pointer to write output to
  * \param[in] details Level of detail desired.  If >1 prints information about 
               supported video formats and modes, etc.
 */

void ADBitFlow::report(FILE *fp, int details)
{
    //static const char *functionName = "report";

    fprintf(fp, "\n");
    fprintf(fp, "Report for camera in use:\n");
    ADGenICam::report(fp, details);
    return;
}

static const iocshArg configArg0 = {"Port name", iocshArgString};
static const iocshArg configArg1 = {"boardNum", iocshArgInt};
static const iocshArg configArg2 = {"# BitFlow buffers", iocshArgInt};
static const iocshArg configArg3 = {"# processing threads", iocshArgInt};
static const iocshArg configArg4 = {"maxMemory", iocshArgInt};
static const iocshArg configArg5 = {"priority", iocshArgInt};
static const iocshArg configArg6 = {"stackSize", iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3,
                                              &configArg4,
                                              &configArg5,
                                              &configArg6};
static const iocshFuncDef configADBitFlow = {"ADBitFlowConfig", 7, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
    ADBitFlowConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival, 
                    args[4].ival, args[5].ival, args[6].ival);
}


static void ADBitFlowRegister(void)
{
    iocshRegister(&configADBitFlow, configCallFunc);
}

extern "C" {
epicsExportRegistrar(ADBitFlowRegister);
}

