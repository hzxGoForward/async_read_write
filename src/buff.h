#pragma once
#include <queue>
#include <fstream>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <unordered_map>

/*
1. Only one thread reads data from file and store to the first memory pools
2. several threads taken data X from the first memory pools, process X get Y, and store Yto the second memory pools
3. Only one thread taken Y from the second memory pools and write to the a new file
*/

typedef std::shared_ptr<char> dataRef;
typedef const std::string CS;
class ReadBuff
{
public:
    ReadBuff(CS& readPath, CS& writePath, const std::streamsize cap, const size_t poolCnt, const size_t batchsize = 512);
    ~ReadBuff();
    bool is_open() const;
    bool is_readfinish() const;
    bool is_writefinish() const;
    std::streamsize readfile(std::string& state, const std::streamsize len);
    std::streamsize writefile(std::string& state);
    dataRef getNextBuff(std::string& state);
    bool writebuff(dataRef buf, std::string& state);

private:
    // read file path and write file path

    std::size_t m_batchsize;

    // variables for reading from file and store to memory pool
    std::string m_readPath;
    std::streamsize m_readBuffCap, m_readBuffSz; // Bytes
    std::ifstream m_readFs;

    std::atomic<bool> m_readfinish;

    // memory cache for reading and taken
    std::vector<std::mutex> m_vReadBuffQueueMtx;
    std::vector<std::queue<dataRef>> m_vReadBuffQueue;
    std::atomic<int> m_readBuffIter;
    std::unordered_map<size_t, size_t> m_readBuffIterMap; // which queue is next buff to process

    // get data from buff and process
    std::mutex m_processMtx;
    int m_processIter;
    std::atomic<bool> m_processfinish;

    // write processed buff to memory pool
    std::streamsize m_writeBuffCap, m_writeBuffSz;
    std::atomic<bool> m_writeBuffFinish;
    std::vector<std::mutex> m_vWriteBuffMtx;
    std::vector<std::queue<dataRef>> m_vWriteBuffQueue;
    std::atomic<int> m_writeBuffIter;         // next buff to write
    std::unordered_map<size_t, size_t> m_writeMap; // which queue is next buff to write

    // write buff to file
    std::string m_writePath;
    std::ofstream m_writeFs;
    int m_writeFileIter; // always point to next queue to write
    bool m_writefinish;
};
