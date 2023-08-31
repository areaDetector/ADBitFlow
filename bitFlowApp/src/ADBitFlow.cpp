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
#include  "CircularInterface.h"
#include  "CiApi.h"
#include  "BiApi.h"
#include	"BFApi.h"
#include	"BFErApi.h"
#include	"DSApi.h"
#include	"BFGTLUtilApi.h"

#else
#include	"BFciLib.h"
#endif

using namespace std;
using namespace BufferAcquisition;

#include <ADGenICam.h>

#include <epicsExport.h>
#include "BFFeature.h"
#include "ADBitFlow.h"

#define DRIVER_VERSION      1
#define DRIVER_REVISION     0
#define DRIVER_MODIFICATION 0

static const char *driverName = "ADBitFlow";

// Size of message queue for callback function
#define CALLBACK_MESSAGE_QUEUE_SIZE 100

/** Configuration function to configure one camera.
 *
 * This function need to be called once for each camera to be used by the IOC. A call to this
 * function instantiates one object from the ADBitFlow class.
 * \param[in] portName asyn port name to assign to the camera.
 * \param[in] boardId The board number.  Default is 0.
 * \param[in] numBFBuffers The number of buffers to allocate in BitFlow driver.
 *            If set to 0 or omitted the default of 100 will be used.
 * \param[in] maxMemory Maximum memory (in bytes) that this driver is allowed to allocate. 0=unlimited.
 * \param[in] priority The EPICS thread priority for this driver.  0=use asyn default.
 * \param[in] stackSize The size of the stack for the EPICS port thread. 0=use asyn default.
 */
extern "C" int ADBitFlowConfig(const char *portName, int boardId, int numBFBuffers,
                               size_t maxMemory, int priority, int stackSize)
{
    new ADBitFlow( portName, boardId, numBFBuffers, maxMemory, priority, stackSize);
    return asynSuccess;
}


static void c_shutdown(void *arg)
{
   ADBitFlow *p = (ADBitFlow *)arg;
   p->shutdown();
}

static void imageGrabTaskC(void *drvPvt)
{
    ADBitFlow *pPvt = (ADBitFlow *)drvPvt;

    pPvt->imageGrabTask();
}

/** Constructor for the ADBitFlow class
 * \param[in] portName asyn port name to assign to the camera.
 * \param[in] boardId The board number.  Default is 0.
 * \param[in] numBFBuffers The number of buffers to allocate in BitFlow driver.
 *            If set to 0 or omitted the default of 100 will be used.
 * \param[in] maxMemory Maximum memory (in bytes) that this driver is allowed to allocate. 0=unlimited.
 * \param[in] priority The EPICS thread priority for this driver.  0=use asyn default.
 * \param[in] stackSize The size of the stack for the EPICS port thread. 0=use asyn default.
 */
ADBitFlow::ADBitFlow(const char *portName, int boardId, int numBFBuffers,
                         size_t maxMemory, int priority, int stackSize )
    : ADGenICam(portName, maxMemory, priority, stackSize),
    boardId_(boardId), pBoard_(0), hBoard_(0), hDevice_(0), numBFBuffers_(numBFBuffers), exiting_(0), pRaw_(NULL), uniqueId_(0)
{
    static const char *functionName = "ADBitFlow";
    asynStatus status;
    
    //pasynTrace->setTraceMask(pasynUserSelf, ASYN_TRACE_ERROR | ASYN_TRACE_WARNING | ASYN_TRACEIO_DRIVER);
    
    if (numBFBuffers_ == 0) numBFBuffers_ = 100;
    if (numBFBuffers_ < 10) numBFBuffers_ = 10;

    status = connectCamera();
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s:  camera connection failed (%d)\n",
            driverName, functionName, status);
        // Call report() to get a list of available cameras
        report(stdout, 1);
        return;
    }

