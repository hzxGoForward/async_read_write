#pragma once
#ifndef _BOOST_READ_HZX_H
#define _BOOST_READ_HZX_H

#include <boost/asio.hpp>
#include "sync_queue.h"
#include <string>


int64_t async_read_file(const std::string &filepath, CSycnQueue_ptr_t &buff);
int64_t writeFile(const std::string &filepath, std::vector<CSycnQueue_ptr_t> &vFromBuff);
void rpw_test();
int64_t process(CSycnQueue_ptr_t fromBuff, CSycnQueue_ptr_t &toBuff);
#endif
