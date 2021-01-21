#include "boost_read_hzx.h"
#include "threadSafeQueue.h"
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

typedef void(haldel_type)(const CDataPkg_ptr_t &datapkg, const boost::system::error_code &e, int read);

std::pair<int64_t, int64_t> async_read_file(const std::string &filepath, CThreadsafeQueue_ptr &buff)
{
    auto start = std::chrono::steady_clock::now();
    HANDLE hfile =
        ::CreateFile(filepath.data(), GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        throw std::invalid_argument("ERROR_FILE_NOT_FOUND " + filepath);
        return {0, 0};
    }

    boost::asio::io_context io;
    boost::asio::windows::random_access_handle rad(io, hfile);


    int64_t pos = 0;
    int64_t offset = 0;
    int64_t file_len = ::GetFileSize(hfile, nullptr);
    const int64_t len = 512;
    int read_len = 0;

    std::function<haldel_type> handle_fun = [&](const CDataPkg_ptr_t &datapkg, const boost::system::error_code &e,
                                                int read) -> void {
        datapkg->ready = true;
        datapkg->length = read;
        buff->push(datapkg);
        read_len += read;
    };

    try
    {
        while (offset < file_len)
        {
            CDataPkg_ptr_t datapkgRef = std::make_shared<CDataPkg>(pos++, len);
            // buff->push(datapkgRef);
            boost::asio::mutable_buffer dataBuff(static_cast<void *>(datapkgRef->data.get()), datapkgRef->length);
            boost::asio::async_read_at(
                rad, offset, dataBuff, std::bind(handle_fun, datapkgRef, std::placeholders::_1, std::placeholders::_2));
            offset += len;
            // boost::asio::ip::tcp::socket;
            // boost::asio::async_read()
        }
    }
    catch (...)
    {
        std::cout << "error" << std::endl;
    }
    io.run();

    // write finish
    buff->set_end();
    auto end = std::chrono::steady_clock::now();
    std::cout << "total read " << read_len << " Bytes " << std::endl;
    std::cout << "file length " << file_len << " Bytes " << std::endl;
    return {(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count(), read_len};
}


std::pair<int64_t, int64_t> process(CThreadsafeQueue_ptr &fromBuff, CThreadsafeQueue_ptr &toBuff)
{
    auto start = std::chrono::steady_clock::now();
    int64_t process_len = 0;
    while (!(fromBuff->empty() && fromBuff->is_end()))
    {
        std::shared_ptr<CDataPkg> datapkgRef = nullptr;
        if (fromBuff->pop(datapkgRef) && toBuff->push(datapkgRef))
        {
            process_len += datapkgRef->length;
            if (process_len > 0 && process_len % (1024*100) == 0)
                std::cout << "process len: -----" << process_len << " thread id : " << std::this_thread::get_id()
                          << std::endl;
        }
    }
    toBuff->set_end();
    std::cout << "process finished, total process " << process_len
              << " Bytes, thread id: " << std::this_thread::get_id() << std::endl;
    auto end = std::chrono::steady_clock::now();
    return {(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count(), process_len};
}


std::pair<int64_t, int64_t> writeFile(const std::string &filepath, std::vector<CThreadsafeQueue_ptr> &vFromBuff)
{
    auto start = std::chrono::steady_clock::now();

    HANDLE hfile = ::CreateFile(
        filepath.data(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        throw std::invalid_argument("ERROR_FILE_NOT_FOUND " + filepath);
    }


    boost::asio::io_context io;
    boost::asio::windows::random_access_handle rad(io, hfile);
    int64_t nextpos = 0;
    int64_t write_len = 0;
    auto handle_fun = [&](const boost::system::error_code &e, std::size_t write) -> void { 
        write_len += write; 
        if (write_len > 0 && write_len % (1024 * 100) == 0)
            std::cout << "write len: -----" << write_len << " thread id : " << std::this_thread::get_id()<< std::endl;
    
    };


    size_t endCnt = 0;
    uint64_t offset = 0;
    std::cout << "start writting -----" << std::endl;
    while (endCnt != vFromBuff.size())
    {
        endCnt = 0;
        for (CThreadsafeQueue_ptr buff : vFromBuff)
        {
            if (buff->empty())
            {
                if (buff->is_end())
                    endCnt++;
                continue;
            }
            std::shared_ptr<CDataPkg> datapkgRef = nullptr;
            if (buff->front(datapkgRef) && datapkgRef->pos == nextpos && buff->pop(datapkgRef))
            {
                boost::asio::mutable_buffer dataBuff(static_cast<void *>(datapkgRef->data.get()), datapkgRef->length);
                boost::asio::async_write_at(rad, offset, dataBuff, handle_fun);
                offset += datapkgRef->length;
                ++nextpos;
            }
        }
    }
    io.run();
    std::cout << "write file finished, total write " << write_len << " Bytes\n";
    auto end = std::chrono::steady_clock::now();
    return {(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count(), write_len};
}

void rpw_test()
{
    // read file
    std::string readPath = "";
    readPath = "C:\\Users\\t4641\\Desktop\\性能测试\\training.processed.noemoticon.csv";
    CThreadsafeQueue_ptr fromBuff = std::make_shared<CThreadSafeQueue<CDataPkg_ptr_t> >(10);
    auto rf = std::async(std::launch::async, async_read_file, readPath, fromBuff);


    // process file
    std::vector<std::future<std::pair<int64_t, int64_t> > > vf;
    std::vector<std::shared_ptr<CThreadSafeQueue<CDataPkg_ptr_t> > > buffQueue;
    size_t processCnt = std::thread::hardware_concurrency() > 2 ? (std::thread::hardware_concurrency() - 2) : 1;
    std::cout << processCnt << " process thread\n";
    // processCnt = 2;
    for (size_t i = 0; i < processCnt; ++i)
    {
        buffQueue.emplace_back(new CThreadSafeQueue<CDataPkg_ptr_t>(10));
        vf.emplace_back(std::async(std::launch::async, process, fromBuff, buffQueue[i]));
    }


    // write file
    std::string writePath = readPath + ".copy";
    auto wf = std::async(std::launch::async, writeFile, writePath, buffQueue);
    


    // statics
    int64_t process_sum = 0;
    for (auto &f : vf)
    {
        auto f1 = f.get();
        std::cout << "process " << f1.second << " Bytes using " << f1.first << " millseconds \n";
        process_sum += f1.second;
    }
    std::cout << "total read     " << rf.get().second << " Bytes\n";
    std::cout << "total process  " << process_sum << " Bytes\n";
   
    auto wfRes = wf.get();
    std::cout << "total write " << wfRes.second << " Bytes, using " << wfRes.first << " millseconds \n";
}
