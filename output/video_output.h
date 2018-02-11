#ifndef VIDEO_OUTPUT_H
#define VIDEO_OUTPUT_H

#include "blob.h"

#include "color_handel.h"
#include "input_handel.h"
#include "video_frame.h"
#include "subtitle_box.h"
#include "QOpenGLWidget"
#include <QMutex>
#include <QWaitCondition>

class video_output : public QOpenGLWidget
{
    Q_OBJECT
public:
    video_output(QWidget *p = NULL);
    ~video_output();
    void play_state(bool s)
    {
       is_play = s;
    }
    virtual void prepare_next_frame(const video_frame &frame,
                                    const subtitle_box &subtitle);

    virtual void initializeGL();
    virtual void paintGL();

    virtual void keyPressEvent(QKeyEvent *);

signals:
    void sig_key_event(QKeyEvent *);

private:
    input_handel *input;
    color_handel *color;
    bool is_play;

    video_frame frame_temp;

};

#endif // VIDEO_OUTPUT_H
