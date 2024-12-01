cmake_minimum_required(VERSION 3.5.0)
project(enchat VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
#QTDIR=X:/Qt/6.7.2/msvc2019_64
list(APPEND CMAKE_PREFIX_PATH $ENV{QTDIR})

find_package(Qt6 CONFIG REQUIRED COMPONENTS Core)
find_package(MbedTLS REQUIRED)
# find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# find_package(CLI11 CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
# qt_standard_project_setup()
set(CMAKE_AUTOMOC TRUE)

set(MbedTLS_Lite_SRC
    crypto/mbedtls_config.h
    crypto/error.h crypto/error.c
    crypto/build_info.h crypto/private_access.h crypto/alignment.h crypto/check_config.h crypto/common.h
    crypto/platform.h crypto/platform.c crypto/platform_util.h crypto/platform_util.c
    crypto/chachapoly.h crypto/chachapoly.c crypto/poly1305.h crypto/poly1305.c
    crypto/chacha20.h crypto/chacha20.c
)

qt_add_library(enchat STATIC
    chatsession.h chatsession.cpp
    chatserver.h chatserver.cpp
    socketsession.h socketsession.cpp
    chatclient.h chatclient.cpp
    Peer.h
    chatcallback.h
    chatcallback.cpp
)
#add_subdirectory(curve25519)

target_include_directories(enchat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(enchat PUBLIC
    Qt6::Core
    MbedTLS::mbedcrypto
    # OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    # CLI11::CLI11
    asio::asio
    base)

# target_include_directories(enchat PRIV)
#target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /utf-8 /TP)
