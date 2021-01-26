#include "boost_read_hzx.h"
#include <error.h>
#include <fcntl.h>
#include <future>
#include <iostream>
#include <sys/stat.h>
#include <thread>

int createFile(const std::string fname)
{
    auto res = open(fname.data(), O_WRONLY | O_CREAT, S_IRUSR);
    std::cout << res << std::endl;
    return res;
}

int main()
{
    try
    {
        rpw_test();
    }
    catch (std::exception &e)
    {
        std::cout << "find a exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "unknown exception" << std::endl;
    }

    // auto res = std::async(std::launch::async, createFile, "/home/zxhu/gitLab/async_read_write/CMakeLists.txt.copy2");
    // std::cout<<res.get()<<std::endl;

    return 0;
}
