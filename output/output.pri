INCLUDEPATH += $$PWD
VPATH += $$PWD

include ($$PWD/../lib/openal/openal.pri)

HEADERS += \
    $$PWD/audio_output.h \
    output/color_handel.h \
    output/input_handel.h \
    #output/subtitle_handel.h \
    #output/subtitle_renderer.h \
    #output/subtitle_updater.h \
    output/video_output.h


SOURCES += \
    $$PWD/audio_output.cpp \
    output/color_handel.cpp \
    output/input_handel.cpp \
    #output/subtitle_handel.cpp \
    #output/subtitle_renderer.cpp \
    #output/subtitle_updater.cpp \
    output/video_output.cpp

RESOURCES += \
    output/shader.qrc

OTHER_FILES += \
    output/video_output_color.fs.glsl


LIBS = -lopengl32 -lglu32 -luuid -lwinmm
