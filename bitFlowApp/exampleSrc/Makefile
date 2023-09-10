TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

STATIC_BUILD = NO

USR_INCLUDES += -I$(BITFLOW_SDK_INCLUDE)
USR_LDFLAGS_WIN32 += /LIBPATH:$(BITFLOW_SDK_LIB)

#PROD_IOC_Linux += GenTLInterfaceExample
#PROD_IOC_WIN32 += GenTLInterfaceExample
#GenTLInterfaceExample_SRCS += GenTLInterfaceExample.cpp GenTLView.cpp
#GenTLInterfaceExample_SRCS += GenTLInterfaceExample.cpp
PROD_IOC_Linux      += CIsimple
CIsimple_SRCS_Linux += CIsimple_Linux.c
PROD_IOC_WIN32      += CiSimple
CiSimple_SRCS_WIN32 += CiSimple_Windows.c

PROD_IOC_WIN32      += test_features
test_features_SRCS  += test_features.c

#PROD_LIBS_Linux += GenTLInterface
#PROD_LIBS_Linux += GCBase_gcc48_v3_3
#PROD_LIBS_Linux += GenApi_gcc48_v3_3
PROD_LIBS_Linux += BFSOciLib.9.08

PROD_SYS_LIBS_WIN32 += BFGTLUtil
PROD_SYS_LIBS_WIN32 += BFEr
PROD_SYS_LIBS_WIN32 += BFD
PROD_SYS_LIBS_WIN32 += DispSurf
PROD_SYS_LIBS_WIN32 += Cid
PROD_SYS_LIBS_WIN32 += User32

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

