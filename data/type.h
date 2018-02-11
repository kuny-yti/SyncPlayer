#ifndef TYPE_H
#define TYPE_H
#include <QtGlobal>
#include <QString>
#include <QVector>
#include <QOpenGLFunctions>

#ifdef HAVE_CLOCK_GETTIME
# include <time.h>
#else
# include <sys/time.h>
#endif

#ifdef NDEBUG
# define HERE QString()
# define DEBUG(a)
#else
# define HERE QString("%1, function %2, line %3").arg( __FILE__).arg(__func__).arg( __LINE__)
# define DEBUG(a) //qDebug() << a
#endif


#ifndef min
template<typename T>
inline const T min(const T& t1, const T& t2)
{
   return t1<t2?t1:t2;
}
#endif

#ifndef max
template <class T>
inline const T& max(const T& t1,const T& t2)
{
    return t1<t2?t2:t1;
}
#endif


// 数据格式
typedef enum
{
    LAYOUT_BGRA32,         // 单面: BGRABGRABGRA....
    LAYOUT_YUV444P,        // 三面, Y/U/V, 所有具有相同格式
    LAYOUT_YUV422P,        // 三面, U and V 半宽度: one U/V pair for 2x1 Y values
    LAYOUT_YUV420P         // 三面, U and V with half width and half height: one U/V pair for 2x2 Y values
} DataLayout;

// 颜色空间
typedef enum
{
    SPACE_SRGB,           // SRGB color space
    SPACE_YUV601,         // YUV according to ITU.BT-601
    SPACE_YUV709          // YUV according to ITU.BT-709
} ColorSpace;

// 值范围
typedef enum
{
    RANGE_U8_FULL,        // 0-255 for all components
    RANGE_U8_MPEG,        // 16-235 for Y, 16-240 for U and V
    RANGE_U10_FULL,       // 0-1023 for all components (stored in 16 bits)
    RANGE_U10_MPEG        // 64-940 for Y, 64-960 for U and V (stored in 16 bits)
} ValueRange;

// 色度取样位置（仅用于色度抽样布局相关的）
typedef enum
{
    LOCATION_CENTER,         // U/V at center of the corresponding Y locations
    LOCATION_LEFT,           // U/V vertically at the center, horizontally at the left Y locations
    LOCATION_TOP_LEFT        // U/V at the corresponding top left Y location
} ChromaLocation;

// 采样格式
typedef enum
{
    FORMAT_U8,             // uint8_t
    FORMAT_S16,            // int16_t
    FORMAT_F32,            // float
    FORMAT_D64             // double
} SampleFormat;

// 字幕格式
typedef enum
{
    FORMAT_ASS,            // Advanced SubStation Alpha (ASS) format
    FORMAT_TEXT,           // UTF-8 text
    FORMAT_IMAGE           // Image in BGRA32 format, with box coordinates
} SubtitleFormat;

// 立体布局: 介绍了左、右视图的存储
typedef enum {
    STEREO_MONO,           // 1 video source: center view
    STEREO_SEPARATE,       // 2 video sources: left and right view independent
    STEREO_ALTERNATING,    // 2 video sources: left and right view consecutively
    STEREO_TOP_BOTTOM,     // 1 video source: left view top, right view bottom, both with full size
    STEREO_TOP_BOTTOM_HALF,// 1 video source: left view top, right view bottom, both with half size
    STEREO_LEFT_RIGHT,     // 1 video source: left view left, right view right, both with full size
    STEREO_LEFT_RIGHT_HALF,// 1 video source: left view left, right view right, both with half size
    STEREO_EVEN_ODD_ROWS   // 1 video source: left view even lines, right view odd lines
} StereoLayout;

// 立体模式: 左和右视图的输出模式
typedef enum {
    MODE_STEREO,                   // OpenGL quad buffered stereo
    MODE_ALTERNATING,              // Left and right view alternating
    MODE_MONO_LEFT,                // Left view only
    MODE_MONO_RIGHT,               // Right view only
    MODE_TOP_BOTTOM,               // Left view top, right view bottom
    MODE_TOP_BOTTOM_HALF,          // Left view top, right view bottom, half height
    MODE_LEFT_RIGHT,               // Left view left, right view right
    MODE_LEFT_RIGHT_HALF,          // Left view left, right view right, half width
    MODE_EVEN_ODD_ROWS,            // Left view even rows, right view odd rows
    MODE_EVEN_ODD_COLUMNS,         // Left view even columns, right view odd columns
    MODE_CHECKERBOARD,             // Checkerboard pattern
    MODE_HDMI_FRAME_PACK,          // HDMI Frame packing (top-bottom separated by 1/49 height)
    MODE_RED_CYAN_MONOCHROME,      // Red/cyan anaglyph, monochrome method
    MODE_RED_CYAN_HALF_COLOR,      // Red/cyan anaglyph, half color method
    MODE_RED_CYAN_FULL_COLOR,      // Red/cyan anaglyph, full color method
    MODE_RED_CYAN_DUBOIS,          // Red/cyan anaglyph, high quality Dubois method
    MODE_GREEN_MAGENTA_MONOCHROME, // Green/magenta anaglyph, monochrome method
    MODE_GREEN_MAGENTA_HALF_COLOR, // Green/magenta anaglyph, half color method
    MODE_GREEN_MAGENTA_FULL_COLOR, // Green/magenta anaglyph, full color method
    MODE_GREEN_MAGENTA_DUBOIS,     // Green/magenta anaglyph, high quality Dubois method
    MODE_AMBER_BLUE_MONOCHROME,    // Amber/blue anaglyph, monochrome method
    MODE_AMBER_BLUE_HALF_COLOR,    // Amber/blue anaglyph, half color method
    MODE_AMBER_BLUE_FULL_COLOR,    // Amber/blue anaglyph, full color method
    MODE_AMBER_BLUE_DUBOIS,        // Amber/blue anaglyph, high quality Dubois method
    MODE_RED_GREEN_MONOCHROME,     // Red/green anaglyph, monochrome method
    MODE_RED_BLUE_MONOCHROME       // Red/blue anaglyph, monochrome method
} StereoMode;


