TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

INC += $(patsubst ../%, %, $(wildcard ../*.h))
INC += $(patsubst ../%, %, $(wildcard ../*.hpp))

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS += ../os/windows-x64/Cid.lib
LIB_INSTALLS += ../os/windows-x64/BFGTLUtil.lib
LIB_INSTALLS += ../os/windows-x64/BFEr.lib
LIB_INSTALLS += ../os/windows-x64/DispSurf.lib
LIB_INSTALLS += ../os/windows-x64/BFD.lib
LIB_INSTALLS += ../os/windows-x64/Bid.lib
LIB_INSTALLS += ../os/windows-x64/BufferInterface.lib

else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS    += ../os/linux-x86_64/libBFSOciLib.9.08.so

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

