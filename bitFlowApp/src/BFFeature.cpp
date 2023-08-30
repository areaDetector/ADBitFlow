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
    static const char *functionName = "BFFeature";
    int err;

    mNodeName = featureName;
    const char *nodeNameStr = mNodeName.c_str();
    ADBitFlow *pDrv = (ADBitFlow *) mSet->getPortDriver();
    mDev = pDrv->getBFGTLDev();
    mIsImplemented = (bool)BFGTLDevNodeExists(mDev, nodeNameStr);
    if (!mIsImplemented) return;
    err = BFGTLNodeOpen(mDev, nodeNameStr, &mNode);
    if (err) {
        printf("%s::%s error creating node %s, error=%d\n", driverName, functionName, nodeNameStr, err);
    }
    BFGTLUtilU32 value;
    size_t size = sizeof(value);
    err = BFGTLNodeRead(mNode, BFGTL_NODE_TYPE, &value, &size);
    if (err) {
        printf("%s::%s error reading node type %s, error=%d\n", driverName, functionName, nodeNameStr, err);
    }
    mNodeType = (BFGTLNodeType)value; 
 }

inline asynStatus BFFeature::checkError(int error, const char *functionName, const char *BFFunction)
{
    if (0 != error) {
        asynPrint(mAsynUser, ASYN_TRACE_ERROR,
            "%s:%s: nodeName=%s, ERROR calling %s error=%d (0x%x)\n",
            driverName, functionName, mNodeName.c_str(), BFFunction, error, error);
        return asynError;
    }
    return asynSuccess;
}

bool BFFeature::isImplemented() { 
    return mIsImplemented; 
}

bool BFFeature::isAvailable() {
    BFGTLUtilU32 value;
    size_t size = sizeof(value);
    if (!mIsImplemented) return false;
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ACCESS, &value, &size), "isAvailable", "BFGTLNodeInquire");
    return (value == BFGTL_ACCESS_NA) ? false : true;
 }

bool BFFeature::isReadable() { 
    BFGTLUtilU32 value;
    size_t size = sizeof(value);
    if (!mIsImplemented) return false;
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ACCESS, &value, &size), "isReadable", "BFGTLNodeInquire");
    return ((value == BFGTL_ACCESS_RO) || (value == BFGTL_ACCESS_RW)) ? true : false;
}

bool BFFeature::isWritable() { 
    BFGTLUtilU32 value;
    size_t size = sizeof(value);
    if (!mIsImplemented) return false;
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_ACCESS, &value, &size), "isWritable", "BFGTLNodeInquire");
    //printf("BFFeature::isWritable nodeName=%s, access=%d\n", mNodeName.c_str(), value);
    return ((value == BFGTL_ACCESS_WO) || (value == BFGTL_ACCESS_RW)) ? true : false;
}

epicsInt64 BFFeature::readInteger() {
    epicsInt64 value;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_INTEGER) printf("BFFeature::readInteger warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readInteger", "BFGTLNodeRead");
    return value;
}

epicsInt64 BFFeature::readIntegerMin() {
    epicsInt64 value;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_INTEGER) printf("BFFeature::readIntegerMin warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MIN, &value, &size), "readIntegerMin", "BFGTLNodeRead");
    return value;
}

epicsInt64 BFFeature::readIntegerMax() {
    epicsInt64 value;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_INTEGER) printf("BFFeature::readIntegerMax warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MAX, &value, &size), "readIntegerMax", "BFGTLNodeRead");
    return value;
}

epicsInt64 BFFeature::readIncrement() { 
    epicsInt64 value;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_INTEGER) printf("BFFeature::readIncrement warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_INC, &value, &size), "readIncrement", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeInteger(epicsInt64 value) { 
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_INTEGER) printf("BFFeature::writeInteger warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeInteger", "BFGTLNodeWrite");
}

bool BFFeature::readBoolean() { 
    epicsInt64 value;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_BOOLEAN) printf("BFFeature::readBoolean warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readBoolean", "BFGTLNodeRead");
    return (bool)value;
}

void BFFeature::writeBoolean(bool bval) {
    epicsInt64 value = bval;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_BOOLEAN) printf("BFFeature::writeBoolean warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeBoolean", "BFGTLNodeWrite");
}

// The Mikrotron cameras use integer node types for ExposureTime and AcquisitionFrameRate but ADGenICam expects these to be float nodes.
// These double methods need to handle integers
double BFFeature::readDouble() {
    if (mNodeType == BFGTL_NODE_TYPE_FLOAT) {
        double value;
        size_t size = sizeof(value);
        checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readDouble", "BFGTLNodeRead");
        return value;
    } else if (mNodeType == BFGTL_NODE_TYPE_INTEGER) {
        epicsInt64 value;
        size_t size = sizeof(value);
        checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readDouble", "BFGTLNodeRead");
        return (double)value;
    } else { 
        printf("BFFeature::readDouble node %s, error node type=%d\n", mNodeName.c_str(), mNodeType);
    }
    return 0;
}