/*
    createParam(SPConvertPixelFormatString,         asynParamInt32,   &SPConvertPixelFormat);
    createParam(SPStartedFrameCountString,          asynParamInt32,   &SPStartedFrameCount);
    createParam(SPDeliveredFrameCountString,        asynParamInt32,   &SPDeliveredFrameCount);
    createParam(SPReceivedFrameCountString,         asynParamInt32,   &SPReceivedFrameCount);
    createParam(SPIncompleteFrameCountString,       asynParamInt32,   &SPIncompleteFrameCount);
    createParam(SPLostFrameCountString,             asynParamInt32,   &SPLostFrameCount);
    createParam(SPDroppedFrameCountString,          asynParamInt32,   &SPDroppedFrameCount);
    createParam(SPInputBufferCountString,           asynParamInt32,   &SPInputBufferCount);
    createParam(SPOutputBufferCountString,          asynParamInt32,   &SPOutputBufferCount);
    createParam(SPReceivedPacketCountString,        asynParamInt32,   &SPReceivedPacketCount);
    createParam(SPMissedPacketCountString,          asynParamInt32,   &SPMissedPacketCount);
    createParam(SPResendRequestedPacketCountString, asynParamInt32,   &SPResendRequestedPacketCount);
    createParam(SPResendReceivedPacketCountString,  asynParamInt32,   &SPResendReceivedPacketCount);
    createParam(SPTimeStampModeString,              asynParamInt32,   &SPTimeStampMode);
    createParam(SPUniqueIdModeString,               asynParamInt32,   &SPUniqueIdMode);
*/
    /* Set initial values of some parameters */
    setIntegerParam(NDDataType, NDUInt8);
    setIntegerParam(NDColorMode, NDColorModeMono);
    setIntegerParam(NDArraySizeZ, 0);
    setIntegerParam(ADMinX, 0);
    setIntegerParam(ADMinY, 0);
    setStringParam(ADStringToServer, "<not used by driver>");
    setStringParam(ADStringFromServer, "<not used by driver>");

/*
    // Create the message queue to pass images from the callback class
    pCallbackMsgQ_ = new epicsMessageQueue(CALLBACK_MESSAGE_QUEUE_SIZE, sizeof(ImagePtr));
    if (!pCallbackMsgQ_) {
        cantProceed("ADBitFlow::ADBitFlow epicsMessageQueueCreate failure\n");
    }

    //pImageEventHandler_ = new ADBitFlowImageEventHandler(pCallbackMsgQ_);
    //pCamera_->RegisterEventHandler(*pImageEventHandler_);
*/
    startEventId_ = epicsEventCreate(epicsEventEmpty);

    // launch image read task
    epicsThreadCreate("ADBitFlowImageTask", 
                      epicsThreadPriorityMedium,
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      imageGrabTaskC, this);
    // shutdown on exit
    epicsAtExit(c_shutdown, this);

    return;
}

BFGTLDev ADBitFlow::getBFGTLDev() {
    return hDevice_;
}

void ADBitFlow::shutdown(void)
{
    static const char *functionName = "shutdown";
    
    lock();
    exiting_ = 1;
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
    pBoard_ = new CircularInterface(boardId_, numBFBuffers_, errorMode, cirSetupOptions);
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
    if (circ = CiVFGopen(0, kCIBO_writeAccess, &hBoard_)) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s error calling CiVFGopen, error=%s.\n",
            driverName, functionName, CiErrStr(circ));
        return asynError;
    }
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

void ADBitFlow::imageGrabTask()
{
    asynStatus status = asynSuccess;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    epicsTimeStamp startTime;
    int acquire;
    static const char *functionName = "imageGrabTask";

    lock();

    while (1) {
        // Is acquisition active? 
        getIntegerParam(ADAcquire, &acquire);
        // If we are not acquiring then wait for a semaphore that is given when acquisition is started 
        if (!acquire) {
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();

            // Wait for a signal that tells this thread that the transmission
            // has started and we can start asking for image buffers...
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s waiting for acquire to start\n", 
                driverName, functionName);
            // Release the lock while we wait for an event that says acquire has started, then lock again
            unlock();
            epicsEventWait(startEventId_);
            lock();
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s started!\n", 
                driverName, functionName);
            setIntegerParam(ADNumImagesCounter, 0);
            setIntegerParam(ADAcquire, 1);
        }

        // Get the current time 
        epicsTimeGetCurrent(&startTime);
        // We are now waiting for an image
        setIntegerParam(ADStatus, ADStatusWaiting);
        // Call the callbacks to update any changes
        callParamCallbacks();

        unlock();
        status = grabImage();
        lock();
