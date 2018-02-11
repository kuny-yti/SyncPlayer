TEMPLATE = lib
CONFIG += plugin

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
MOC_DIR = build/moc

INCLUDEPATH += . ../../playcore/headers
DEPENDPATH += . ../../playcore/headers

HEADERS += SRT.hpp Classic.hpp Subtitles.hpp
SOURCES += SRT.cpp Classic.cpp Subtitles.cpp
