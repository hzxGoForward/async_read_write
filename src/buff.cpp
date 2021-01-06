#include "buff.h"

ReadBuff::ReadBuff(CS& readPath, CS& writePath, const std::streamsize cap, const size_t poolCnt, const size_t batchsize)
    : m_readPath(readPath), m_readBuffCap(cap), m_readBuffSz(0)
{
    m_readFs.open(m_readPath, std::ios::in);
    if (m_readFs.is_open() == false)
    {
        m_readfinish = true;
        return;
    }
    else
    {
        m_readfinish = false;
    }
    m_vReadBuffQueue.resize(poolCnt);
    m_vWriteBuffQueue.resize(poolCnt);
    // m_vReadBuffQueueMtx.reserve(poolCnt);
    // m_vWriteBuffMtx.resize(poolCnt);

    // todo: move to function
    m_readBuffIter = 0;
    m_processIter = 0;
    m_writeFileIter = 0;
    m_batchsize = batchsize;
}

ReadBuff::~ReadBuff()
{
    if (m_readFs.is_open())
        m_readFs.close();
    if (m_writeFs.is_open())
        m_writeFs.close();
}

bool ReadBuff::is_open() const
{
    return m_readFs.is_open();
}

bool ReadBuff::is_readfinish() const
{
    return m_readfinish;
}

bool ReadBuff::is_writefinish() const
{
    return m_writefinish;
}

// read is just support single thread
std::streamsize ReadBuff::readfile(std::string& state, const std::streamsize len)
{
    std::streamsize readlen = -1;
    if (!is_open())
    {
        state = "file is not open";
        return readlen;
    }
    if (m_readBuffSz + len > m_readBuffCap)
    {
        state = "out of memory catche";
        return readlen;
    }
    else if (m_readfinish.load())
    {
        state = "read finished";
        return readlen;
    }

    dataRef buf = std::make_shared<char>(new char[len]);
    m_readFs.read(buf.get(), len);
    // if reading is end
    if (m_readFs.eof())
    {
        m_readfinish.store(true);
        m_readFs.close();
    }

    // search a queue to store data
    bool store = false;
    while (!store)
    {
        for (size_t i = 0; i < m_vReadBuffQueue.size() && !store; ++i)
        {
            if (m_vReadBuffQueueMtx[i].try_lock())
            {
                std::lock_guard<std::mutex> lg(m_vReadBuffQueueMtx[i], std::adopt_lock);
                m_vReadBuffQueue[i].push(buf);
                m_readBuffSz += readlen;
                m_readBuffIterMap[m_readBuffIter.load()] = i;
                m_readBuffIter.store(m_readBuffIter.load() + 1);
                store = true;
            }
        }
    }
    return sizeof(buf.get());
}

dataRef ReadBuff::getNextBuff(std::string& state)
{
    dataRef buf = nullptr;
    {
        std::lock_guard<std::mutex> lg1(m_processMtx);
        if (m_processIter > m_readBuffIter.load())
        {
            if (m_readfinish.load())
                state = "all data have been taken";
            else
            {
                state = "data has not prepared";
            }
            return buf;
        }

        // reading data from queue
        size_t idx = m_readBuffIterMap[m_processIter];
        std::lock_guard<std::mutex> lg2(m_vReadBuffQueueMtx[idx]);
        if (m_vReadBuffQueue[idx].empty())
        {
            throw std::runtime_error("queue is empty while reading");
        }
        buf = m_vReadBuffQueue[idx].front();
        m_vReadBuffQueue[idx].pop();
        m_readBuffIterMap.erase(++m_processIter);
    }
    return buf;
}

// write process result to mempool
bool ReadBuff::writebuff(dataRef buf, std::string& state)
{
    bool res = false;
    if (buf == nullptr)
    {
        state = "data is empty";
        return res;
    }
    else if (m_writeBuffSz + sizeof(buf.get()) > m_writeBuffSz)
    {
        state = "out of memory";
        return res;
    }
    bool store = false;
    while(!store)
    {
        for (size_t i = 0; i < m_vWriteBuffQueue.size() && !store; ++i)
        {
            if (m_vWriteBuffMtx[i].try_lock())
            {
                std::lock_guard<std::mutex> lg(m_vWriteBuffMtx[i], std::adopt_lock);
                m_vWriteBuffQueue[i].push(buf);
                m_writeBuffSz += sizeof(buf);
                m_readBuffIterMap[m_writeBuffIter.load()] = i;
                m_writeBuffIter.store(m_writeBuffIter.load() + 1);
                store = true;
            }
        }
    }
    if (m_processfinish && m_writeBuffIter > m_processIter) {

    }
}

std::streamsize ReadBuff::writefile(std::string& state)
{
    std::streamsize writelen = -1;
    if (m_writefinish || m_writeFileIter > m_processIter)
    {
        state = "write finished";
        return writelen;
    }
    else if (m_writeMap.count(m_writeFileIter) == 0)
    {
        state = "data have not been processed";
        return writelen;
    }

    // read data
    dataRef buf = nullptr;
    size_t idx = m_writeMap[m_writeFileIter];
    {
        std::lock_guard<std::mutex> lg(m_vWriteBuffMtx[idx]);
        if (m_vWriteBuffQueue[idx].empty())
        {
            throw std::runtime_error("queue is empty while reading");
        }
        buf = m_vWriteBuffQueue[idx].front();
        m_vWriteBuffQueue[idx].pop();
    }

    // write data
    if (buf)
    {
        m_writeFs.write(buf.get(), sizeof(buf.get()));
    }

    // check wheter is finish
    if (++m_writeFileIter == m_processIter && m_processfinish)
    {
        m_writeFs.close();
        m_writefinish = true;
    }
}
