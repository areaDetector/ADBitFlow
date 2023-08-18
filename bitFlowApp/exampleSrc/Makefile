TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

STATIC_BUILD = NO

PROD_IOC_Linux += GenTLInterfaceExample
PROD_IOC_WIN32 += GenTLInterfaceExample
GenTLInterfaceExample_SRCS += GenTLInterfaceExample.cpp GenTLView.cpp

#PROD_LIBS_WIN32 += Spinnaker_v140

PROD_LIBS_Linux += GenTLInterface
PROD_LIBS_Linux += GCBase_gcc48_v3_3
PROD_LIBS_Linux += GenApi_gcc48_v3_3

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

