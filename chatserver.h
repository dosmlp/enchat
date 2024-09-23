#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include "asio.hpp"
#include "chatsession.h"

using namespace asio::ip;
using io_context_work = asio::executor_work_guard<asio::io_context::executor_type>;

class ChatServer
{
public:
    ChatServer(uint16_t port, int size = std::thread::hardware_concurrency()):
        acceptor_io_ctx_(1),
        acceptor_(acceptor_io_ctx_, tcp::endpoint(make_address("::"),port)),
        works_(size),
        client_io_ctxs_(size)
    {
        for (int i = 0;i < size;++i) {
            works_[i] = std::make_unique<io_context_work>(asio::make_work_guard(client_io_ctxs_[i]));
            threads_ioctxs_.emplace_back(std::thread([this,i](){
                client_io_ctxs_[i].run();
            }));
        }
        doAccept();
    }
    void run()
    {
        thread_acceptor_ = std::thread([this](){
            acceptor_io_ctx_.run();
        });
    }
    ~ChatServer();
private:
    void doAccept();

    asio::io_context& getIocontext()
    {
        auto& io = client_io_ctxs_[next_io_];
        if (next_io_ == client_io_ctxs_.size()) next_io_ = 0;
        return io;
    }


    asio::io_context acceptor_io_ctx_;
    tcp::acceptor acceptor_;
    std::thread thread_acceptor_;

    uint16_t next_io_ = 0;
    std::vector<std::unique_ptr<io_context_work>> works_;
    std::vector<asio::io_context> client_io_ctxs_;
    std::vector<std::thread> threads_ioctxs_;
};

#endif // CHATSERVER_H
