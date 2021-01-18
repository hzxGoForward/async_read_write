#include "boost_read_hzx.h"
#include "sync_queue.h"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/windows/random_access_handle.hpp>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <windows.h>

typedef void(
    haldel_type)(CSycnQueue_ptr_t &buff, const CDataPkg_ptr_t &datapkg, const boost::system::error_code &e, int read);

int64_t async_read_file(const std::string &filepath, CSycnQueue_ptr_t &buff)
{
    auto start = std::chrono::steady_clock::now();
    HANDLE hfile =
        ::CreateFile(filepath.data(), GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        throw std::invalid_argument("ERROR_FILE_NOT_FOUND " + filepath);
        return -1;
    }

    boost::asio::io_context io;
    boost::asio::windows::random_access_handle rad(io, hfile);


    uint64_t pos = 0;
    uint64_t read_len = 0;
    uint64_t offset = 0;
    uint64_t file_len = ::GetFileSize(hfile, nullptr);
    const uint64_t len = 256;

    std::function<haldel_type> handle_fun = [&](CSycnQueue_ptr_t &buff, const CDataPkg_ptr_t &datapkg,
                                                const boost::system::error_code &e, int read) {
        read_len += read;
        datapkg->length = read;
        while (buff->push(datapkg, datapkg->length) == false)
        {
            std::unique_lock<std::mutex> lg(buff->mutex());
            buff->pop_cond().wait(lg, [&] { return buff->full() == false; });
        }
        // std::cout << datapkg->pos << " successfully inserted into queue\n ";
    };

    try
    {
        while (offset < file_len)
        {
            CDataPkg_ptr_t datapkg = std::make_shared<CDataPkg>(pos++, len);
            boost::asio::mutable_buffer dataBuff(static_cast<void *>(datapkg->data.get()), datapkg->length);
            boost::asio::async_read_at(rad, offset, dataBuff,
                std::bind(handle_fun, buff, datapkg, std::placeholders::_1, std::placeholders::_2));
            offset += len;
        }
    }
    catch (...)
    {
        std::cout << "error" << std::endl;
        return -1;
    }
    io.run();

    // write finish
    buff->writeEnd();
    auto end = std::chrono::steady_clock::now();
    std::cout << "total write " << file_len << " Bytes " << std::endl;
    return (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count();
}

int64_t process(CSycnQueue_ptr_t fromBuff, CSycnQueue_ptr_t &toBuff)
{
    auto start = std::chrono::steady_clock::now();
    int lastProcess = 1;
    int processLen = 0;

    while (!(fromBuff->is_end() && fromBuff->empty()))
    {
        std::shared_ptr<CDataPkg> datapkgRef = nullptr;
        while (!fromBuff->pop(datapkgRef))
        {
            std::unique_lock<std::mutex> ul(fromBuff->mutex());
            fromBuff->push_cond().wait(ul, [&]() { return !fromBuff->empty()||fromBuff->is_end(); });
            if (fromBuff->is_end())
                break;
        }

        if (datapkgRef)
        {
            while (toBuff->push(datapkgRef, datapkgRef->length) == false)
            {
                std::unique_lock<std::mutex> ul(toBuff->mutex());
                toBuff->pop_cond().wait(ul, [&]() { return toBuff->full() == false; });
            }
            processLen += datapkgRef->length;
        }
    } 
    toBuff->writeEnd();
    std::cout << " process file finished, total process " << processLen
              << " Bytes, thread id: " << std::this_thread::get_id() << std::endl;
    auto end = std::chrono::steady_clock::now();
    return (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count();
}

int64_t writeFile(const std::string &filepath, std::vector<CSycnQueue_ptr_t> &vFromBuff)
{
    HANDLE hfile =
        ::CreateFile(filepath.data(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        throw std::invalid_argument("ERROR_FILE_NOT_FOUND " + filepath);
        return -1;
    }

    auto start = std::chrono::steady_clock::now();
    boost::asio::io_context io;
    boost::asio::windows::random_access_handle rad(io, hfile);

    uint64_t nextpos = 0;
    int writeLen = 0;
    int lastwrite = 1;
    int offset = 0;
    size_t endCnt = 0;
    auto f = [](const boost::system::error_code &e, int read) { std::cout << "write " << read << std::endl; };
    while (!(lastwrite == 0 && endCnt == vFromBuff.size()))
    {
        lastwrite = 0;
        endCnt = 0;
        for (CSycnQueue_ptr_t buff : vFromBuff)
        {
            std::shared_ptr<CDataPkg> datapkgRef = nullptr;
            if (buff->frontPos() == nextpos && buff->pop(datapkgRef))
            {
                boost::asio::mutable_buffer dataBuff(static_cast<void *>(datapkgRef->data.get()), datapkgRef->length);
                boost::asio::async_write_at(rad, offset, dataBuff, f);
                offset += datapkgRef->length;
                lastwrite = datapkgRef->length;
                writeLen += lastwrite;
                ++nextpos;
            }
            if (buff->is_end()&&buff->empty())
                endCnt++;
        }
    }
    io.run();
    std::cout << "write file finished, total write " << writeLen << " Bytes\n";
    auto end = std::chrono::steady_clock::now();
    return (std::chrono::duration_cast<std::chrono::seconds>(end - start)).count();
}


void rpw_test()
{
    std::string readPath = "";
    readPath = "C:\\Users\\t4641\\Desktop\\性能测试\\training.processed.noemoticon.csv_1024.runtime";
    // std::cin >> readPath;
    CSycnQueue_ptr_t fromBuff = std::make_shared<CSyncQueue>(4096000);
    async_read_file(readPath, fromBuff);
    
    std::vector<std::future<int64_t> > vf;
    std::vector<std::shared_ptr<CSyncQueue> > buffQueue;
    size_t processCnt = std::thread::hardware_concurrency() > 2 ? (std::thread::hardware_concurrency() - 2) : 1;
    processCnt = 1;
     for (size_t i = 0; i < processCnt; ++i)
    {
        buffQueue.emplace_back(new CSyncQueue(409600));
        vf.emplace_back(std::async(std::launch::async, process, fromBuff, buffQueue[i]));
    }

     for (auto &f : vf)
        f.wait();
     std::string writePath = readPath + ".copy";
     auto wf = std::async(std::launch::async, writeFile, writePath, buffQueue);
     wf.wait();
}
