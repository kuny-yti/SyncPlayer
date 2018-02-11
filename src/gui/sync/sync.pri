INCLUDEPATH += $$PWD
VPATH += $$PWD

DEFINES += EXTSYNC

HEADERS += \
    $$PWD/command.h \
    $$PWD/sync_data.h \
    $$PWD/sync_udp.h \
    $$PWD/sync_tcp.h \
    $$PWD/sync_core.h

SOURCES += \
    $$PWD/command.cpp \
    $$PWD/sync_udp.cpp \
    $$PWD/sync_tcp.cpp \
    $$PWD/sync_core.cpp

