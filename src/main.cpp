#include "buff.h"
#include <future>
#include <iostream>
#include <string>
#include <thread>

std::thread::id readFunc(ReadBuff &rb)
{
    std::cout << "thread readFunc runs, id: " << std::this_thread::get_id() << std::endl;

    while (rb.is_readFileFinish() == false)
    {
        std::string state = "";
        rb.readfileToBuff(state, 256);
        std::cout << state << std::endl;
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::this_thread::get_id();
}

std::thread::id processData(ReadBuff &rb, int queueIndex)
{
    std::cout << "thread processData runs, id: " << std::this_thread::get_id() << std::endl;
    while (!rb.is_readBuffFinish())
    {
        std::string state = "";
        auto res = rb.getNextBuff(state);
        if (res.first != nullptr && res.second != -1)
            rb.writebuff(res.first, state, queueIndex, res.second);
        else
        {
            std::cout << "write buff fail: "<< state << std::endl;
        }
    }
    return std::this_thread::get_id();
}


std::thread::id writeFunc(ReadBuff &rb)
{
    std::cout << "thread writeFunc runs, id: " << std::this_thread::get_id() << std::endl;
    while (!rb.is_writeFileFinish())
    {
        std::string state = "";
        rb.writeFileFromBuff(state);
		std::cout<<state<<std::endl;
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::this_thread::get_id();
}


int main()
{
    std::cout << "hello world\n";
    std::string readPath = "/home/hzx/Desktop/clang-format";
    std::string writePath = "/home/hzx/Desktop/clang_copy-format.txt";
    ReadBuff rb(readPath, writePath, 4096, 1, 512);
    // readFunc(rb);
	// processData(rb, 0);
	// writeFunc(rb);
    // for(int i = 0; i < 8; ++i){
	// 	std::string state = "";
    // 	auto r1 = rb.getNextBuff(state);
    	
	// 	rb.writebuff(r1.first, state, 0, r1.second);
	// 	std::cout << state << std::endl;
	// 	std::cout<< r1.first<< std::endl;
	// }
	
    
    // auto r2 = rb.getNextBuff(state);
    // std::cout << state << r2.first<< std::endl;
    
    // auto r3 = rb.getNextBuff(state);
    // std::cout << state << r3.first<< std::endl;
    // processData(rb, 0);
    // writeFunc(rb);

    std::future<std::thread::id> f1 = std::async(std::launch::async, readFunc, std::ref(rb));
    std::future<std::thread::id> f2 = std::async(std::launch::async, processData, std::ref(rb), 0);
    std::future<std::thread::id> f3 = std::async(std::launch::async, writeFunc, std::ref(rb));


    return 0;
}
