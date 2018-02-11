TEMPLATE = lib
CONFIG += plugin

QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
	lessThan(QT_VERSION, 5.7.0): CONFIG -= c++11
	QT += widgets
}

win32|macx {
        DESTDIR = $${PWD}/../../../app/modules
	QMAKE_LIBDIR += ../../../app
}
else {
        DESTDIR = $${PWD}/../../../app/share/qmplay2/modules
	QMAKE_LIBDIR += ../../../app/lib
}
LIBS += $${PWD}/../../../app/libplaycore.a
include (../../../lib/lib.pri)

!ext_lib {
LIBS += -lqmplay2
}

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../playcore/headers
DEPENDPATH  += . ../../playcore/headers

HEADERS += Extensions.hpp YouTube.hpp Downloader.hpp Radio.hpp LastFM.hpp ProstoPleer.hpp
SOURCES += Extensions.cpp YouTube.cpp Downloader.cpp Radio.cpp LastFM.cpp ProstoPleer.cpp

unix:!macx:!android {
	QT += dbus
	HEADERS += MPRIS2.hpp
	SOURCES += MPRIS2.cpp
	DEFINES += USE_MPRIS2
}
