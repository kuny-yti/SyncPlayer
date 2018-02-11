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

RCC_DIR = build/rcc
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

RESOURCES += icon.qrc

INCLUDEPATH += . ../../playcore/headers
DEPENDPATH += . ../../playcore/headers

HEADERS += QPainterWriter.hpp QPainter.hpp
SOURCES += QPainterWriter.cpp QPainter.cpp
