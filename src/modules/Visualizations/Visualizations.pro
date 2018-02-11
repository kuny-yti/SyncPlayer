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

win32: LIBS += -lavcodec -lavutil
else {
	macx: QT_CONFIG -= no-pkg-config
	CONFIG += link_pkgconfig
	PKGCONFIG += libavcodec libavutil
}
LIBS += -lqmplay2
}

OBJECTS_DIR = build/obj
MOC_DIR = build/moc

INCLUDEPATH += . ../../playcore/headers
DEPENDPATH += . ../../playcore/headers

HEADERS += Visualizations.hpp SimpleVis.hpp FFTSpectrum.hpp VisWidget.hpp
SOURCES += Visualizations.cpp SimpleVis.cpp FFTSpectrum.cpp VisWidget.cpp

DEFINES += __STDC_CONSTANT_MACROS
