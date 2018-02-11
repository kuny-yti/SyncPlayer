#ifndef THREAD_H
#define THREAD_H

#include <QThread>

class thread :public QThread
{
private:
    class thread_p *d;

    void run();

public:
    thread();
    ~thread();

//protected:
    void begin(QThread::Priority p = QThread::InheritPriority);
    void end();

    void update();
    void finish();

protected:
    virtual int exec() = 0;

};

#endif // THREAD_H
