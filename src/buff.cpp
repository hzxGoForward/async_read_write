//#include "buff.h"
//#include <iostream>
//#include <cstring>
//
//ReadBuff::ReadBuff(CS &readPath, CS &writePath, const std::streamsize cap, const size_t poolCnt, const size_t batchsize)
//    : m_readPath(readPath),m_writePath(writePath), m_readFileBuffCap(cap), m_writeBuffCap(cap), m_batchsize(batchsize), m_readFileBuffSz(0), m_writeBuffSz(0)
//{
//    m_readFs.open(m_readPath, std::fstream::in | std::fstream::binary);
//    if (m_readFs.is_open() == false)
//    {
//        throw std::invalid_argument("can not open file" + m_readPath);
//        return;
//    }
//
//    m_writeFs.open(m_writePath, std::fstream::out | std::fstream::binary);
//    if (m_writeFs.is_open()==false)
//    {
//        throw std::invalid_argument("can not open file" + m_readPath);
//        return;
//    }
//
//
//    m_vReadFileQueueMtx.reserve(poolCnt);
//    m_vWriteBuffQueueMtx.reserve(poolCnt);
//    
//    for (size_t i = 0; i < poolCnt; ++i)
//    {
//        m_vReadFileQueueMtx.emplace_back(new std::mutex());
//        m_vWriteBuffQueueMtx.emplace_back(new std::mutex());
//        m_vReadFileQueue.emplace_back(std::queue<CDataPkg>());
//        m_vWriteBuffQueue.emplace_back(std::queue<CDataPkg>());
//    }
//
//    // todo: move to function
//    m_writeFileIter = 0;
//    m_batchsize = batchsize;
//
//    // atomic variables
//    m_readFileIter.store(0);
//    m_readBuffIter.store(0);
//    m_readFileFinish.store(false);
//    m_writeBuffFinish.store(false);
//    m_writeFileFinish = false;
//}
//
//ReadBuff::~ReadBuff()
//{
//    if (m_readFs.is_open())
//        m_readFs.close();
//    if (m_writeFs.is_open())
//        m_writeFs.close();
//}
//
//
//// read is just support single thread
//std::streamsize ReadBuff::readfileToBuff(std::string &state)
//{
//    std::streamsize readlen = -1;
//    std::cout<<"enter readfileToBuff func " <<std::endl;
//    if (!m_readFs.is_open())
//    {
//        state = m_readPath + " is not open ";
//        throw std::runtime_error(state);
//        return readlen;
//    }
//    if (m_readFileBuffSz + m_batchsize > m_readFileBuffCap)
//    {
//        state = "out of memory catche";
//        return readlen;
//    }
//    else if (m_readFileFinish.load())
//    {
//        state = "read already finished";
//        return readlen;
//    }
//
//
//    std::cout<<"start reading " <<std::endl;
//    char *buf = new char[m_batchsize];
//    memset(buf, '\0', m_batchsize);
//    m_readFs.read(buf, m_batchsize);
//    CDataPkg datapkg(m_readBuffIter.load(), m_readFs.gcount(), buf);
//    // if reading is end
//    if (datapkg.length == 0)
//    {
//        std::cout << "---------file reading finish------m_readFileIter: " << m_readFileIter.load() << std::endl;
//        m_readFileFinish.store(true);
//        m_readFs.close();
//        return datapkg.length;
//    }
//
//
//    std::cout<<"start to store , queue size: " <<m_vReadFileQueue.size()<<std::endl;
//    std::cout << "cur read file buff size: " << m_readFileBuffSz <<" data length: "<<datapkg.length<< std::endl;
//    // search a queue to store data
//    bool store = false;
//    for (size_t i = 0; i < m_vReadFileQueue.size() && !store; ++i)
//    {
//        std::cout<<"query "<< i << " queue to store \n";
//        {
//            std::lock_guard<std::mutex> lg1(*m_vReadFileQueueMtx[i]);
//            std::lock_guard<std::mutex> lg2(m_readFileIterMapMtx);
//            m_vReadFileQueue[i].push(datapkg);
//            m_readFileBuffSz += datapkg.length;
//            m_readFileIterMap[m_readFileIter.load()] = i;
//            std::cout<<"m_readFilIter index: " << m_readFileIter.load()<<std::endl;
//            m_readFileIter.store(m_readFileIter.load() + 1);
//            store = true;
//        }
//    }
//    if (!store)
//    {
//        throw std::runtime_error("can not store data");
//    }
//
//    return datapkg.length;
//}
//
//// get nextBuff and its dataindex
//std::pair<CDataPkg, int> ReadBuff::getNextBuff(std::string &state)
//{
//    std::cout<<"get buff "<<std::endl;
//    CDataPkg datapkg;
//    int dataIndex = -1;
//
//
//    if (std::try_lock(m_readBuffIterMtx, m_readFileIterMapMtx ) == -1)
//    {
//        
//        std::lock_guard<std::mutex> lg1(m_readBuffIterMtx, std::adopt_lock);
//        std::lock_guard<std::mutex> lg2(m_readFileIterMapMtx, std::adopt_lock);
//
//        
//        std::cout << "m_readBuffIter index: " << m_readBuffIter.load() << std::endl;
//        //for (auto &e : m_readFileIterMap)
//        //{
//        //    std::cout << e.first << " " << e.second << std::endl;
//        //}
//
//
//        if (m_readFileIterMap.count(m_readBuffIter.load()) == 0)
//        {
//            state = "data has not been prepared";
//            return {datapkg, dataIndex};
//        }
//        else if (m_readBuffFinish.load())
//        {
//            state = "data has already been taken, it's empty";
//            return {datapkg, dataIndex};
//        }
//
//
//        // reading data from queue
//        size_t idx = m_readFileIterMap[m_readBuffIter.load()];
//        std::lock_guard<std::mutex> lg3(*m_vReadFileQueueMtx[idx]);
//        dataIndex = m_readBuffIter.load();
//        datapkg = m_vReadFileQueue[idx].front();
//        m_vReadFileQueue[idx].pop();
//        m_readFileBuffSz -= datapkg.length;
//        m_readBuffIter.store(dataIndex + 1);
//        if (m_readBuffIter.load() >= m_readFileIter.load() && m_readFileFinish)
//        {
//            m_readBuffFinish.store(true);
//            std::cout << "all buff has been taken \n " << std::endl;
//        }  
//    }
//    std::cout<<"get buff finished, from ------------------" << m_readBuffIter.load()-1<<std::endl;
//    return {datapkg, dataIndex};
//}
//
//// write process result to a specific wirte memory pool
//bool ReadBuff::writebuff(CDataPkg& datapkg, std::string &state, int writeQueueIndex, int dataIndex)
//{
//    std::cout<<"start to write to buff *********\n";
//    bool res = false;
//    if (datapkg.dataRef == nullptr)
//    {
//        state = "data is empty";
//        return res;
//    }
//    else if (m_writeBuffSz + datapkg.length > m_writeBuffCap)
//    {
//        state = "out of memory";
//        return res;
//    }
//    else if (m_writeBuffFinish.load())
//    {
//        state = "write buff finish";
//        return res;
//    }
//
//    std::cout<<"wait to write to buff *********\n";
//    // wait to store data
//   
//    {
//        std::cout<<"enter lock\n";
//        std::lock_guard<std::mutex> lg1(*m_vWriteBuffQueueMtx[writeQueueIndex]);
//        std::lock_guard<std::mutex> lg2(m_writeBuffMapMtx);
//        m_vWriteBuffQueue[writeQueueIndex].push(datapkg);
//        m_writeBuffSz += datapkg.length;
//        m_writeBuffMap[dataIndex] = writeQueueIndex;
//        std::cout<<"go out a lock, write data index: "<<dataIndex<<std::endl;
//    }
//    if (m_readFileFinish.load() && dataIndex == m_readFileIter.load()-1)
//    {
//        m_writeBuffFinish.store(true);
//        std::cout << "m_readBuffIter: " << m_readBuffIter.load() << " dataindex: " << dataIndex << std::endl;
//        std::cout << " %%%%%%%%%%%%%%%%%% write buff finished %%%%%%%%%%%%%%%%%%\n";
//    }
//    std::cout<<"write buff to queue ------------" << writeQueueIndex <<" queue, index: " << dataIndex<<std::endl;
//    return true;
//}
//
//std::streamsize ReadBuff::writeFileFromBuff(std::string &state)
//{
//    std::streamsize writelen = -1;
//    size_t idx = m_writeBuffMap[m_writeFileIter];
//    if (m_writeFileFinish)
//    {
//        state = "write finished";
//        return writelen;
//    }
//    else if (m_writeBuffMap.count(idx) == 0)
//    {
//        state = "data have not been processed";
//        std::cout << "writeFileIter: " << idx << std::endl;
//        return writelen;
//    }
//
//
//    CDataPkg datapkg;
//    {
//        
//        std::lock_guard<std::mutex> lg(*m_vWriteBuffQueueMtx[idx]);
//        std::lock_guard<std::mutex> lg2(m_writeBuffMapMtx);
//        if (m_vWriteBuffQueue[idx].empty())
//        {
//            state = "data not prepared";
//            return writelen;
//        }
//        std::cout<<"gou out lock, m_writeFileIter: "<<m_writeFileIter<<std::endl;
//        if (m_vWriteBuffQueue[idx].empty())
//        {
//            state = "queue is empty, error xxxxxxxxxxxxxxx";
//            return writelen;
//        }
//        datapkg = m_vWriteBuffQueue[idx].front();
//        m_vWriteBuffQueue[idx].pop();
//        m_writeFileIter++;
//        m_writeBuffSz -= datapkg.length; 
//    }
//
//    // write data
//    if (datapkg.dataRef)
//    {
//        writelen = datapkg.length;
//        m_writeFs.write(datapkg.dataRef.get(), datapkg.length);
//        std::cout<<"write len: "<<writelen<<std::endl;
//    }
//
//    // check wheter is finish
//    if (m_writeBuffFinish.load() &&  m_writeFileIter == m_readFileIter.load())
//    {
//        m_writeFs.close();
//        m_writeFileFinish = true;
//        std::cout<<"writting file finish----------------------\n";
//    }
//    
//    return writelen;
//}
