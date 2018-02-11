#include "audio_blob.h"

audio_blob::audio_blob() :
    language(),
    channels(-1),
    rate(-1),
    sample_format(FORMAT_U8),
    data(NULL),
    size(0),
    p_time(0UL)
{
}

QString audio_blob::format_info() const
{
    return QString("%1, %2 ch., %3 kHz, %4 bit")
            .arg(language.isEmpty()? "unknown": language.toUtf8().constData())
            .arg(channels)
            .arg(rate / 1e3f)
            .arg(sample_bits());
}

QString audio_blob::format_name() const
{
    const char *sample_format_name = "";
    switch (sample_format)
    {
    case FORMAT_U8:
        sample_format_name = "u8";
        break;
    case FORMAT_S16:
        sample_format_name = "s16";
        break;
    case FORMAT_F32:
        sample_format_name = "f32";
        break;
    case FORMAT_D64:
        sample_format_name = "d64";
        break;
    }
    return QString("%1-%2-%3-%4")
            .arg(language.isEmpty() ? "unknown" : language.toUtf8().constData())
            .arg(channels)
            .arg(rate)
            .arg(sample_format_name);
}

int audio_blob::sample_bits() const
{
    int bits = 0;
    switch (sample_format)
    {
    case FORMAT_U8:
        bits = 8;
        break;
    case FORMAT_S16:
        bits = 16;
        break;
    case FORMAT_F32:
        bits = 32;
        break;
    case FORMAT_D64:
        bits = 64;
        break;
    }
    return bits;
}
