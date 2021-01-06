#include "buff.h"
#include <iostream>

ReadBuff::ReadBuff(CS &readPath, CS &writePath, const std::streamsize cap, const size_t poolCnt, const size_t batchsize)
    : m_readPath(readPath),m_writePath(writePath), m_readBuffCap(cap), m_writeBuffCap(cap), m_readBuffSz(0), m_writeBuffSz(0)
{
    m_readFs.open(m_readPath, std::ios::in);
    if (m_readFs.is_open() == false)
    {
        std::cout<<"open file failed : "<<m_readPath<<std::endl;
        m_readFileFinish.store(true);
        return;
    }

    m_vReadFileQueueMtx.reserve(poolCnt);
    m_vWriteBuffQueueMtx.reserve(poolCnt);
    
    for (size_t i = 0; i < poolCnt; ++i)
    {
        m_vReadFileQueueMtx.emplace_back(new std::mutex());
        m_vWriteBuffQueueMtx.emplace_back(new std::mutex());
        m_vReadFileQueue.emplace_back(std::queue<dataRef>());
        m_vWriteBuffQueue.emplace_back(std::queue<dataRef>());
    }

    // todo: move to function
    m_writeFileIter = 0;
    m_batchsize = batchsize;

    // atomic variables
    m_readFileIter.store(0);
    m_readBuffIter.store(0);
    m_readFileFinish.store(false);
    m_writeBuffFinish.store(false);
    
    m_writeFileFinish = false;
    
}

ReadBuff::~ReadBuff()
{
    if (m_readFs.is_open())
        m_readFs.close();
    if (m_writeFs.is_open())
        m_writeFs.close();
}


// read is just support single thread
std::streamsize ReadBuff::readfileToBuff(std::string &state, const std::streamsize len)
{
    std::streamsize readlen = -1;
    std::cout<<"enter readfileToBuff func " <<std::endl;
    if (!m_readFs.is_open())
    {
        state = m_readPath + " is not open ";
        throw std::runtime_error(state);
        return readlen;
    }
    if (m_readBuffSz + len > m_readBuffCap)
    {
        state = "out of memory catche";
        return readlen;
    }
    else if (m_readFileFinish.load())
    {
        state = "read already finished";
        return readlen;
    }
    std::cout<<"start reading " <<std::endl;
    dataRef buf = make_shared_array<char>(len);
    m_readFs.read(buf.get(), len);

    std::cout<<"start to store , queue size: " <<m_vReadFileQueue.size()<<std::endl;
    // search a queue to store data
    bool store = false;
    while (!store)
    {
        for (size_t i = 0; i < m_vReadFileQueue.size() && !store; ++i)
        {
            std::cout<<"query "<< i << " queue to store \n";
            if (m_vReadFileQueueMtx[i]->try_lock())
            {
                std::cout<<"enter a lock "<< i << "\n";
                std::lock_guard<std::mutex> lg(*m_vReadFileQueueMtx[i], std::adopt_lock);
                m_vReadFileQueue[i].push(buf);
                m_readBuffSz += len;
                m_readFileIterMap[m_readFileIter.load()] = i;
                std::cout<<"m_readFilIter index: " << m_readFileIter.load()<<std::endl;
                m_readFileIter.store(m_readFileIter.load() + 1);
                store = true;
            }
        }
    }
    // if reading is end
    if (!m_readFs.good())
    {
        std::cout<<"---------file reading finish------m_readFileIter: "<< m_readFileIter.load()<<std::endl;
        m_readFileFinish.store(true);
        m_readFs.close();
    }
    std::cout<<"read data to --------------------" << m_readFileIter.load()<<std::endl;
    // std::cout<<buf<<std::endl;
    return len;
}

// get nextBuff and its dataindex
std::pair<dataRef, int> ReadBuff::getNextBuff(std::string &state)
{
    std::cout<<"get buff "<<std::endl;
    dataRef buf = nullptr;
    int dataIndex = -1;
    {
        std::lock_guard<std::mutex> lg1(m_readBuffIterMtx);
        
        std::cout<<"m_readBuffIter index: " << m_readBuffIter.load()<<std::endl;
        for(auto&e: m_readFileIterMap){
            std::cout<<e.first << " "<<e.second <<std::endl;;
        }
        if (m_readFileIterMap.count(m_readBuffIter.load()) == 0)
        {
            state = "data has not been prepared";
            return {buf, dataIndex};
        }
        else if (m_readBuffFinish.load())
        {
            state = "data has already been taken, it's empty";
            return {buf, dataIndex};
        }
        // reading data from queue
        dataIndex = m_readBuffIter.load();
        size_t idx = m_readFileIterMap[m_readBuffIter.load()];
        std::lock_guard<std::mutex> lg2(*m_vReadFileQueueMtx[idx]);
        if (m_vReadFileQueue[idx].empty())
        {
            throw std::runtime_error("queue is empty while reading");
        }
        buf = m_vReadFileQueue[idx].front();
        m_vReadFileQueue[idx].pop();
        m_readBuffSz -= sizeof(buf.get());
        m_readFileIterMap.erase(dataIndex);
        m_readBuffIter.store(dataIndex+1);
        if (m_readBuffIter.load() >= m_readFileIter.load() && m_readFileFinish)
        {
            m_readBuffFinish.store(true);
            std::cout<<"all buff has been taken \n "<<std::endl;
        }
    }
    std::cout<<"get buff finished, from ------------------" << m_readBuffIter.load()-1<<std::endl;
    return {buf, dataIndex};
}