typedef enum
{
    DEVICE_NO,             // No device request.
    DEVICE_SYS_DEFAULT,    // Request for system default video device type.
    DEVICE_FIREWIRE,       // Request for a firefire video device.
    DEVICE_X11             // Request for an X11 grabber.
} Device;

typedef enum
{
    TIMER_REALTIME,
    TIMER_MONOTONIC,
    TIMER_PROCESS_CPU,
    TIMER_THREAD_CPU
}TimerType;

//! 定义各需要控制对象的起始位置
typedef enum
{
    CTRL_AUDIO_BEGIN = 1,
    CTRL_AUDIO_END = 1 << 8,
    CTRL_VIDEO_BEGIN = CTRL_AUDIO_END,
    CTRL_VIDEO_END = 1 << 24,
    CTRL_LOOP_BEGIN = CTRL_VIDEO_END,
    CTRL_LOOP_END = 1 << 30
}eCtrlCommand;

//! 定义音频控制命令
typedef enum
{
    AUDIO_INIT = CTRL_AUDIO_BEGIN,//打开音频引擎
    AUDIO_UNINIT,//关闭音频引擎
    AUDIO_START,//启动音频输出
    AUDIO_STOP,//停止音频输出
    AUDIO_PAUSE,//暂停音频输出
    AUDIO_UNPAUSE,//恢复音频输出
    AUDIO_MUTE,//音频输出静音
    AUDIO_VOLUME//设置音频输出音量
}eAudioCtrl;

typedef enum
{
    // Play state
    PLAYER_OPEN = CTRL_VIDEO_BEGIN,        // 打开播放器
    PLAYER_CLOSE,                          // 关闭播放器
    PLAYER_PAUSE,                          // 暂停播放
    PLAYER_UNPAUSE,                        // 恢复播放
    PLAYER_VOLUME,
    PLAYER_PLAY,
    PLAYER_STOP,
    PLAYER_TOOGLE_PLAY,                    // no parameters
    PLAYER_TOOGLE_MUTE,                    // no parameters
    PLAYER_TOOGLE_PAUSE,                   // no parameters
    PLAYER_STEP,                           // no parameters
    PLAYER_SEEK,                           // float (relative adjustment)
    PLAYER_SET_POS,                         // float (absolute position)
    PLAYER_NETSYNC
} ePlayerCtrl;

typedef enum
{
    LOOP_NO = CTRL_LOOP_BEGIN,      // 播放完当前的文件，不循环
    LOOP_CURRENT_FILE,              // 循环当前文件
    LOOP_CURRENT_LIST,              // 循环当前列表
    LOOP_SWITCH_FILE
} eLoopMode;

inline qint64 get_microseconds(TimerType type = TIMER_MONOTONIC)
{
#ifdef HAVE_CLOCK_GETTIME

    struct timespec time;
    int r;
    if (t == TIMER_REALTIME)
    {
        r = clock_gettime(CLOCK_REALTIME, &time);
    }
    else if (t == TIMER_MONOTONIC)
    {
        r = clock_gettime(CLOCK_MONOTONIC, &time);
    }
    else if (t == TIMER_PROCESS_CPU)
    {
#if defined(CLOCK_PROCESS_CPUTIME_ID)
        r = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
#elif defined(CLOCK_PROF)
        r = clock_gettime(CLOCK_PROF, &time);
#else
        r = -1;
        errno = ENOSYS;
#endif
    } else /* t == thread_cpu */ {
#ifdef CLOCK_THREAD_CPUTIME_ID
        r = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
#else
        r = -1;
        errno = ENOSYS;
#endif
    }
    if (r != 0)
        DEBUG( QString("Cannot get time.").arg(errno));
    return static_cast<qint64>(time.tv_sec) * 1000000 + time.tv_nsec / 1000;

#else

    if (type == TIMER_REALTIME || type == TIMER_MONOTONIC)
    {
        struct timeval tv;
        int r = gettimeofday(&tv, NULL);
        if (r != 0)
            DEBUG( QString("Cannot get time. (%1)").arg(errno));

        return static_cast<qint64>(tv.tv_sec) * 1000000 + tv.tv_usec;
    }
    else if (type == TIMER_PROCESS_CPU)
    {
        // In W32, clock() starts with zero on program start, so we do not need
        // to subtract a start value.
        // XXX: Is this also true on Mac OS X?
        clock_t c = clock();
        return (c * static_cast<qint64>(1000000) / CLOCKS_PER_SEC);
    }
    else {
        DEBUG( QString("Cannot get time.").arg( ENOSYS));
    }

#endif
}

#endif // TYPE_H
