#include "thread_base.h"
#include "type.h"

#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sched.h>

const pthread_mutex_t mutex::_initializer = PTHREAD_MUTEX_INITIALIZER;
const pthread_cond_t condition::_initializer = PTHREAD_COND_INITIALIZER;
const int thread::priority_default;
const int thread::priority_min;

mutex::mutex() : _mutex(_initializer)
{
    int e = pthread_mutex_init(&_mutex, NULL);
    if (e != 0)
        DEBUG( QString("System function failed: ")
            + "pthread_mutex_init(): " +QString::number(e));
}

mutex::mutex(const mutex&) : _mutex(_initializer)
{
    int e = pthread_mutex_init(&_mutex, NULL);
    if (e != 0)
        DEBUG( QString("System function failed: ")
            + "pthread_mutex_init(): " +QString::number(e));
}

mutex::~mutex()
{
    (void)pthread_mutex_destroy(&_mutex);
}

void mutex::lock()
{
    int e = pthread_mutex_lock(&_mutex);
    if (e != 0)
        DEBUG( QString("System function failed: ")
            + "pthread_mutex_lock(): " + QString::number(e));
}

bool mutex::trylock()
{
    return (pthread_mutex_trylock(&_mutex) == 0);
}

void mutex::unlock()
{
    int e = pthread_mutex_unlock(&_mutex);
    if (e != 0)
        DEBUG( QString("System function failed: ")
                + "pthread_mutex_unlock(): " + QString::number(e));
}

condition::condition() : _cond(_initializer)
{
    int e = pthread_cond_init(&_cond, NULL);
    if (e != 0)
        DEBUG( QString("System function failed: ")
                + "pthread_cond_init(): " + QString::number(e));
}

condition::condition(const condition&) : _cond(_initializer)
{
    int e = pthread_cond_init(&_cond, NULL);
    if (e != 0)
        DEBUG( QString("System function failed: ")
                + "pthread_cond_init(): " +QString::number(e));
}

condition::~condition()
{
    (void)pthread_cond_destroy(&_cond);
}

void condition::wait(mutex& m)
{
    int e = pthread_cond_wait(&_cond, &m._mutex);
    if (e != 0)
        DEBUG( QString("System function failed: ")
                + "pthread_cond_wait(): " + QString::number(e));
}

void condition::wake_one()
{
    int e = pthread_cond_signal(&_cond);
    if (e != 0)
        DEBUG( QString("System function failed: ")
                + "pthread_cond_signal(): " + QString::number(e));
}

void condition::wake_all()
{
    int e = pthread_cond_broadcast(&_cond);
    if (e != 0)
        DEBUG( QString("System function failed: ")
                + "pthread_cond_broadcast(): " + QString::number(e));
}

thread::thread() :
    __thread_id(pthread_self()),
    __joinable(false),
    __running(false),
    __wait_mutex()
{
}

thread::thread(const thread&) :
    __thread_id(pthread_self()),
    __joinable(false),
    __running(false),
    __wait_mutex()
{
    // The thread state cannot be copied; a new state is created instead.
}

thread::~thread()
{
    if (__joinable)
        (void)pthread_detach(__thread_id);
}

void* thread::__run(void* p)
{
    thread* t = static_cast<thread*>(p);
    try
    {
        t->run();
    }
    catch (QString& e)
    {
        t->__exc = e;
    }
    catch (...)
    {
        t->__running = false;
        //throw;
    }
    t->__running = false;
    return NULL;
}

void thread::start(int priority)
{
    if (__running.bool_compare_swap(false, true))
    {
        wait();
        int e;
        pthread_attr_t priority_thread_attr;
        pthread_attr_t* thread_attr = NULL;
        if (priority != priority_default)
        {
            int policy = 0, min_priority = 0;
            struct sched_param param;
            e = pthread_attr_init(&priority_thread_attr);
            e = e || pthread_attr_getschedpolicy(&priority_thread_attr, &policy);
            if (e == 0)
            {
                min_priority = sched_get_priority_min(policy);
                if (min_priority == -1)
                    e = errno;
            }
            e = e || pthread_attr_getschedparam(&priority_thread_attr, &param);
            if (e == 0)
            {
                param.sched_priority = min_priority;
            }
            e = e || pthread_attr_setschedparam(&priority_thread_attr, &param);
            if (e != 0) {
                DEBUG( QString("System function failed: ")
                        + "pthread_attr_*(): " + QString::number(e));
            }
            thread_attr = &priority_thread_attr;
        }
        e = pthread_create(&__thread_id, thread_attr, __run, this);
        if (e != 0) {
            DEBUG( QString("System function failed: ")
                    + "pthread_create(): " +  QString::number(e));
        }
        __joinable = true;
    }
}

void thread::wait()
{
    __wait_mutex.lock();
    if (__joinable.bool_compare_swap(true, false))
    {
        int e = pthread_join(__thread_id, NULL);
        if (e != 0) {
            __wait_mutex.unlock();
            DEBUG(  QString("System function failed: ")
                    + "pthread_join(): " +  QString::number(e));
        }
    }
    __wait_mutex.unlock();
}

void thread::finish()
{
    wait();
    if (!__exc.isEmpty())
        DEBUG( __exc);
}

void thread::cancel()
{
    __wait_mutex.lock();
    int e = pthread_cancel(__thread_id);
    if (e != 0)
    {
        __wait_mutex.unlock();
        DEBUG( QString("System function failed: ")
                + "pthread_cancel(): " +  QString::number(e));
    }
    __wait_mutex.unlock();
}
