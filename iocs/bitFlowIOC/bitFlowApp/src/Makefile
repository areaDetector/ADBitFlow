TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USR_LDFLAGS_WIN32 += /LIBPATH:$(BITFLOW_SDK_LIB)

PROD_NAME = bitFlowApp
PROD_IOC_WIN32 += $(PROD_NAME)
PROD_IOC_Linux += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += ADBitFlowSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp

# Add locally compiled object code
PROD_LIBS += ADBitFlow
PROD_LIBS += ADGenICam

PROD_SYS_LIBS_Linux += BFSOciLib.9.08
PROD_LIBS_Linux += BFGTLUtil
BFGTLUtil_DIR = /usr/local/lib/BitFlow

PROD_SYS_LIBS_WIN32 += BFGTLUtil
PROD_SYS_LIBS_WIN32 += BFEr
PROD_SYS_LIBS_WIN32 += BFD
PROD_SYS_LIBS_WIN32 += DispSurf
PROD_SYS_LIBS_WIN32 += Cid
PROD_SYS_LIBS_WIN32 += Bid
PROD_SYS_LIBS_WIN32 += BufferInterface
PROD_SYS_LIBS_WIN32 += User32

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

