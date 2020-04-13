cmake_minimum_required(VERSION 3.15)
project(syslog_client)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(syslog_client main.cpp include/syslog/client.hpp)