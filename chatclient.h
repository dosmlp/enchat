#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <thread>
#include "asio.hpp"
#include "chatsession.h"

using namespace asio::ip;

class ChatClient
{
public:
    ChatClient();
    ~ChatClient();
    void setServer(const std::string& addr, uint16_t port);
    void doConnect();
    void close();
private:
    void connect(const tcp::endpoint &ep);
    void run();
    asio::io_context io_ctx_;
    tcp::endpoint endpoint_srv_;
    std::shared_ptr<ChatSession> session_;

    std::thread thread_run_;

};

#endif // CHATCLIENT_H
