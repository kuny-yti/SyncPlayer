#ifndef SUBTITLE_UPDATER_H
#define SUBTITLE_UPDATER_H

#include "thread_base.h"
#include "subtitle_box.h"
#include "parameters.h"
#include "blob.h"
#include "subtitle_renderer.h"

class subtitle_updater : public thread
{
private:
    // The last rendered subtitle
    subtitle_box _last_subtitle;
    qint64 _last_time;
    parameters _last_params;
    int _last_outwidth;
    int _last_outheight;
    float _last_pixel_ar;
    // The current subtitle to render
    subtitle_box _subtitle;
    qint64 _timestamp;
    parameters _params;
    int _outwidth;
    int _outheight;
    float _pixel_ar;
    // The renderer and rendering buffer
    subtitle_renderer* _renderer;
    blob _buffer;
    int _bb_x, _bb_y, _bb_w, _bb_h;
    bool _buffer_changed;

public:
    subtitle_updater(subtitle_renderer* sr);

    // Reset
    void reset();
    // Set all necessary information to render the next subtitle.
    void set(const subtitle_box& subtitle, qint64 timestamp,
            const parameters& params, int outwidth, int outheight, float pixel_ar);
    // Start the subtitle rendering with thread::start(), which will execute the run() fuction.
    // The wait for it to finish using the thread::finish() function.
    virtual void run();
    // Find out if the buffer changed, and get all required info.
    bool get(int *outwidth, int* outheight,
            void** ptr, int* bb_x, int* bb_y, int* bb_w, int* bb_h);
};

#endif // SUBTITLE_UPDATER_H
