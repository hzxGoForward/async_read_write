#pragma once
#pragma once
#ifndef _THREAD_SAFE_QUEUE_H
#define _THREAD_SAFE_QUEUE_H

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>


template <typename T>
class CThreadSafeQueue
{
#define GUARD_LOCK std::lock_guard<std::mutex> lg(m_queueMtx)
    using value_type = T;
    using size_type = typename std::list<T>::size_type;

public:
    explicit CThreadSafeQueue(size_type maxSz = std::numeric_limits<size_type>::max()) : m_maxSize(maxSz), m_writeEnd(false) {}

    size_type size() const
    {
        GUARD_LOCK;
        return m_queue.size();
    }

    size_type maxSize() const
    {
        GUARD_LOCK;
        return m_maxSize;
    }

    bool empty() const
    {
        GUARD_LOCK;
        return m_queue.empty();
    }

    bool full() const
    {
        GUARD_LOCK;
        return m_queue.size() >= m_maxSize;
    }

    bool push(const T &val)
    {
        GUARD_LOCK;
        if (m_queue.size() >= m_maxSize)
            return false;
        m_queue.push_back(val);
        return true;
    }

    bool pop(T &val)
    {
        GUARD_LOCK;
        if (m_queue.empty())
            return false;
        val = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    bool front(T &t)
    {
        GUARD_LOCK;
        if (m_queue.empty())
            return false;
        t = m_queue.front();
        return true;
    }

    void set_end()
    {
        GUARD_LOCK;
        m_writeEnd = true;
    }

    bool is_end()
    {
        GUARD_LOCK;
        return m_writeEnd;
    }

    std::mutex &mutex() { return m_queueMtx; }
    std::list<T> &content() { return m_queue; }

private:
protected:
    mutable std::mutex m_queueMtx;
    std::list<T> m_queue;
    size_type m_maxSize;
    bool m_writeEnd;

#undef GUARD_LOCK
};


typedef std::shared_ptr<char> char_ptr_t;

template <typename T>
std::shared_ptr<T> make_shared_array(size_t size)
{
    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
}

struct CDataPkg
{
    int64_t pos;
    int64_t length;
    char_ptr_t data;
    bool ready;
    CDataPkg(int64_t a = -1, std::streamsize sz = -1, char *s = nullptr) : pos(a), length(sz), ready(false)
    {
        data = make_shared_array<char>(length);
    }
};


typedef std::shared_ptr<CDataPkg> CDataPkg_ptr_t;
typedef std::shared_ptr<CThreadSafeQueue<CDataPkg_ptr_t> > CThreadsafeQueue_ptr;

#endif
