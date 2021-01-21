//#ifndef _SYNC_QUEUE_H
//#define _SYNC_QUEUE_H
//
//#include <condition_variable>
//#include <list>
//#include <memory>
//#include <mutex>
//#include <numeric>
//#include <functional>
//
//
//typedef std::shared_ptr<char> char_ptr_t;
//typedef std::shared_ptr<std::condition_variable> cond_var_ptr_t;
//
//
//template <typename T>
//std::shared_ptr<T> make_shared_array(size_t size)
//{
//    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
//}
//
//struct CDataPkg
//{
//    uint64_t pos;
//    uint64_t length;
//    char_ptr_t data;
//    CDataPkg(uint64_t a = -1, std::streamsize sz = -1, char *s = nullptr) : pos(a), length(sz)
//    {
//        data = make_shared_array<char>(length);
//    }
//};
//typedef std::shared_ptr<CDataPkg> CDataPkg_ptr_t;
//
//class CSyncQueue
//{
//#define GUARD_LOCK std::lock_guard<std::mutex> lg(m_queueMtx)
//public:
//    CSyncQueue(uint64_t cap = -1) : m_buffCap(cap), m_queue(), m_queueMtx()
//    {
//        m_curSz = 0;
//        m_writeEnd = false;
//    }
//
//    std::size_t size()
//    {
//        GUARD_LOCK;
//        return m_queue.size();
//    }
//
//    bool full()
//    {
//        GUARD_LOCK;
//        return m_curSz >= m_buffCap;
//    }
//
//    bool empty()
//    {
//        GUARD_LOCK;
//        return m_queue.empty();
//    }
//    bool push(const CDataPkg_ptr_t val, const uint64_t len)
//    {
//        {
//            GUARD_LOCK;
//            if (m_curSz < 0 || m_curSz > m_buffCap)
//                throw std::runtime_error("size error");
//            else if (m_curSz + len > m_buffCap && m_buffCap != -1)
//                return false;
//            m_queue.push_back(val);
//            m_curSz += len;
//        }
//        m_push_condVar.notify_one();
//        return true;
//    }
//
//    bool pop(CDataPkg_ptr_t &val)
//    {
//        bool notify_all = false;
//        {
//            GUARD_LOCK;
//            if (m_queue.empty())
//                return false;
//            else if (m_curSz <= 0 || m_curSz > m_buffCap)
//                throw std::runtime_error("size error");
//            val = m_queue.front();
//            m_queue.pop_front();
//            m_curSz -= val->length;
//            if (m_writeEnd && m_queue.empty())
//                notify_all = true;
//        }
//        if (notify_all)
//            m_pop_condVar.notify_all();
//        else
//            m_pop_condVar.notify_one();
//        return true;
//    }
//
//    uint64_t frontPos()
//    {
//        GUARD_LOCK;
//        if (m_queue.empty())
//            return -1;
//        return m_queue.front()->pos;
//    }
//
//    void writeEnd()
//    {
//        {
//            GUARD_LOCK;
//            m_writeEnd = true;
//        }
//        m_push_condVar.notify_all();
//        m_pop_condVar.notify_all();
//    }
//
//    bool is_end()
//    {
//        GUARD_LOCK;
//        return m_writeEnd;
//    }
//
//    std::mutex &mutex() { return m_queueMtx; }
//    std::list<CDataPkg_ptr_t> &content() { return m_queue; }
//    std::condition_variable &push_cond() { return m_push_condVar; }
//    std::condition_variable &pop_cond() { return m_pop_condVar; }
//
//private:
//
//protected:
//    mutable std::mutex m_queueMtx;
//    std::list<CDataPkg_ptr_t> m_queue;
//    int64_t m_buffCap;
//    int64_t m_curSz;
//    bool m_writeEnd;
//    std::condition_variable m_push_condVar;
//    std::condition_variable m_pop_condVar;
//
//#undef GUARD_LOCK
//};
//
//
//typedef std::shared_ptr<CSyncQueue> CSycnQueue_ptr_t;
//
//#endif
