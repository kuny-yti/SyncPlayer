
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

QT += core gui network opengl

HEADERS += \
    #viewer/camera.h \
    #viewer/window_base.h \
    #viewer/context_thread.h \
    #viewer/window_view.h \
    viewer/render_kernel.h \
    viewer/type_define.h

SOURCES += \
    #viewer/camera.cpp \
    #viewer/window_base.cpp \
    #viewer/context_thread.cpp \
    #viewer/window_view.cpp \
    viewer/render_kernel.cpp \
    viewer/type_define.cpp

