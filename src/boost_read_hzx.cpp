#include "boost_read_hzx.h"
#include "asio_read.h"
#include "threadSafeQueue.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

std::pair<int64_t, int64_t> async_read_file(const std::string &filepath, CThreadsafeQueue_ptr buff)
{
    asio_read af(filepath);
    auto res = af.async_read(buff);
    buff->set_end();

    std::cout << "total read " << res.second << " Bytes " << std::endl;
    std::cout << "using " << res.first << "mill sec " << std::endl;
    return res;
}


std::pair<int64_t, int64_t> process(CThreadsafeQueue_ptr fromBuff, CThreadsafeQueue_ptr toBuff)
{
    auto start = std::chrono::steady_clock::now();
    int64_t process_len = 0;
    while (!(fromBuff->empty() && fromBuff->is_end()))
    {
        std::shared_ptr<CDataPkg> datapkgRef = nullptr;
        if (fromBuff->pop(datapkgRef) && toBuff->push(datapkgRef))
        {
            process_len += datapkgRef->length;
            if (process_len > 0 && process_len % (1024 * 100) == 0)
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
    std::remove(filepath.data());

    asio_read af("");
    auto res = af.async_write(filepath, vFromBuff);

    std::cout << "total write " << res.second << " Bytes " << std::endl;
    std::cout << "using " << res.first << "mill sec " << std::endl;
    return res;
}

void rpw_test()
{
    // read file
    std::string readPath = "";
    readPath = "C:\\Users\\t4641\\Desktop\\性能测试\\recordings-overview.csv_1024.runtime";
    CThreadsafeQueue_ptr fromBuff = std::make_shared<CThreadSafeQueue<CDataPkg_ptr_t> >(10);
    // c++ 线程对象默认使用拷贝构造函数，因此使用std::ref 和std::cref告知线程使用的是引用
    auto rf = std::async(std::launch::async, async_read_file, std::cref(readPath), fromBuff);

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
    auto wf = std::async(std::launch::async, writeFile, std::cref(writePath), std::ref(buffQueue));


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