/*
        if (status == asynError) {
            // remember to release the NDArray back to the pool now
            // that we are not using it (we didn't get an image...)
            if (pRaw_) pRaw_->release();
            pRaw_ = NULL;
            continue;
        }
*/
        getIntegerParam(NDArrayCounter, &imageCounter);
        getIntegerParam(ADNumImages, &numImages);
        getIntegerParam(ADNumImagesCounter, &numImagesCounter);
        getIntegerParam(ADImageMode, &imageMode);
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        imageCounter++;
        numImagesCounter++;
        setIntegerParam(NDArrayCounter, imageCounter);
        setIntegerParam(ADNumImagesCounter, numImagesCounter);

/*
        if (arrayCallbacks) {
            // Call the NDArray callback
            doCallbacksGenericPointer(pRaw_, NDArrayData, 0);
        }
        // Release the NDArray buffer now that we are done with it.
        // After the callback just above we don't need it anymore
        pRaw_->release();
        pRaw_ = NULL;

*/
        getIntegerParam(ADAcquire, &acquire);
        // See if acquisition is done if we are in single or multiple mode
        // The check for acquire=0 means this thread will call stopCapture and hence pCamera_->EndAcquisition().
        // Failure to do this result in hang in call to pCamera_->EndAcquisition() in other thread
        if ((acquire == 0) || 
            (imageMode == ADImageSingle) || 
            ((imageMode == ADImageMultiple) && (numImagesCounter >= numImages))) {
            setIntegerParam(ADStatus, ADStatusIdle);
            status = stopCapture();
        }
/*
        try {
            const TransportLayerStream& streamStats = pCamera_->TLStream;
            pTLStreamNodeMap_->InvalidateNodes();
            setIntegerParam(SPStartedFrameCount,          (int)streamStats.StreamStartedFrameCount.GetValue());
            setIntegerParam(SPDeliveredFrameCount,        (int)streamStats.StreamDeliveredFrameCount.GetValue());
            setIntegerParam(SPReceivedFrameCount,         (int)streamStats.StreamReceivedFrameCount.GetValue());
            setIntegerParam(SPIncompleteFrameCount,       (int)streamStats.StreamIncompleteFrameCount.GetValue());
            setIntegerParam(SPLostFrameCount,             (int)streamStats.StreamLostFrameCount.GetValue());
            setIntegerParam(SPDroppedFrameCount,          (int)streamStats.StreamDroppedFrameCount.GetValue());
            setIntegerParam(SPInputBufferCount,           (int)streamStats.StreamInputBufferCount.GetValue());
            setIntegerParam(SPOutputBufferCount,          (int)streamStats.StreamOutputBufferCount.GetValue());
            setIntegerParam(SPReceivedPacketCount,        (int)streamStats.StreamReceivedPacketCount.GetValue());
            setIntegerParam(SPMissedPacketCount,          (int)streamStats.StreamMissedPacketCount.GetValue());
            setIntegerParam(SPResendRequestedPacketCount, (int)streamStats.StreamPacketResendRequestedPacketCount.GetValue());
            setIntegerParam(SPResendReceivedPacketCount,  (int)streamStats.StreamPacketResendReceivedPacketCount.GetValue());
        }
        catch (Spinnaker::Exception &e) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s exception %s\n",
                driverName, functionName, e.what());
        }
*/
        callParamCallbacks();
    }
}

