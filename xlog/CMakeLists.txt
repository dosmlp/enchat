cmake_minimum_required(VERSION 3.5.0)
project(xlog VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(MbedTLS CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core)
find_package(spdlog CONFIG REQUIRED)

add_library(xlog STATIC
    XLog.h XLog.cpp
    filehelper_chacha.h filehelper_chacha.cpp
    daily_filechacha_sink.h
    windebug_sink.h)

target_link_libraries(xlog PUBLIC MbedTLS::mbedcrypto Qt6::Core spdlog::spdlog_header_only)
