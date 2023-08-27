TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# From Linux SDK
INC += BFciLib.h
INC += BFGTLUtilApi.h
INC += BFGTLUtilDef.h
INC += BFGTLExtensionsApi.h

# From Windows SDK
INC += BFType.h
INC += BuildTarget.h
INC += BFWhatSystem.h
INC += BFTypeLX.h
INC += BFTypeNT.h
INC += BFTypePT.h

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))

else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS    += ../os/linux-x86_64/libBFSOciLib.9.08.so

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

