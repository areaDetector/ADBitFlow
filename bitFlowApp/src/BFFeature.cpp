// BFFeature.cpp
// Mark Rivers
// August 26, 2023

#include <BFFeature.h>
#include <ADBitFlow.h>

static const char *driverName="BFFeature";

BFFeature::BFFeature(GenICamFeatureSet *set, 
                     std::string const & asynName, asynParamType asynType, int asynIndex,
                     std::string const & featureName, GCFeatureType_t featureType)
                     
         : GenICamFeature(set, asynName, asynType, asynIndex, featureName, featureType),
         mAsynUser(set->getUser())
{
    mNodeName = featureName.c_str();
    mIsImplemented = (bool)BFGTLDevNodeExists(mDev, mNodeName);
}

inline asynStatus BFFeature::checkError(int error, const char *functionName, const char *BFFunction)
{
    if (0 != error) {
        asynPrint(mAsynUser, ASYN_TRACE_ERROR,
            "%s:%s: ERROR calling %s error=%d\n",
            driverName, functionName, BFFunction, error);
        return asynError;
    }
    return asynSuccess;
}

bool BFFeature::isImplemented() { 
    return mIsImplemented; 
}

bool BFFeature::isAvailable() {
    BFGTLUtilU32 value;
    if (!mIsImplemented) return false;
    checkError(BFGTLNodeInquire(mNode, BFGTL_NODE_ACCESS, BFGTL_NODE_INQ_ACCESS, &value), "isAvailable", "BFGTLNodeInquire");
    return (value == BFGTL_ACCESS_NA) ? false : true;
 }

bool BFFeature::isReadable() { 
    BFGTLUtilU32 value;
    if (!mIsImplemented) return false;
    checkError(BFGTLNodeInquire(mNode, BFGTL_NODE_ACCESS, BFGTL_NODE_INQ_ACCESS, &value), "isReadable", "BFGTLNodeInquire");
    return ((value == BFGTL_ACCESS_RO) || (value == BFGTL_ACCESS_RW)) ? true : false;
}

bool BFFeature::isWritable() { 
    BFGTLUtilU32 value;
    if (!mIsImplemented) return false;
    checkError(BFGTLNodeInquire(mNode, BFGTL_NODE_ACCESS, BFGTL_NODE_INQ_ACCESS, &value), "isWritable", "BFGTLNodeInquire");
    return ((value == BFGTL_ACCESS_WO) || (value == BFGTL_ACCESS_RW)) ? true : false;
}

epicsInt64 BFFeature::readInteger() {
    epicsInt64 value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readInteger", "BFGTLNodeRead");
    return value;
}

epicsInt64 BFFeature::readIntegerMin() {
    epicsInt64 value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MIN, &value, &size), "readIntegerMin", "BFGTLNodeRead");
    return value;
}

epicsInt64 BFFeature::readIntegerMax() {
    epicsInt64 value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MAX, &value, &size), "readIntegerMax", "BFGTLNodeRead");
    return value;
}

epicsInt64 BFFeature::readIncrement() { 
    epicsInt64 value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_INC, &value, &size), "readIncrement", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeInteger(epicsInt64 value) { 
    size_t size = sizeof(value);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeInteger", "BFGTLNodeWrite");
}

bool BFFeature::readBoolean() { 
    bool value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readBoolean", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeBoolean(bool value) { 
    size_t size = sizeof(value);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeBoolean", "BFGTLNodeWrite");
}

double BFFeature::readDouble() { 
    double value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readDouble", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeDouble(double value) { 
    size_t size = sizeof(value);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeDouble", "BFGTLNodeWrite");
}

double BFFeature::readDoubleMin() {
    double value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MIN, &value, &size), "readDoubleMin", "BFGTLNodeRead");
    return value;
}

double BFFeature::readDoubleMax() {
    double value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MAX, &value, &size), "readDoubleMax", "BFGTLNodeRead");
    return value;
}

int BFFeature::readEnumIndex() { 
    epicsInt64 value;
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readEnumIndex", "BFGTLNodeRead");
    return (int) value;
}

void BFFeature::writeEnumIndex(int value) {
    epicsInt64 iVal = value; 
    size_t size = sizeof(iVal);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &iVal, size), "writeEnumIndex", "BFGTLNodeWrite");
}

std::string BFFeature::readEnumString() { 
    char value[256];
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE_STR, value, &size), "readEnumString", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeEnumString(std::string const &value) { 
}

std::string BFFeature::readString() { 
    char value[256];
    size_t size = sizeof(value);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, value, &size), "readString", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeString(std::string const & value) { 
    size_t size = value.size() + 1;
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, value.c_str(), size), "writeString", "BFGTLNodeWrite");
}

void BFFeature::writeCommand() {
    epicsInt64 value = 1; 
    size_t size = sizeof(value);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeEnumCommand", "BFGTLNodeWrite");
}

void BFFeature::readEnumChoices(std::vector<std::string>& enumStrings, std::vector<int>& enumValues) {
    size_t numEnums;
    size_t size = sizeof(numEnums);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ENTRY_COUNT, &numEnums, &size), "readEnumChoices", "BFGTLNodeRead");
    size = 0;
    // The first call with BFGTL_NODE_ENTRY_NAMES is with pValue=0 so it just returns the required size in size;
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ENTRY_SYMBOLICS, 0, &size), "readEnumChoices", "BFGTLNodeRead");
    // Allocate the required buffer and call again with pValue = pBuff
    char *pBuff = reinterpret_cast<char*>(malloc(size));
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ENTRY_SYMBOLICS, pBuff, &size), "readEnumChoices", "BFGTLNodeRead");
    size_t *pOffset = reinterpret_cast<size_t*>(pBuff);
    size = numEnums * sizeof(epicsInt64);
    epicsInt64 *pValues = reinterpret_cast<epicsInt64*>(malloc(size));
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ENTRY_VALUES, pValues, &size), "readEnumChoices", "BFGTLNodeRead");    
    for (size_t i=0; i<numEnums; i++) {
        if (*pOffset == 0) {
            printf("Error, offset should not be 0\n");
        }
        char *pStr = pBuff + *pOffset;
//        if (IsAvailable(pEntry) && IsReadable(pEntry)) {
        if (true) {
            enumStrings.push_back(pStr);
            enumValues.push_back((int)pValues[i]);
        }
    }
}