asynStatus ADBitFlow::grabImage()
{
    asynStatus status = asynSuccess;
    size_t nRows, nCols;
    NDDataType_t dataType;
    NDColorMode_t colorMode;
    int timeStampMode;
    int uniqueIdMode;
    int convertPixelFormat;
    bool imageConverted = false;
    int numColors;
    size_t dims[3];
    int pixelSize;
    size_t dataSize, dataSizePG;
    void *pData;
    int nDims;
    BiCirHandle cirHandle;
    static const char *functionName = "grabImage";
    
    try {
        //unlock();
        int status = pBoard_->waitDoneFrame(INFINITE, &cirHandle);
        switch (status) {
          case BI_CIR_STOPPED:
            printf("Circular acquisition stopped\n"); break;
          case BI_CIR_ABORTED:
            printf("Circular acquisition aborted\n"); break;
          case BI_ERROR_CIR_WAIT_TIMEOUT:
            printf("Circular wait timeout\n"); break;
          case BI_ERROR_CIR_WAIT_FAILED:
            printf("Circular wait failed\n"); break;
          case BI_ERROR_QEMPTY:
            printf("Circular queue was empty\n"); break;
        }
/*        
        pImage = *imagePtrAddr;
        // Delete the ImagePtr that was passed to us
        delete imagePtrAddr;
        imageStatus = pImage->GetImageStatus();
        if (imageStatus != SPINNAKER_IMAGE_STATUS_NO_ERROR) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s error GetImageStatus  %d, description:  %s\n",
                driverName, functionName, imageStatus, Image::GetImageStatusDescription(imageStatus));
            pImage->Release();
            return asynError;
        } 
        if (pImage->IsIncomplete()) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s error image is incomplete\n",
                driverName, functionName);
            pImage->Release();
            return asynError;
        }
        nCols = pImage->GetWidth();
        nRows = pImage->GetHeight();
        // Print the first 16 bytes of the buffer in hex
        //pData = pImage->GetData();
        //for (int i=0; i<16; i++) printf("%x ", ((epicsUInt8 *)pData)[i]); printf("\n");
     
        // Convert the pixel format if requested
        getIntegerParam(SPConvertPixelFormat, &convertPixelFormat);
        if (convertPixelFormat != SPPixelConvertNone) {
            PixelFormatEnums convertedFormat;
            switch (convertPixelFormat) {
                case SPPixelConvertMono8:
                    convertedFormat = PixelFormat_Mono8;
                    break;
                case SPPixelConvertMono16:
                    convertedFormat = PixelFormat_Mono16;
                    break;
                case SPPixelConvertRaw16:
                    convertedFormat = PixelFormat_Raw16;
                    break;
                case SPPixelConvertRGB8:
                    convertedFormat = PixelFormat_RGB8;
                    break;
                case SPPixelConvertRGB16:
                    convertedFormat = PixelFormat_RGB16;
                    break;
                default:
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s::%s Error: Unknown pixel conversion format %d\n",
                        driverName, functionName, convertPixelFormat);
                    convertedFormat = PixelFormat_Mono8;
                    break;
            }
    
            pixelFormat = pImage->GetPixelFormat();
            ImageProcessor processor; 
            unlock();
            try {
                //epicsTimeStamp tstart, tend;
                //epicsTimeGetCurrent(&tstart);
                pImage  = processor.Convert(pImage, convertedFormat);
                //epicsTimeGetCurrent(&tend);
                //asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s time for pImage->convert=%f\n", 
                //    driverName, functionName, epicsTimeDiffInSeconds(&tend, &tstart));
                imageConverted = true;
            }
            catch (Spinnaker::Exception &e) {
                 asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                     "%s::%s pixel format conversion exception %s\n",
                 driverName, functionName, e.what());
            }
            lock();
        }
    
        pixelFormat = pImage->GetPixelFormat();
        switch (pixelFormat) {
            case PixelFormat_Mono8:
            case PixelFormat_Raw8:
                dataType = NDUInt8;
                colorMode = NDColorModeMono;
                numColors = 1;
                pixelSize = 1;
                break;
    
            case PixelFormat_BayerGB8:
                dataType = NDUInt8;
                colorMode = NDColorModeBayer;
                numColors = 1;
                pixelSize = 1;
                break;
            case PixelFormat_RGB8:
                dataType = NDUInt8;
                colorMode = NDColorModeRGB1;
                numColors = 3;
                pixelSize = 1;
                break;
    
            case PixelFormat_Mono16:
            case PixelFormat_Raw16:
                dataType = NDUInt16;
                colorMode = NDColorModeMono;
                numColors = 1;
                pixelSize = 2;
                break;
    
            case PixelFormat_RGB16:
                dataType = NDUInt16;
                colorMode = NDColorModeRGB1;
                numColors = 3;
                pixelSize = 2;
                break;
    
            default:
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: unsupported pixel format=0x%x\n",
                    driverName, functionName, pixelFormat);
                return asynError;
        }
    
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
        dataSizePG = pImage->GetBufferSize();
        // Note, we should be testing for equality here.  However, there appears to be a bug in the
        // SDK when images are converted.  When converting from raw8 to mono8, for example, the
        // size returned by GetDataSize is the size of an RGB8 image, not a mono8 image.
        if (dataSize > dataSizePG) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: data size mismatch: calculated=%lu, reported=%lu\n",
                driverName, functionName, (long)dataSize, (long)dataSizePG);
            //return asynError;
        }
        setIntegerParam(NDArraySizeX, (int)nCols);
        setIntegerParam(NDArraySizeY, (int)nRows);
        setIntegerParam(NDArraySize, (int)dataSize);
        setIntegerParam(NDDataType,dataType);
        if (nDims == 3) {
            colorMode = NDColorModeRGB1;
        } 
        setIntegerParam(NDColorMode, colorMode);
    
        pRaw_ = pNDArrayPool->alloc(nDims, dims, dataType, 0, NULL);
        if (!pRaw_) {
            // If we didn't get a valid buffer from the NDArrayPool we must abort
            // the acquisition as we have nowhere to dump the data...
            setIntegerParam(ADStatus, ADStatusAborting);
            callParamCallbacks();
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s [%s] ERROR: Serious problem: not enough buffers left! Aborting acquisition!\n",
                driverName, functionName, portName);
            setIntegerParam(ADAcquire, 0);
            return(asynError);
        }
        pData = pImage->GetData();
        // Print the first 8 pixels of the buffer in decimal
        //for (int i=0; i<8; i++) printf("%u ", ((epicsUInt16 *)pData)[i]); printf("\n");
        if (pData) {
            memcpy(pRaw_->pData, pData, dataSize);
        } else {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s [%s] ERROR: pData is NULL!\n",
                driverName, functionName, portName);
            return asynError;
        }
    
        // Put the frame number into the buffer
        getIntegerParam(SPUniqueIdMode, &uniqueIdMode);
        if (uniqueIdMode == UniqueIdCamera) {
            pRaw_->uniqueId = (int)pImage->GetFrameID();
        } else {
            pRaw_->uniqueId = uniqueId_;
        }
        uniqueId_++;
        updateTimeStamp(&pRaw_->epicsTS);
        getIntegerParam(SPTimeStampMode, &timeStampMode);
        // Set the timestamps in the buffer
        if (timeStampMode == TimeStampCamera) {
            long long timeStamp = pImage->GetTimeStamp();
            if (timeStamp == 0) {
                asynPrint(pasynUserSelf, ASYN_TRACE_WARNING,
                    "%s::%s pImage->GetTimeStamp() returned 0\n",
                    driverName, functionName);
            }
            pRaw_->timeStamp = timeStamp / 1e9;
        } else {
            pRaw_->timeStamp = pRaw_->epicsTS.secPastEpoch + pRaw_->epicsTS.nsec/1e9;
        }
        try {
            // We get a "No Stream Available" exception if pImage points to an image resulting from ConvertPixeFormat
            // Not sure why?
            if (!imageConverted) {
                pImage->Release();
            } 
        }
        catch (Spinnaker::Exception &e) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s pImage->Release() exception %s\n",
                driverName, functionName, e.what());
        }
        // Get any attributes that have been defined for this driver        
        getAttributes(pRaw_->pAttributeList);
        
        // Change the status to be readout...
        setIntegerParam(ADStatus, ADStatusReadout);
        callParamCallbacks();
    
        pRaw_->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
        return status;
*/
        // Mark the buffer as available
        pBoard_->setBufferStatus(cirHandle, BIAVAILABLE);
    }

    catch (BFException e) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s exception %s\n",
            driverName, functionName, e.showErrorMsg());
        return asynError;
    }

    return asynSuccess;
}

