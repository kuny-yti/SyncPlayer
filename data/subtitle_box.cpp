#include "subtitle_box.h"

subtitle_box::subtitle_box() :
    language(),
    format(FORMAT_TEXT),
    style(),
    str(),
    images(),
    p_start_time(0UL),
    p_stop_time(0UL)
{
}

QString subtitle_box::format_info() const
{
    return QString(language.isEmpty() ? "unknown" : language);
}

QString subtitle_box::format_name() const
{
    return format_info();
}

void subtitle_box::image_t::save(QDataStream &os) const
{
    os << size << pos << palette.size();

    if (palette.size() > 0)
    {
        os.writeRawData((char *)(&(palette[0])), palette.size() * sizeof(quint8));
    }
    os << data.size();
    if (data.size() > 0)
    {
        os.writeRawData( (char *)(&(data[0])), data.size() * sizeof(quint8));
    }
    os << linesize;
}

void subtitle_box::image_t::load(QDataStream &is)
{
    size_t s;
    is >> size >> pos >> s;

    palette.resize(s);
    if (palette.size() > 0)
    {
        is.readRawData((char *)(&(palette[0])), palette.size() * sizeof(quint8));
    }
    is >> s;

    data.resize(s);
    if (data.size() > 0)
    {
        is.readRawData((char *)(&(data[0])), data.size() * sizeof(quint8));
    }
    is >> linesize;
}

void subtitle_box::save(QDataStream &os) const
{
    os << language << static_cast<int>(format);
    os << style << str ;
    os << images.size();
    for (int i = 0; i < images.size(); i++)
    {
        images[i].save(os);
    }
    os << p_start_time;
    os << p_stop_time;
}

void subtitle_box::load(QDataStream &is)
{
    is >> language;
    int x;
    is >> x;
    format = static_cast<SubtitleFormat>(x);
    is >> style >> str;
    is >> x ;
    for (int i = 0; i < x; i++)
    {
        image_t t;
        t.load(is);
        images << t;
    }
    is>> p_start_time;
    is >> p_stop_time;
}
