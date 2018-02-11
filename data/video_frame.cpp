#include "video_frame.h"

video_frame::video_frame() :
    raw_size(-1, -1),
    raw_aspect_ratio(0.0f),
    size(-1, -1),
    aspect_ratio(0.0f),
    layout(LAYOUT_BGRA32),
    color_space(SPACE_SRGB),
    value_range(RANGE_U8_FULL),
    chroma_location(LOCATION_CENTER),
    stereo_layout(STEREO_MONO),
    stereo_layout_swap(false),
    p_time(0UL)
{
    for (int i = 0; i < 2; i++)
    {
        for (int p = 0; p < 3; p++)
        {
            data[i][p] = NULL;
            line_size[i][p] = 0;
        }
    }
}
void video_frame::set_view_dimensions()
{
    size = raw_size;
    aspect_ratio = raw_aspect_ratio;
    if (stereo_layout == STEREO_LEFT_RIGHT)
    {
        size.setWidth(size.width() / 2);
        aspect_ratio /= 2.0f;
    }
    else if (stereo_layout == STEREO_LEFT_RIGHT_HALF)
    {
        size.setWidth(size.width() / 2);
    }
    else if (stereo_layout == STEREO_TOP_BOTTOM)
    {
        size.setHeight(size.height() / 2);
        aspect_ratio *= 2.0f;
    }
    else if (stereo_layout == STEREO_TOP_BOTTOM_HALF)
    {
        size.setHeight(size.height() / 2);
    }
    else if (stereo_layout == STEREO_EVEN_ODD_ROWS)
    {
        size.setHeight(size.height() / 2);
        //aspect_ratio *= 2.0f;
        // The only video files I know of which use row-alternating format (those from stereopia.com)
        // do not want this adjustment of aspect ratio.
    }
}

QString video_frame::format_name() const
{
    QString name = QString("%1x%2-%3:1-").arg(raw_size.width()).arg(raw_size.height())
            .arg(raw_aspect_ratio);
    switch (layout)
    {
    case LAYOUT_BGRA32:
        name.append( "bgra32");
        break;
    case LAYOUT_YUV444P:
        name.append( "yuv444p");
        break;
    case LAYOUT_YUV422P:
        name.append( "yuv422p");
        break;
    case LAYOUT_YUV420P:
        name.append( "yuv420p");
        break;
    }
    switch (color_space)
    {
    case SPACE_SRGB:
        name .append( "-srgb");
        break;
    case SPACE_YUV601:
        name .append( "-601");
        break;
    case SPACE_YUV709:
        name .append( "-709");
        break;
    }
    if (layout != LAYOUT_BGRA32)
    {
        switch (value_range)
        {
        case RANGE_U8_FULL:
            name .append( "-jpeg");
            break;
        case RANGE_U8_MPEG:
            name .append( "-mpeg");
            break;
        case RANGE_U10_FULL:
            name .append( "-jpeg10");
            break;
        case RANGE_U10_MPEG:
            name .append( "-mpeg10");
            break;
        }
    }
    if (layout == LAYOUT_YUV422P || layout == LAYOUT_YUV420P)
    {
        switch (chroma_location)
        {
        case LOCATION_CENTER:
            name .append( "-c");
            break;
        case LOCATION_LEFT:
            name .append( "-l");
            break;
        case LOCATION_TOP_LEFT:
            name .append( "-tl");
            break;
        }
    }
    return name;
}

QString video_frame::format_info() const
{
    /* TRANSLATORS: This is a very short string describing the video size and aspect ratio. */
    return QString("%1x%2, %3:1").arg(raw_size.width())
            .arg(raw_size.height()).arg(aspect_ratio);
}
static int next_multiple_of_4(int x)
{
    return (x / 4 + (x % 4 == 0 ? 0 : 1)) * 4;
}
void video_frame::copy_plane(int view, int plane, void *buf) const
{
    char *dst = (char *)buf;
    const char *src = NULL;
    size_t src_offset = 0;
    size_t src_row_size = 0;
    size_t dst_row_width = 0;
    size_t dst_row_size = 0;
    size_t lines = 0;
    size_t type_size = (value_range == RANGE_U8_FULL
                        || value_range == RANGE_U8_MPEG) ? 1 : 2;

    switch (layout)
    {
    case LAYOUT_BGRA32:
        dst_row_width = raw_size.width() * 4;
        dst_row_size = dst_row_width * type_size;
        lines = raw_size.height();
        break;

    case LAYOUT_YUV444P:
        dst_row_width = raw_size.width();
        dst_row_size = next_multiple_of_4(dst_row_width * type_size);
        lines = raw_size.height();
        break;

    case LAYOUT_YUV422P:
        if (plane == 0)
        {
            dst_row_width = raw_size.width();
            dst_row_size = next_multiple_of_4(dst_row_width * type_size);
            lines = raw_size.height();
        }
        else
        {
            dst_row_width = raw_size.width() / 2;
            dst_row_size = next_multiple_of_4(dst_row_width * type_size);
            lines = raw_size.height();
        }
        break;

    case LAYOUT_YUV420P:
        if (plane == 0)
        {
            dst_row_width = raw_size.width();
            dst_row_size = next_multiple_of_4(dst_row_width * type_size);
            lines = raw_size.height();
        }
        else
        {
            dst_row_width = raw_size.width() / 2;
            dst_row_size = next_multiple_of_4(dst_row_width * type_size);
            lines = raw_size.height() / 2;
        }
        break;
    }

    if (stereo_layout_swap)
    {
        view = (view == 0 ? 1 : 0);
    }
    switch (stereo_layout)
    {
    case STEREO_MONO:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = line_size[0][plane];
        src_offset = 0;
        break;
    case STEREO_SEPARATE:
    case STEREO_ALTERNATING:
        src = static_cast<const char *>(data[view][plane]);
        src_row_size = line_size[view][plane];
        src_offset = 0;
        break;
    case STEREO_TOP_BOTTOM:
    case STEREO_TOP_BOTTOM_HALF:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = line_size[0][plane];
        src_offset = view * lines * src_row_size;
        break;
    case STEREO_LEFT_RIGHT:
    case STEREO_LEFT_RIGHT_HALF:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = line_size[0][plane];
        src_offset = view * dst_row_width;
        break;
    case STEREO_EVEN_ODD_ROWS:
        src = static_cast<const char *>(data[0][plane]);
        src_row_size = 2 * line_size[0][plane];
        src_offset = view * line_size[0][plane];
        break;
    }

    Q_ASSERT(src);
    if (src_row_size == dst_row_size)
    {
        memcpy(dst, src + src_offset, lines * src_row_size);
    }
    else
    {
        size_t dst_offset = 0;
        for (size_t y = 0; y < lines; y++)
        {
            memcpy(dst + dst_offset, src + src_offset, dst_row_width * type_size);
            dst_offset += dst_row_size;
            src_offset += src_row_size;
        }
    }
}
