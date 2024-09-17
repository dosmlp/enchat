cmake_minimum_required(VERSION 3.5.0)
project(enchat VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB ed255_src "orlp/*")
file(GLOB x25519_src "agl/curve25519-donna-c64.c")
file(GLOB curve25519_src "boringssl/*")

find_package(MbedTLS REQUIRED)

add_library(ed25519 STATIC ${ed255_src})
#add_library(x25519 STATIC ${x25519_src})
add_library(curve25519 STATIC ${curve25519_src})

target_link_libraries(curve25519 PRIVATE MbedTLS::mbedcrypto)