// write process result to a specific wirte memory pool
bool ReadBuff::writebuff(dataRef buf, std::string &state, int writeQueueIndex, int dataIndex)
{
    std::cout<<"start to write to buff *********\n";
    bool res = false;
    if (buf == nullptr)
    {
        state = "data is empty";
        return res;
    }
    else if (m_writeBuffSz + sizeof(buf.get()) > m_writeBuffCap)
    {
        state = "out of memory";
        return res;
    }
    else if (m_writeBuffFinish.load())
    {
        state = "write buff finish";
        return res;
    }

    std::cout<<"wait to write to buff *********\n";
    // wait to store data
    {
        std::cout<<"enter lock\n";
        std::lock_guard<std::mutex> lg(*m_vWriteBuffQueueMtx[writeQueueIndex]);
        m_vWriteBuffQueue[writeQueueIndex].push(buf);
        m_writeBuffSz += sizeof(buf);
        m_writeBuffMap[dataIndex] = writeQueueIndex;
        std::cout<<"go out a lock, write data index: "<<dataIndex<<std::endl;
    }
    if (m_readFileFinish.load() && dataIndex == m_readFileIter.load()-1)
    {
        std::cout<<"write buff finished %%%%%%%%%%%%%%%%%%\n";
        std::cout<<"m_readBuffIter: "<< m_readBuffIter.load()<<" dataindex: "<<dataIndex<<std::endl;
        m_writeBuffFinish.store(true);
    }
    std::cout<<"write buff to queue ------------" << writeQueueIndex <<" queue, index: " << dataIndex<<std::endl;
    return true;
}

std::streamsize ReadBuff::writeFileFromBuff(std::string &state)
{
    std::cout<<"wrtting file"<<std::endl;
    std::streamsize writelen = -1;
    if (m_writeFileFinish)
    {
        state = "write finished";
        return writelen;
    }
    else if(m_writeFileIter == 0){
        std::cout<<"create file *************"<<m_writePath<<std::endl;
        m_writeFs.open(m_writePath, std::ios::out|std::ios::ate);
        if(m_writeFs.is_open() == false){
            throw std::runtime_error("write file could not be opened\n");
        }
    }
    else if (m_writeBuffMap.count(m_writeFileIter) == 0)
    {
        state = "data have not been processed";
        std::cout<<"writeFileIter: "<<m_writeFileIter<<std::endl;
        return writelen;
    }

    for(auto&e: m_writeBuffMap){
            std::cout<<e.first << " "<<e.second <<std::endl;;
        }
    // read data
    dataRef buf = nullptr;
    size_t idx = m_writeBuffMap[m_writeFileIter];
    {
        std::cout<<"enter lock\n";
        std::lock_guard<std::mutex> lg(*m_vWriteBuffQueueMtx[idx]);
        if (m_vWriteBuffQueue[idx].empty())
        {
            throw std::runtime_error("queue is empty while reading");
        }
        std::cout<<"gou out lock, m_writeFileIter: "<<m_writeFileIter<<std::endl;
        buf = m_vWriteBuffQueue[idx].front();
        m_vWriteBuffQueue[idx].pop();
        m_writeBuffMap.erase(m_writeFileIter++);
        m_writeBuffSz -= sizeof(buf.get());
        
    }

    // write data
    if (buf)
    {
        // std::dynamic_pointer_cast<char[]>
        writelen = std::string(buf.get()).size();
        m_writeFs.write(buf.get(), writelen);
        std::cout<<"----------write len: "<<writelen<<std::endl;
        std::cout<<buf.get()<<std::endl;
    }

    // check wheter is finish
    if (m_writeBuffFinish.load() &&  m_writeFileIter == m_readFileIter.load())
    {
        m_writeFs.close();
        m_writeFileFinish = true;
        std::cout<<"writting file finish----------------------\n";
    }
    
    return writelen;
}
