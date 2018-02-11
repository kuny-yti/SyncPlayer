#ifndef THREAD_BASE_H
#define THREAD_BASE_H

#include <pthread.h>
#include <QString>

template<typename T>
class bs_atomic
{
public:
    bs_atomic()
    {ope_value = T(0);}
    bs_atomic(T v)
    {ope_value = v;}

    operator T ()
    {return ope_value;}
    operator const T ()const
    {return ope_value;}
    operator T ()volatile
    {return ope_value;}
    operator const T ()const volatile
    {return ope_value;}
public:
    // 后置
    T operator ++ (int)
    {return __sync_fetch_and_add (&ope_value, 1);}
    T operator -- (int)
    {return __sync_fetch_and_sub (&ope_value, 1);}
    T operator ++ (int)volatile
    {return __sync_fetch_and_add (&ope_value, 1);}
    T operator -- (int)volatile
    {return __sync_fetch_and_sub (&ope_value, 1);}

    // 前置
    T operator ++ ()
    {return __sync_add_and_fetch(&ope_value, 1);}
    T operator -- ()
    {return __sync_sub_and_fetch(&ope_value, 1);}
    T operator ++ ()volatile
    {return __sync_add_and_fetch(&ope_value, 1);}
    T operator -- ()volatile
    {return __sync_sub_and_fetch(&ope_value, 1);}

    T operator + (T value)
    {return __sync_fetch_and_add (&ope_value, value);}
    T operator - (T value)
    {return __sync_fetch_and_sub (&ope_value, value);}
    T operator + (T value)volatile
    {return __sync_fetch_and_add (&ope_value, value);}
    T operator - (T value)volatile
    {return __sync_fetch_and_sub (&ope_value, value);}

    T operator & (T value) //与
    {return __sync_fetch_and_and (&ope_value, value);}
    T operator | (T value) //或
    {return __sync_fetch_and_or (&ope_value, value);}
    T operator ^ (T value) //异或
    {return __sync_fetch_and_xor (&ope_value, value);}
    //T operator ~ (T value) //非
    //{return __sync_fetch_and_nand (&ope_value, value);}
    T operator & (T value)volatile //与
    {return __sync_fetch_and_and (&ope_value, value);}
    T operator | (T value)volatile //或
    {return __sync_fetch_and_or (&ope_value, value);}
    T operator ^ (T value)volatile //异或
    {return __sync_fetch_and_xor (&ope_value, value);}
    //T operator ~ (T value)volatile //非
    //{return __sync_fetch_and_nand (&ope_value, value);}

public:
    T operator = (T value)
    {return __sync_lock_test_and_set (&ope_value, value);}
    T operator = (T value)volatile
    {return __sync_lock_test_and_set (&ope_value, value);}

    bs_atomic<T> & operator = (const bs_atomic<T> &rhs)
    {__sync_lock_test_and_set (&ope_value, rhs.ope_value); return *this;}
    bs_atomic<T> & operator = (const bs_atomic<T> &rhs)volatile
    {__sync_lock_test_and_set (&ope_value, rhs.ope_value); return *this;}

public:
    T operator += (T value)
    {return __sync_add_and_fetch (&ope_value, value);}
    T operator -= (T value)
    {return __sync_sub_and_fetch (&ope_value, value);}
    T operator += (T value)volatile
    {return __sync_add_and_fetch (&ope_value, value);}
    T operator -= (T value)volatile
    {return __sync_sub_and_fetch (&ope_value, value);}

    T operator &= (T value) //与
    {return __sync_and_and_fetch (&ope_value, value);}
    T operator |= (T value) //或
    {return __sync_or_and_fetch (&ope_value, value);}
    T operator ^= (T value) //异或
    {return __sync_xor_and_fetch (&ope_value, value);}
    //T operator ~= (T value) //非
    //{return __sync_nand_and_fetch (&ope_value, value);}
    T operator &= (T value)volatile //与
    {return __sync_and_and_fetch (&ope_value, value);}
    T operator |= (T value)volatile //或
    {return __sync_or_and_fetch (&ope_value, value);}
    T operator ^= (T value)volatile //异或
    {return __sync_xor_and_fetch (&ope_value, value);}
    //T operator ~ (T value)volatile //非
    //{return __sync_fetch_and_nand (&ope_value, value);}

public:
    bool bool_compare_swap(bool oldval, bool newval)
    { return __sync_bool_compare_and_swap(&ope_value, oldval, newval);}
    T compare_swap(T oldval, T newval)
    {return __sync_val_compare_and_swap(&ope_value, oldval, newval);}
    bool bool_compare_swap(bool oldval, bool newval)volatile
    { return __sync_bool_compare_and_swap(&ope_value, oldval, newval);}
    T compare_swap(T oldval, T newval)volatile
    {return __sync_val_compare_and_swap(&ope_value, oldval, newval);}

private:
    T ope_value;
};
class mutex
{
private:
    static const pthread_mutex_t _initializer;
    pthread_mutex_t _mutex;

public:
    mutex();
    mutex(const mutex& m);
    ~mutex();

    // 锁定互斥。
    void lock();
    // 尝试锁定互斥锁。返回true表示成功，否则返回false。
    bool trylock();
    // 解锁互斥
    void unlock();

    friend class condition;
};

class condition
{
private:
    static const pthread_cond_t _initializer;
    pthread_cond_t _cond;

public:
    condition();
    condition(const condition& c);
    ~condition();

    // 等待状态。调用线程必须拥有该互斥锁。
    void wait(mutex& m);
    // 唤醒一个线程，但条件是等待。
    void wake_one();
    // 唤醒等待该条件的线程。
    void wake_all();
};

class thread
{
private:
    pthread_t __thread_id;
    bs_atomic<bool> __joinable;
    bs_atomic<bool> __running;
    mutex __wait_mutex;
    QString __exc;

    static void* __run(void* p);
public:
    // 优先权
    static const int priority_default = 0;
    static const int priority_min = 1;

    thread();
    thread(const thread& t);
    virtual ~thread();

    // 在子类中实现这个;
    virtual void run() = 0;

    // 开始执行该run（）函数的新线程。如果线程已经在运行，这个函数不执行任何操作。
    void start(int priority = thread::priority_default);

    // 返回该线程是否正在运行。
    bool is_running()
    {
        return __running;
    }

    // 等待线程结束。如果线程没有运行，这个函数立即返回。
    void wait();

    // 等待线程完成，就像等待（），并重新抛出的run（）函数可能会在执行过程中引发异常。
    void finish();

    // 取消一个线程。这是危险的，不应该被使用。
    void cancel();

};


#endif // THREAD_BASE_H
