#include "asio_read.h"
#include <fstream>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

int64_t asio_read::get_file_size()
{
    std::ifstream ifs(m_file_name.data(), std::ios::in);
    if (!ifs)
        throw std::invalid_argument("can not open file: " + m_file_name);
    ifs.seekg(0, ifs.end);
    int64_t size = ifs.tellg();
    ifs.close();
    return size;
}

void asio_read::read_handler(const CDataPkg_ptr_t datapkg, const boost::system::error_code &e, size_t read)
{
    if (datapkg && read > 0)
    {
        m_read_buff->push(datapkg);
        datapkg->length = read;
    }
    m_read_size += read;
    std::cout << "read " << m_read_size << " bytes\n";

    if (m_read_size < m_file_size)
    {
        // mutable_buffer 和 boost::asio::buffer的用法造成指针分配和析构时出现异常
        CDataPkg_ptr_t datapkgRef = std::make_shared<CDataPkg>(m_pos++, 256);
        boost::asio::mutable_buffer dataBuff(static_cast<void *>(datapkgRef->data.get()), 256);
#ifndef WIN32
        boost::asio::async_read(*m_stream_ptr, dataBuff,
            std::bind(&asio_read::read_handler, this, datapkgRef, std::placeholders::_1, std::placeholders::_2));
#endif

#ifdef WIN32
        try
        {
            boost::asio::async_read_at(*m_stream_ptr, m_read_size, dataBuff,
                std::bind(&asio_read::read_handler, this, datapkgRef, std::placeholders::_1, std::placeholders::_2));
        }
        catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
#endif
    }
    else
    {
        std::cout << "total read " << m_read_size << " bytes\n";
    }
}

// read file data to buff
std::pair<int64_t, int64_t> asio_read::async_read(CThreadsafeQueue_ptr buff)
{
    auto start = std::chrono::steady_clock::now();
    m_file_size = get_file_size();
    m_read_buff = buff;
    std::cout << "start to read ...\n";
#ifndef WIN32
    int fm = open(m_file_name.data(), O_RDONLY);
    m_stream_ptr = std::make_shared<boost::asio::posix::stream_descriptor>(m_ios, fm);

#endif

#ifdef WIN32
    HANDLE fm = ::CreateFile(
        m_file_name.data(), GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    m_stream_ptr = std::make_shared<boost::asio::windows::random_access_handle>(m_ios, fm);
#endif

    read_handler(nullptr, boost::system::error_code(), 0);
    m_ios.run();
#ifdef WIN32
    CloseHandle(fm);
#endif

#ifndef WIN32
    close(fm);
#endif


    std::cout << "total read:" << m_read_size << " Bytes\n";
    std::cout << "file size: " << m_file_size << " Bytes\n";
    auto end = std::chrono::steady_clock::now();
    return {(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count(), m_read_size};
}


void asio_read::write_handler(const boost::system::error_code &e, size_t write)
{
    if (write > 0)
        std::cout << "write " << write << std::endl;
}


std::pair<int64_t, int64_t> asio_read::async_write(const std::string &filepath,
    std::vector<CThreadsafeQueue_ptr> &vFromBuff)
{
    auto start = std::chrono::steady_clock::now();
    std::remove(filepath.data());

#ifndef WIN32
    int fm = open(m_file_name.data(), O_WriteOnly);
    m_stream_ptr = std::make_shared<boost::asio::posix::stream_descriptor>(m_ios, fm);
#endif

#ifdef WIN32
    HANDLE fm = ::CreateFile(
        filepath.data(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    m_stream_ptr = std::make_shared<boost::asio::windows::random_access_handle>(m_ios, fm);
#endif

    m_write_size = 0;
    int64_t nextpos = 0;

    size_t endCnt = 0;
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
#ifdef WIN32
                boost::asio::async_write_at(*m_stream_ptr, m_write_size, dataBuff,
                    std::bind(&asio_read::write_handler, this, std::placeholders::_1, std::placeholders::_2));
#endif

#ifndef WIN32
                boost::asio::async_write(*m_stream_ptr, dataBuff,
                    std::bind(&asio_read::write_handler, this, std::placeholders::_1, std::placeholders::_2));
#endif
                m_write_size += datapkgRef->length;
                ++nextpos;
            }
        }
    }

    m_ios.run();
#ifdef WIN32
    CloseHandle(fm);
#endif

#ifndef WIN32
    close(fm);
#endif

    std::cout << "write file finished, total write " << m_write_size << " Bytes\n";
    auto end = std::chrono::steady_clock::now();
    return {(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)).count(), m_write_size};
}
