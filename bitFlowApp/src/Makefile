TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================

DBD += ADBitFlowSupport.dbd

USR_INCLUDES += -I$(BITFLOW_SDK_INCLUDE)

LIBRARY_IOC_Linux += ADBitFlow
LIBRARY_IOC_WIN32 += ADBitFlow
LIB_SRCS += BFFeature.cpp
LIB_SRCS += ADBitFlow.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

