//#pragma once
//#include <atomic>
//#include <fstream>
//#include <functional>
//#include <memory>
//#include <mutex>
//#include <queue>
//#include <unordered_map>
//
///*
//1. Only one thread reads data from file and store to the first memory pools
//2. several threads taken data X from the first memory pools, process X get Y, and store Yto the second memory pools
//3. Only one thread taken Y from the second memory pools and write to the a new file
//*/
//
//typedef std::shared_ptr<char> SPChar;
//typedef std::shared_ptr<std::mutex> mutexRef;
//typedef const std::string CS;
//
//
//
//struct CDataPkg
//{
//    int pos;
//    std::streamsize length;
//    SPChar dataRef;
//    CDataPkg(int a = -1, std::streamsize sz = -1, char *s = nullptr) : pos(a), length(sz) { 
//        dataRef.reset(s);
//    }
//    
//    //DataPkg operator=(const DataPkg &d) { pos = d.pos;
//    //    length = d.length;
//    //    dataRef = d.dataRef;
//    //}
//};
//
//template <typename T>
//std::shared_ptr<T> make_shared_array(size_t size)
//{
//    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
//}
//
//class ReadBuff
//{
//public:
//    ReadBuff(CS &readPath,
//        CS &writePath,
//        const std::streamsize cap,
//        const size_t poolCnt,
//        const size_t batchsize);
//    ~ReadBuff();
//    bool is_open() const { return m_readFs.is_open(); }
//
//    bool is_readFileFinish() const { return m_readFileFinish.load(); }
//
//    bool is_writeFileFinish() const { return m_writeFileFinish; }
//
//    bool is_readBuffFinish() const { return m_readBuffFinish.load(); }
//
//    bool is_writeBuffFinish() const { return m_writeBuffFinish.load(); }
//
//    std::streamsize readfileToBuff(std::string &state);
//    std::streamsize writeFileFromBuff(std::string &state);
//    std::pair<CDataPkg, int> getNextBuff(std::string &state);
//    bool writebuff(CDataPkg &datapkg, std::string &state, int writeQueueIndex, int dataIndex);
//
//private:
//    // read file path and write file path
//    std::size_t m_batchsize;
//
//    // variables for reading from file and store to memory pool
//    std::string m_readPath;
//    std::streamsize m_readFileBuffCap, m_readFileBuffSz; // Bytes
//    std::ifstream m_readFs;
//    std::atomic<bool> m_readFileFinish;
//    std::atomic<int> m_readFileIter;
//    
//    // 
//    std::mutex m_readFileIterMapMtx;
//    std::unordered_map<int, int> m_readFileIterMap; // which queue is next buff to process
//    
//    
//    std::vector<mutexRef> m_vReadFileQueueMtx;
//    std::vector<std::queue<CDataPkg> > m_vReadFileQueue;
//    
//    // memory cache for reading from buff and store to processed memory pool
//    std::mutex m_readBuffIterMtx;
//    std::atomic<int> m_readBuffIter;
//    std::atomic<bool> m_readBuffFinish;
//
//
//    // write processed buff to memory pool
//    std::streamsize m_writeBuffCap, m_writeBuffSz;
//    std::atomic<bool> m_writeBuffFinish;
//    std::vector<mutexRef> m_vWriteBuffQueueMtx;
//    std::vector<std::queue<CDataPkg> > m_vWriteBuffQueue;
//    std::mutex m_writeBuffMapMtx;
//    std::unordered_map<int, int> m_writeBuffMap; // which queue is next buff to write
//
//    // write buff to file
//    std::string m_writePath;
//    std::ofstream m_writeFs;
//    int m_writeFileIter; // always point to next queue to write
//    bool m_writeFileFinish;
//};
