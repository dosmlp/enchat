cmake_minimum_required(VERSION 3.5.0)
project(xlog VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(MbedTLS REQUIRED)

add_library(xlog STATIC
    XLog.h XLog.cpp)

target_include_directories(xlog PUBLIC spdlog-1.12.0/include)
target_link_libraries(xlog PUBLIC MbedTLS::mbedcrypto)
