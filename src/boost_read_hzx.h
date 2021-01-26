#pragma once
#ifndef _BOOST_READ_HZX_H
#define _BOOST_READ_HZX_H

#include "threadSafeQueue.h"
#include <boost/asio.hpp>
#include <string>

std::pair<int64_t, int64_t> async_read_file(const std::string &filepath, CThreadsafeQueue_ptr buff);
std::pair<int64_t, int64_t> writeFile(const std::string &filepath, std::vector<CThreadsafeQueue_ptr> &vFromBuff);
std::pair<int64_t, int64_t> process(CThreadsafeQueue_ptr fromBuff, CThreadsafeQueue_ptr toBuff);

void rpw_test();
#endif
