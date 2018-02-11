#include "video_output.h"
#include <QThread>
#include <QMouseEvent>
#include "command.h"

class locks
{
public:
    locks()
    {
        mutex = false;
        condi = false;
    }
    ~locks()
    {

    }

    void lock()
    {
        while (!__sync_bool_compare_and_swap(&mutex, false, true))
            sched_yield();
    }
    void unlock()
    {
        __sync_bool_compare_and_swap(&mutex, true, false);
    }

    void wait()
    {
        lock();
        while (!__sync_bool_compare_and_swap(&condi, false, true))
            sched_yield();
        unlock();
    }
    void wake()
    {
        __sync_bool_compare_and_swap(&condi, true, false);
    }

    void qlock()
    {
        qmutex.lock();
    }
    void qunlock()
    {
        qmutex.unlock();
    }

    void qwait()
    {
        qlock();
        qcondi.wait(&qmutex, 100);
        qunlock();
    }
    void qwake()
    {
        qlock();
        qcondi.wakeAll();
        qunlock();
    }

private:
    volatile bool mutex;
    volatile bool condi;

    QMutex qmutex;
    QWaitCondition qcondi;
};

static locks sync_video;
static QMutex upmutex;

video_output::video_output(QWidget *p):
    QOpenGLWidget(p)
{
    input = 0;
    color = 0;
    is_play = false;

    if (!input)
        input = new input_handel();
    if (!color)
        color = new color_handel(input);

    if (cmdcfg[Command_FullScreen].toBool())
    {
        setCursor (Qt::BlankCursor);
    }
}
video_output::~video_output()
{
    is_play = false;
    sync_video.qwake();
    if (input)
        delete input;
    if (color)
        delete color ;
}


void video_output::prepare_next_frame(const video_frame &frame, const subtitle_box &)
{
    if (!is_play)
        return;
    if (!frame.is_valid())
        return;

    frame_temp = frame;
    this->update();
    sync_video.qwait();
}

void video_output::keyPressEvent(QKeyEvent *ke)
{
    emit sig_key_event(ke);
}
void video_output::initializeGL()
{

}


void video_output::paintGL()
{
    try
    {
        if (is_play)
        {
            if (!frame_temp.is_valid())
                return;
            if (upmutex.tryLock (40))
            {
                input->handel(frame_temp);
                color->handel(frame_temp);
                color->draw(frame_temp);
                upmutex.unlock ();
            }
            sync_video.qwake();
        }
    }
    catch (const QString &se)
    {
        qDebug() << "paint error:" << se;
    }
}
