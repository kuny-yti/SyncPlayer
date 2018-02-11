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
        QMAKE_LIBDIR += ../../../app
}
LIBS += $${PWD}/../../../app/libplaycore.a
include (../../../lib/lib.pri)

OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
MOC_DIR = build/moc

RESOURCES += icons.qrc

INCLUDEPATH += . ../../playcore/headers
DEPENDPATH += . ../../playcore/headers

HEADERS += Inputs.hpp ToneGenerator.hpp PCM.hpp Rayman2.hpp
SOURCES += Inputs.cpp ToneGenerator.cpp PCM.cpp Rayman2.cpp
