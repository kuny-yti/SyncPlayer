#-------------------------------------------------
#
# Project created by QtCreator 2014-02-26T20:56:28
#
#-------------------------------------------------

QT       += core gui network opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SyncPlayer
TEMPLATE = app

DEFINES += NDEBUG

DESTDIR = $${PWD}/bulid/
UI_DIR = $${PWD}/bulid/temp/.ui
MOC_DIR = $${PWD}/bulid/temp/.moc
OBJECTS_DIR = $${PWD}/bulid/temp/.obj
RCC_DIR = $${PWD}/bulid/temp/.rcc

include(./data/data.pri)
include(./decode/decode.pri)
include(./player/player.pri)
include(./output/output.pri)
include(./viewer/viewer.pri)
include(./lib/lib.pri)
include(./sync/sync.pri)

SOURCES += main.cpp \
    ctrl_default.cpp \
    ctrl_sync.cpp \
    setup.cpp

FORMS += \
    ctrl_default.ui \
    ctrl_sync.ui \
    setup.ui

HEADERS += \
    ctrl_default.h \
    ctrl_sync.h \
    setup.h

LIBS += -lws2_32 -lwsock32
