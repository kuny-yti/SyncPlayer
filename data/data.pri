INCLUDEPATH += $$PWD
VPATH += $$PWD

HEADERS += \
    $$PWD/video_frame.h \
    $$PWD/type.h \
    $$PWD/audio_blob.h \
    $$PWD/subtitle_box.h \
    $$PWD/device_request.h \
    $$PWD/blob.h \
    $$PWD/open_input_data.h \
    data/thread_base.h
SOURCES += \
    $$PWD/video_frame.cpp \
    $$PWD/audio_blob.cpp \
    $$PWD/subtitle_box.cpp \
    $$PWD/device_request.cpp \
    $$PWD/open_input_data.cpp \
    data/thread_base.cpp