asynStatus ADBitFlow::readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                               size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    //static const char *functionName = "readEnum";

    // There are a few enums we don't want to autogenerate the values
    if (function == SPConvertPixelFormat) {
        return asynError;
    }
    
    return ADGenICam::readEnum(pasynUser, strings, values, severities, nElements, nIn);
}


asynStatus ADBitFlow::startCapture()
{
    static const char *functionName = "startCapture";
    
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s entry\n", driverName, functionName);
    
    pBoard_->cirControl(BISTART, BiAsync);
    epicsEventSignal(startEventId_);
    return asynSuccess;
}

asynStatus ADBitFlow::stopCapture()
{
    int status;
    static const char *functionName = "stopCapture";

    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s entry\n", driverName, functionName);
    
    pBoard_->cirControl(BISTOP, BiAsync);

    // Set ADAcquire=0 which will tell the imageGrabTask to stop
    setIntegerParam(ADAcquire, 0);
    setShutter(0);

    // Need to wait for the imageGrabTask to set the status to idle
    while (1) {
        getIntegerParam(ADStatus, &status);
        if (status == ADStatusIdle) break;
        unlock();
        epicsThreadSleep(.1);
        lock();
    }

    return asynSuccess;
}

/*
void ADBitFlow::reportNode(FILE *fp, INodeMap *pNodeMap, gcstring nodeName, int level)
{
    gcstring displayName;
    gcstring value;
    CNodePtr pBase = (CNodePtr)pNodeMap->GetNode(nodeName);
    if (IsAvailable(pBase) && IsReadable(pBase)) {
        displayName = pBase->GetDisplayName();
        switch (pBase->GetPrincipalInterfaceType()) {
            case intfIString: {
                CStringPtr pNode = static_cast<CStringPtr>(pBase);
                value = pNode->GetValue();
                break;
            }
            case intfIInteger: {
                CIntegerPtr pNode = static_cast<CIntegerPtr>(pBase);
                value = pNode->ToString();
                break;
            }
            case intfIFloat: {
                CFloatPtr pNode = static_cast<CFloatPtr>(pBase);
                value = pNode->ToString();
                break;
                }
            case intfIBoolean: {
                CBooleanPtr pNode = static_cast<CBooleanPtr>(pBase);
                value = pNode->ToString();
                break;
                }
            case intfICommand: {
                CCommandPtr pNode = static_cast<CCommandPtr>(pBase);
                value = pNode->GetToolTip();
                break;
                }
            case intfIEnumeration: {
                CEnumerationPtr pNode = static_cast<CEnumerationPtr>(pBase);
                CEnumEntryPtr pEntry = pNode->GetCurrentEntry();
                value = pEntry->GetSymbolic();
                break;
               }
            default:
                value = "Unhandled data type";
                break;
        }
    }
    fprintf(fp, "%s (%s):%s\n", displayName.c_str(), nodeName.c_str(), value.c_str());
}
*/

