INCLUDEPATH += $$PWD
VPATH += $$PWD

HEADERS += \
    $$PWD/media_object.h \
    $$PWD/read_thread.h \
    $$PWD/video_decode_thread.h \
    $$PWD/audio_decode_thread.h \
    $$PWD/ffmpeg.h \
    $$PWD/subtitle_decode_thread.h

SOURCES += \
    $$PWD/media_object.cpp \
    $$PWD/read_thread.cpp \
    $$PWD/video_decode_thread.cpp \
    $$PWD/audio_decode_thread.cpp \
    $$PWD/subtitle_decode_thread.cpp

