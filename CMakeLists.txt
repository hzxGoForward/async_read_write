CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(async_read_write VERSION 0.1.0)
SET(SOURCES ./src/main.cpp ./src/buff.cpp)
ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})
# 允许c++11标准、O3优化、多线程。match选项可避免一些cpu上的问题
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -march=native -O3 -pthread" )
endif()
#设定可执行二进制文件的目录
SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) 