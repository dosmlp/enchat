//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mbedtls/ecp.h>
#include <memory>
#include <system_error>
#include <thread>
#include "asio.hpp"
#include "asio/buffer.hpp"
#include "asio/completion_condition.hpp"
#include "asio/impl/read.hpp"
#include "asio/impl/write.hpp"
#include <mbedtls/ecdh.h>
#include "mbedtls.h"

using asio::ip::tcp;

class ChatClient
{
public:
    ChatClient(asio::io_context& io_context, const tcp::resolver::results_type& endpoints) : 
    io_context_(io_context),
    socket_(io_context)
    {
        do_connect(endpoints);
    }


    void close()
    {
        asio::post(io_context_, [this]() { socket_.close(); });
    }

private:
    void sendHello()
    {
        mbedtls_ecdh_free(&ecdh_ctx_);
        mbedtls_ecdh_init(&ecdh_ctx_);
        int ret = mbedtls_ecdh_setup(&ecdh_ctx_, MBEDTLS_ECP_DP_CURVE25519);
        if (ret != 0) {
            std::cerr << "mbedtls_ecdh_setup fail "<< ret << "\n"; 
            return;
        }
        std::shared_ptr<uint8_t> cli2svr((uint8_t*)malloc(64),[](void* p){free(p);});
        size_t outlen = 0;
        ret = mbedtls_ecdh_make_params(&ecdh_ctx_, &outlen, cli2svr.get(), 64, mbedtls_ctr_drbg_random, &drbg);
        if (ret != 0) {
            std::cerr << "mbedtls_ecdh_make_params fail "<< ret << "\n"; 
            return;
        }

        asio::async_write(socket_,
            asio::buffer(cli2svr.get(),64),
            asio::transfer_exactly(outlen),
            [this,cli2svr](std::error_code ec,size_t size){
                if (!ec) {
                    calcSecret();
                }
        });
    }
    void calcSecret()
    {
        asio::async_read(socket_,asio::buffer(),asio::transfer_exactly(64),[](std::error_code ec){
            
        });
    }
    void do_connect(const tcp::resolver::results_type& endpoints)
    {
      asio::async_connect(socket_, endpoints,
          [this](std::error_code ec, tcp::endpoint)
          {
              if (!ec) {
                sendHello();
              }
          });
    }


    void do_write()
    {

    }

private:
  asio::io_context& io_context_;
  tcp::socket socket_;
  mbedtls_ecdh_context ecdh_ctx_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);

    std::thread t([&io_context](){ io_context.run(); });

    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      chat_message msg;
      msg.body_length(std::strlen(line));
      std::memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
