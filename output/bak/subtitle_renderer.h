#ifndef SUBTITLE_RENDERER_H
#define SUBTITLE_RENDERER_H
#include "thread_base.h"
#include "subtitle_box.h"
#include "parameters.h"

extern "C"
{
    #include <ass/ass.h>
}

class subtitle_renderer;

class subtitle_renderer_initializer : public thread
{
private:
    subtitle_renderer &_subtitle_renderer;

public:
    subtitle_renderer_initializer(subtitle_renderer &renderer);
    void run();
};

class subtitle_renderer
{
private:
    // Initialization
    subtitle_renderer_initializer _initializer;
    bool _initialized;
    const char *_fontconfig_conffile;
    const char *get_fontconfig_conffile();
    void init();
    friend class subtitle_renderer_initializer;

    // Static ASS data
    ASS_Library *_ass_library;
    ASS_Renderer *_ass_renderer;

    // Dynamic data (changes with each subtitle)
    SubtitleFormat _fmt;
    ASS_Track *_ass_track;
    ASS_Image *_ass_img;
    const subtitle_box *_img_box;
    int _bb_x, _bb_y, _bb_w, _bb_h;

    // ASS helper functions
    void blend_ass_image(const ASS_Image *img, quint32 *buf);
    void set_ass_parameters(const parameters &params);

    // Rendering ASS and text subtitles
    bool prerender_ass(const subtitle_box &box, qint64 timestamp,
            const parameters &params,
            int width, int height, float pixel_aspect_ratio);
    void render_ass(quint32 *bgra32_buffer);

    // Rendering bitmap subtitles
    bool prerender_img(const subtitle_box &box);
    void render_img(quint32 *bgra32_buffer);

public:
    subtitle_renderer();
    ~subtitle_renderer();

    bool is_initialized();

    bool render_to_display_size(const subtitle_box &box) const;

    bool prerender(const subtitle_box &box, qint64 timestamp,
            const parameters &params,
            int width, int height, float pixel_aspect_ratio,
            int &bb_x, int &bb_y, int &bb_w, int &bb_h);

    void render(quint32 *bgra32_buffer);
};

#endif // SUBTITLE_RENDERER_H