/** Print out a report; calls ADGenICam::report to get base class report as well.
  * \param[in] fp File pointer to write output to
  * \param[in] details Level of detail desired.  If >1 prints information about 
               supported video formats and modes, etc.
 */

void ADBitFlow::report(FILE *fp, int details)
{
    int numCameras;
    int i;
    static const char *functionName = "report";

/*
    try {    
        numCameras = camList_.GetSize();
        fprintf(fp, "\n");
        fprintf(fp, "Number of cameras detected: %d\n", numCameras);
        if (details <1) return;
        for (i=0; i<numCameras; i++) {
            CameraPtr pCamera;
            pCamera = camList_.GetByIndex(i);
            INodeMap *pNodeMap = &pCamera->GetTLDeviceNodeMap();
    
            fprintf(fp, "Camera %d\n", i);
            reportNode(fp, pNodeMap, "DeviceVendorName", 1);
            reportNode(fp, pNodeMap, "DeviceModelName", 1);
            reportNode(fp, pNodeMap, "DeviceSerialNumber", 1);
            reportNode(fp, pNodeMap, "DeviceVersion", 1);
            reportNode(fp, pNodeMap, "DeviceType", 1);
        }
    }
    catch (Spinnaker::Exception &e) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
          "%s::%s exception %s\n",
          driverName, functionName, e.what());
    }
*/    
    fprintf(fp, "\n");
    fprintf(fp, "Report for camera in use:\n");
    ADGenICam::report(fp, details);
    return;
}

static const iocshArg configArg0 = {"Port name", iocshArgString};
static const iocshArg configArg1 = {"boardId", iocshArgInt};
static const iocshArg configArg2 = {"# BitFlow buffers", iocshArgInt};
static const iocshArg configArg3 = {"maxMemory", iocshArgInt};
static const iocshArg configArg4 = {"priority", iocshArgInt};
static const iocshArg configArg5 = {"stackSize", iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3,
                                              &configArg4,
                                              &configArg5};
static const iocshFuncDef configADBitFlow = {"ADBitFlowConfig", 6, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
    ADBitFlowConfig(args[0].sval, args[1].ival, args[2].ival, 
                      args[3].ival, args[4].ival, args[5].ival);
}


static void ADBitFlowRegister(void)
{
    iocshRegister(&configADBitFlow, configCallFunc);
}

extern "C" {
epicsExportRegistrar(ADBitFlowRegister);
}

