#ifndef READ_THREAD_H
#define READ_THREAD_H

#include "thread_base.h"

//! 读取线程
//! 从 avformatcontext 读取数据包存储到队列中
//!

class read_thread_p;
class read_thread : public thread
{
private:
    read_thread_p *_dptr;
public:
    explicit read_thread(const QString &url, bool is_device,
                         class ffmpeg_stuff *ffmpeg);

    void run();
    void reset();
    bool eof() const;

};

#endif // READ_THREAD_H
