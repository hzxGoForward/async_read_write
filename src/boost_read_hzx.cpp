#include "boost_read_hzx.h"
#include "asio_read.h"
#include "threadSafeQueue.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

std::pair<int64_t, int64_t> async_read_file(const std::string &filepath, CThreadsafeQueue_ptr buff)
{
    asio_read af(filepath);
    auto res = af.async_read(buff);
    buff->set_end();

    /*std::cout << "total read " << res.second << " Bytes " << std::endl;
    std::cout << "using " << res.first << "mill sec " << std::endl;*/
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
            if (process_len > 0 && process_len % (1024 * 1000) == 0)
                std::cout << "process len: -----" << process_len << " thread id : " << std::this_thread::get_id()
                          << std::endl;
        }
    }
    toBuff->set_end();
    /*std::cout << "process finished, total process " << process_len
              << " Bytes, thread id: " << std::this_thread::get_id() << std::endl;*/
    auto end = std::chrono::steady_clock::now();
    return {(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count(), process_len};
}


std::pair<int64_t, int64_t> writeFile(const std::string &filepath, std::vector<CThreadsafeQueue_ptr> &vFromBuff)
{
    auto start = std::chrono::steady_clock::now();

    asio_read af("");
    auto res = af.async_write(filepath, vFromBuff);

    /*std::cout << "total write " << res.second << " Bytes " << std::endl;
    std::cout << "using " << res.first << "millseconds" << std::endl;*/
    return res;
}

void rpw_test()
{
    std::ios::sync_with_stdio(false);
    // read file
    std::string readPath = "";

#ifdef WIN32
    readPath = "C:\\Users\\t4641\\Desktop\\dataset\\training.processed.noemoticon.csv_512.runtime";
#endif

#ifndef WIN32
    readPath = "/home/zxhu/gitLab/dataset/testdata.manual.2009.06.14.csv";
#endif
    std::cout << "please input data path: \n";
    std::cin >> readPath;
    std::cout << "path: " << readPath << std::endl;
    auto start = std::chrono::steady_clock::now();

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
    // std::string writePath = "copy";
    auto wf = std::async(std::launch::async, writeFile, std::cref(writePath), std::ref(buffQueue));

    // statics
    // wait write finish
    auto wfRes = wf.get();
    std::cout << "--------total write " << wfRes.second << " Bytes, using " << wfRes.first << " millseconds \n";

    // wait process finish
    int64_t process_sum = 0;
    for (auto &f : vf)
    {
        auto f1 = f.get();
        std::cout << "---------process " << f1.second << " Bytes using " << f1.first << " millseconds \n";
        process_sum += f1.second;
    }

    // wait read finish
    std::cout << "-----------total read     " << rf.get().second << " Bytes\n";
    std::cout << "-----------total process  " << process_sum << " Bytes\n";

    auto end = std::chrono::steady_clock::now();
    std::cout << "--------read+process+write using "
              << (std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count() << " millseconds"
              << std::endl;
}
