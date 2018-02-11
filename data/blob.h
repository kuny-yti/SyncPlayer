#ifndef BLOB_H
#define BLOB_H
#include <QtGlobal>
#include <QString>
#include "type.h"

class blob
{
private:

    long _size;
    void* _ptr;

    static void* alloc(long s)
    {
        if (!s) return 0;

        void* ptr = malloc(s);
        if (s != 0 && !ptr)
        {
            DEBUG( QString("malloc fail! %1").arg(s));
        }
        return ptr;
    }

    static void* realloc(void* p, size_t s)
    {
        void* ptr = 0;
        /*if (s && !p)
            ptr= alloc(s);
        else*/
            ptr = ::realloc(p, s);

        if (s != 0 && !ptr)
        {
            DEBUG( QString("realloc fail! [%1]:%2").arg((char*)p).arg(s));
        }
        return ptr;
    }

public:

    blob() throw () :
        _size(0), _ptr(0)
    {
    }

    blob(long s) :
        _size(s), _ptr(alloc(_size))
    {
    }

    blob(long s, long n) :
        _size(s * n), _ptr(alloc(_size))
    {
    }

    blob(long s, long n0, long n1) :
        _size(s *n0 *n1), _ptr(alloc(_size))
    {
    }

    blob(long s, long n0, long n1, long n2) :
        _size(s *n0 *n1*n2), _ptr(alloc(_size))
    {
    }

    blob(const blob& b) :
        _size(b._size), _ptr(alloc(_size))
    {
        if (_size > 0)
        {
            memcpy(_ptr, b._ptr, _size);
        }
    }

    const blob& operator=(const blob& b)
    {
        if (b.size() == 0)
        {
            free(_ptr);
            _size = 0;
            _ptr = NULL;
        }
        else
        {
            void *ptr = alloc(b.size());
            memcpy(ptr, b.ptr(), b.size());
            free(_ptr);
            _ptr = ptr;
            _size = b.size();
        }
        return *this;
    }

    ~blob() throw ()
    {
        free(_ptr);
    }

    void resize(long s)
    {
        _ptr = realloc(_ptr, s);
        _size = s;
    }

    void resize(long s, long n)
    {
        _ptr = realloc(_ptr, s* n);
        _size = s;
    }

    void resize(long s, long n0, long n1)
    {
        _ptr = realloc(_ptr, s*n0* n1);
        _size = s;
    }

    void resize(long s, long n0, long n1, long n2)
    {
        _ptr = realloc(_ptr, s* n0*n1* n2);
        _size = s;
    }

    long size() const throw ()
    {
        return _size;
    }

    const void* ptr(long offset = 0) const throw ()
    {
        return static_cast<const void*>(static_cast<const char*>(_ptr) + offset);
    }

    void* ptr(long offset = 0) throw ()
    {
        return static_cast<void*>(static_cast<char*>(_ptr) + offset);
    }

    template<typename T>
    const T* ptr(long offset = 0) const throw ()
    {
        return static_cast<const T*>(ptr(offset * sizeof(T)));
    }

    template<typename T>
    T* ptr(long offset = 0) throw ()
    {
        return static_cast<T*>(ptr(offset * sizeof(T)));
    }
};

#endif // BLOB_H
