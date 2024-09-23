#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <thread>
#include <QString>
#include "asio.hpp"
#include "chatsession.h"

using namespace asio::ip;
using io_context_guard = asio::executor_work_guard<asio::io_context::executor_type>;

class ChatClient
{
public:
    ChatClient();
    ~ChatClient();
    void setServer(const std::string& addr, uint16_t port);
    void doConnect();
    void close();

    void setName(const QString& name);
    void sendTextMsg(const QString& text);
private:
    void connect(const tcp::endpoint &ep);
    void run();
    asio::io_context io_ctx_;
    tcp::endpoint endpoint_srv_;
    io_context_guard io_guard_;
    std::shared_ptr<ChatSession> session_;
    std::thread thread_run_;
    QString name_;
};

#endif // CHATCLIENT_H
