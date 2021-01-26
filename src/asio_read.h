#pragma once
#ifndef ASIO_READ_H
#define ASIO_READ_H
#include "threadSafeQueue.h"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#ifndef WIN32
#include <boost/asio/posix/stream_descriptor.hpp>
#endif

#ifdef WIN32
#include <boost/asio/windows/random_access_handle.hpp>
#endif

#include <boost/system/error_code.hpp>
#include <iostream>
#include <memory>
#include <string>

class asio_read
{
public:
    asio_read(const std::string &file_name_) : m_file_name(file_name_), m_ios(), m_stream_ptr(nullptr) {}
    asio_read(const char *file_name_) : m_file_name(file_name_), m_ios(), m_stream_ptr(nullptr) {}
    ~asio_read() { 
        // std::cout << "deconstruct asio_read object\n"; 
    }

private:
    const std::string m_file_name;
#ifndef WIN32
    std::shared_ptr<boost::asio::posix::stream_descriptor> m_stream_ptr;
#endif
#ifdef WIN32
    std::shared_ptr<boost::asio::windows::random_access_handle> m_stream_ptr;
#endif
    boost::asio::io_service m_ios;
    int64_t m_read_size = 0;
    int64_t m_write_size = 0;
    int64_t m_total_read = 0;
    int64_t m_file_size = 0;
    int64_t m_pos = 0;
    CThreadsafeQueue_ptr m_read_buff;

public:
    std::pair<int64_t, int64_t> async_read(CThreadsafeQueue_ptr buff);
    std::pair<int64_t, int64_t> async_write(const std::string &filepath, std::vector<CThreadsafeQueue_ptr> &vFromBuff);


private:
    int64_t get_file_size();
    void read_handler(const CDataPkg_ptr_t datapkg, const boost::system::error_code &e, size_t read);
    void write_handler(const boost::system::error_code &e, size_t read);
};


#endif
