#include <mutex>
#include <list>
#include <numeric>
#include <memory>




typedef std::shared_ptr<char> SPChar;
class CSyncQueue;
typedef std::shared_ptr<CSyncQueue> QueRef;
struct CDataPkg
{
    uint64_t pos;
    uint64_t length;
    SPChar data;
    CDataPkg(int a = -1, std::streamsize sz = -1, char *s = nullptr) : pos(a), length(sz) { data.reset(s); }
};
typedef std::shared_ptr<CDataPkg> dataRef;

class CSyncQueue
{
#define GUARD_LOCK std::lock_guard<std::mutex> lg(m_queueMtx)
public:


public:
    CSyncQueue(uint64_t cap = -1) : m_buffCap(cap), m_queue(), m_queueMtx()
    {
        m_curSz = 0;
        m_writeEnd = false;
    }

     std::size_t size() {
        GUARD_LOCK;
        return m_queue.size();
    }

     bool empty()
    { 
        GUARD_LOCK;
        return m_queue.empty();
    }
     bool push(const dataRef val, const uint64_t len)
    {
        GUARD_LOCK;
        if (m_curSz < 0 || m_curSz > m_buffCap)
            throw std::runtime_error("size error");   
        else if (m_curSz + len > m_buffCap && m_buffCap!=-1)
            return false;
        m_queue.push_back(val);
        m_curSz += len;
        return true;
    }

     bool pop(dataRef &val)
    { 
        GUARD_LOCK;
        if (m_queue.empty())
            return false;
        else if (m_curSz <= 0 || m_curSz > m_buffCap)
            throw std::runtime_error("size error");
        val = m_queue.front();
        m_queue.pop_front();
        m_curSz -= val->length;
        if (m_curSz < 0 || m_curSz > m_buffCap)
            throw std::runtime_error("size error");   
        return true;
    }

     uint64_t frontPos() {
         GUARD_LOCK;
         if (m_queue.empty())
             return -1;
         return m_queue.front()->pos;

     }

     void writeEnd() { 
         GUARD_LOCK;
         m_writeEnd = true;
     }

     bool is_end() { 
         GUARD_LOCK;
         return m_writeEnd;
     }

    std::mutex &mutex() { return m_queueMtx; }
    std::list<dataRef> &content() { return m_queue; }

protected:
    mutable std::mutex m_queueMtx;
    std::list<dataRef> m_queue;
    int64_t m_buffCap;
    int64_t m_curSz;
    bool m_writeEnd;

#undef GUARD_LOCK
};
