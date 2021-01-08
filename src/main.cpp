#include "sync_queue.h"
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <functional>


template <typename T>
std::shared_ptr<T> make_shared_array(size_t size)
{
    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
}


int64_t readFile(const std::string &filepath, QueRef buff)
{
    auto start = std::chrono::steady_clock::now();
    std::fstream fs(filepath, std::fstream::in | std::fstream::binary);
    if (fs.is_open() == false)
        throw std::invalid_argument("can not open " + filepath);

    uint64_t pos = 0;
    int readLen = 0;
    while (fs)
    {
        int len = 512;
        char *buf = new char[len]{0};
        // memset(char, 0, len);
        fs.read(buf, len);
        len = fs.gcount();
        std::shared_ptr<CDataPkg> datapkgRef = nullptr;
        if (len > 0)
            datapkgRef = std::make_shared<CDataPkg>(pos++, len, buf);
        while (!buff->push(datapkgRef, len))
            std::this_thread::yield();
        std::cout << "write len: -----" << datapkgRef->length << std::endl;
        readLen += len;
    }
    buff->writeEnd();
    fs.close();
    std::cout << "read file finished, total read " << readLen << "Bytes\n";
    auto end = std::chrono::steady_clock::now();
    return (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count();
}

int64_t writeFile(const std::string &filepath, std::vector<QueRef> vFromBuff)
{
    auto start = std::chrono::steady_clock::now();
    std::fstream fs(filepath, std::fstream::out | std::fstream::binary);
    if (fs.is_open() == false)
        throw std::invalid_argument("can not open " + filepath);

    uint64_t nextpos = 0;
    int writeLen = 0;
    int lastwrite = 1;
    size_t endCnt = 0;
    while (!(lastwrite == 0 && endCnt == vFromBuff.size()))
    {
        lastwrite = 0;
        endCnt = 0;
        for (QueRef buff : vFromBuff)
        {
            std::shared_ptr<CDataPkg> datapkgRef = nullptr;
            if (buff->frontPos() == nextpos && buff->pop(datapkgRef))
            {
                fs.write(datapkgRef->data.get(), datapkgRef->length);
                std::cout << "write len: -----" << datapkgRef->length << std::endl;
                lastwrite = datapkgRef->length;
                writeLen += lastwrite;
                ++nextpos;
            }
            if (buff->is_end())
                endCnt++;
        }
    }
    fs.close();
    std::cout << "write file finished, total write " << writeLen << " Bytes\n";
    auto end = std::chrono::steady_clock::now();
    return (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count();
}


int64_t process(QueRef fromBuff, QueRef toBuff)
{
    auto start = std::chrono::steady_clock::now();
    int lastProcess = 1;
    int processLen = 0;
    while (!(lastProcess == 0 && fromBuff->is_end()))
    {
        lastProcess = 0;
        std::shared_ptr<CDataPkg> datapkgRef = nullptr;
        if (fromBuff->pop(datapkgRef))
        {
            // todo process
            lastProcess = datapkgRef->length;
            while (toBuff->push(datapkgRef, datapkgRef->length) == false)
            {
                std::this_thread::yield();
            }

            std::cout << "process len: -----" << datapkgRef->length << std::endl;
            lastProcess = datapkgRef->length;
            processLen += lastProcess;
        }
    }
    toBuff->writeEnd();
    std::cout << " process file finished, total process " << processLen
              << "Bytes, thread id: " << std::this_thread::get_id() << std::endl;
    auto end = std::chrono::steady_clock::now();
    return (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count();
}


int main()
{
    std::cout << "hello world, input data path: \n";

    std::string readPath = "";
    // readPath = "C:\\Users\\t4641\\Desktop\\性能测试\\recordings-overview.csv_512.runtime";
    std::cin >> readPath;
    std::string writePath = readPath + ".copy";
    QueRef fromBuff = std::make_shared<CSyncQueue>(4096000);
    std::vector<std::shared_ptr<CSyncQueue> > buffQueue;
    std::future<int64_t> f1 = std::async(std::launch::async, readFile, readPath, fromBuff);
    std::vector<std::future<int64_t> > vf;
    size_t processCnt = std::thread::hardware_concurrency() > 2 ? (std::thread::hardware_concurrency()-2):1;
    for (size_t i = 0; i < processCnt; ++i)
    {
        buffQueue.emplace_back(new CSyncQueue(409600));
        vf.emplace_back(std::async(std::launch::async, process, fromBuff, buffQueue[i]));
    }
    std::future<int64_t> f2 = std::async(std::launch::async, writeFile, writePath, buffQueue);
    // f1.get();
    //f2.get();
    //for (auto &f : vf)
    //    f.get();

    std::cout << "read file use " << f1.get() << "seconds\n";
    std::cout << "write file use " << f2.get() << "seconds\n";
    for (size_t i = 0; i < vf.size(); ++i)
        std::cout << "process " << i << " use " << vf[i].get() << "sec\n";

}
