#ifndef SUBTITLE_UPDATER_H
#define SUBTITLE_UPDATER_H
#include "thread_base.h"
#include "blob.h"
#include "subtitle_box.h"
#include "subtitle_renderer.h"

class subtitle_updater : public thread
{
private:
    subtitle_box _last_subtitle;
    qint64 _last_timestamp;
    parameters _last_params;
    int _last_outwidth;
    int _last_outheight;
    float _last_pixel_ar;

    subtitle_box _subtitle;
    qint64 _timestamp;
    parameters _params;
    int _outwidth;
    int _outheight;
    float _pixel_ar;

    subtitle_renderer* _renderer;
    blob _buffer;
    int _bb_x, _bb_y, _bb_w, _bb_h;
    bool _buffer_changed;

public:
    subtitle_updater(subtitle_renderer* sr);

    void reset();
    void set(const subtitle_box& subtitle, qint64 timestamp,
            const parameters& params, int outwidth, int outheight,
             float pixel_ar);
    virtual void run();
    bool get(int *outwidth, int* outheight,
            void** ptr, int* bb_x, int* bb_y, int* bb_w, int* bb_h);
};

#endif // SUBTITLE_UPDATER_H
