TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++11

QMAKE_CXXFLAGS += -O2 -Wall -Wextra -Wno-unused-parameter -Wno-implicit-fallthrough

INCLUDEPATH += $${LOCAL_INCLUDEPATH}
DEPENDPATH += $${LOCAL_INCLUDEPATH}

# BitFlow include
!include($$PWD/../../ext/BFT4Lin_Ext_BitFlow_inc.pri):error("BitFlow inc PRI is missing!")

SOURCES += \
    BFLogIOMessageApi.cpp \
    BFLogIOMessageClass.cpp \
    BFLogIOSourceApi.cpp \
    BFLogIOSourceClass.cpp \
    BFLogIOSyncApi.cpp \
    BFLogIOSyncClass.cpp \
    BFLogClientApi.cpp

HEADERS += \
    BFLogIODef.h \
    BFLogIOMessageApi.h \
    BFLogIOMessageClass.h \
    BFLogIOSourceApi.h \
    BFLogIOSourceClass.h \
    BFLogIOSyncApi.h \
    BFLogIOSyncClass.h \
    TimeoutHelper.h \
    BFUniversalDef.h \
    BFLogClientApi.h