void BFFeature::writeDouble(double value) { 
    if (mNodeType == BFGTL_NODE_TYPE_FLOAT) {
        size_t size = sizeof(value);
        checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &value, size), "writeDouble", "BFGTLNodeWrite");
    } else if (mNodeType == BFGTL_NODE_TYPE_INTEGER) {
        epicsInt64 ival = (epicsInt64)value;
        size_t size = sizeof(ival);
       checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &ival, size), "writeDouble", "BFGTLNodeWrite");
    } else { 
        printf("BFFeature::writeDouble node %s, error node type=%d\n", mNodeName.c_str(), mNodeType);
    }
}

double BFFeature::readDoubleMin() {
    if (mNodeType == BFGTL_NODE_TYPE_FLOAT) {
        double value;
        size_t size = sizeof(value);
        checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MIN, &value, &size), "readDoubleMin", "BFGTLNodeRead");
        return value;
    } else if (mNodeType == BFGTL_NODE_TYPE_INTEGER) {
        epicsInt64 value;
        size_t size = sizeof(value);
        checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MIN, &value, &size), "readDoubleMin", "BFGTLNodeRead");
        return (double)value;
    } else { 
        printf("BFFeature::readDoubleMin node %s, error node type=%d\n", mNodeName.c_str(), mNodeType);
    }
    return 0;
}

double BFFeature::readDoubleMax() {
    if (mNodeType == BFGTL_NODE_TYPE_FLOAT) {
        double value;
        size_t size = sizeof(value);
        checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MAX, &value, &size), "readDoubleMax", "BFGTLNodeRead");
        return value;
    } else if (mNodeType == BFGTL_NODE_TYPE_INTEGER) {
        epicsInt64 value;
        size_t size = sizeof(value);
        checkError(BFGTLNodeRead(mNode, BFGTL_NODE_MAX, &value, &size), "readDoubleMax", "BFGTLNodeRead");
        return (double)value;
    } else { 
        printf("BFFeature::readDoubleMin node %s, error node type=%d\n", mNodeName.c_str(), mNodeType);
    }
    return 0;
}

int BFFeature::readEnumIndex() { 
    epicsInt64 value;
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_ENUMERATION) printf("BFFeature::readEnumIndex warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, &value, &size), "readEnumIndex", "BFGTLNodeRead");
    return (int) value;
}

void BFFeature::writeEnumIndex(int value) {
    epicsInt64 iVal = value; 
    size_t size = sizeof(iVal);
    if (mNodeType != BFGTL_NODE_TYPE_ENUMERATION) printf("BFFeature::writeEnumIndex warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, &iVal, size), "writeEnumIndex", "BFGTLNodeWrite");
}

std::string BFFeature::readEnumString() { 
    char value[256];
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_ENUMERATION) printf("BFFeature::readEnumString warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE_STR, value, &size), "readEnumString", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeEnumString(std::string const &value) { 
}

std::string BFFeature::readString() { 
    char value[256];
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_STRING) printf("BFFeature::readString warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeRead(mNode, BFGTL_NODE_VALUE, value, &size), "readString", "BFGTLNodeRead");
    return value;
}

void BFFeature::writeString(std::string const & value) { 
    size_t size = value.size() + 1;
    if (mNodeType != BFGTL_NODE_TYPE_STRING) printf("BFFeature::writeString warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, value.c_str(), size), "writeString", "BFGTLNodeWrite");
}

void BFFeature::writeCommand() {
    epicsInt64 value = 1; 
    size_t size = sizeof(value);
    if (mNodeType != BFGTL_NODE_TYPE_COMMAND) printf("BFFeature::writeCommand warning node type=%d\n", mNodeType);
    checkError(BFGTLNodeWrite(mNode, BFGTL_NODE_VALUE, 0, 0), "writeCommand", "BFGTLNodeWrite");
}

void BFFeature::readEnumChoices(std::vector<std::string>& enumStrings, std::vector<int>& enumValues) {
    size_t numEnums;
    size_t size = sizeof(numEnums);
    if (mNodeType != BFGTL_NODE_TYPE_ENUMERATION) printf("BFFeature::readEnumChoices warning node type=%d\n", mNodeType);
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
    for (size_t i=0; i<numEnums; i++, pOffset++) {
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
