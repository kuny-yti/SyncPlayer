#include "thread.h"
#include <QWaitCondition>
#include <QMutex>

class thread_p
{
public:
    QMutex mutex;
    QWaitCondition cond;

    QMutex mutex_finish;
    bool finish_state;

    bool state;
};

thread::thread()
{
    d = new thread_p;
}

thread::~thread()
{
    end();
    delete d;
}

void thread::begin(Priority p)
{
    d->state = true;
    d->finish_state = false;
    this->start(p);
}
void thread::end()
{
    d->state = false;
    d->finish_state = true;
    d->cond.wakeOne();
    this->wait();
}

void thread::update()
{
    d->cond.wakeOne();
}
void thread::finish()
{
    while (1)
    {
        d->mutex_finish.lock();
        if (d->finish_state)
        {
            d->finish_state = false;
            break;
        }
        d->mutex_finish.unlock();
    }
}

void thread::run()
{
    while (d->state)
    {
        d->mutex.lock();
        d->cond.wait(&d->mutex);
        d->mutex.unlock();

        if (this->exec())
        {
        }
        d->mutex_finish.lock();
        d->finish_state = true;
        d->mutex_finish.unlock();
    }
}
