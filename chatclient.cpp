#include "chatclient.h"
#include "XLog.h"

ChatClient::ChatClient():
    io_ctx_(1)
{
    thread_run_ = std::thread(std::bind(&ChatClient::run,this));
}

ChatClient::~ChatClient()
{
    io_ctx_.stop();
    thread_run_.join();
}

void ChatClient::setServer(const std::string &addr, uint16_t port)
{
    endpoint_srv_ = tcp::endpoint(make_address(addr),port);
}

void ChatClient::doConnect()
{
    connect(endpoint_srv_);
}

void ChatClient::close()
{
    session_.reset();
}

void ChatClient::connect(const tcp::endpoint& ep)
{
    tcp::socket socket(io_ctx_);
    session_ = std::make_shared<ChatSession>(std::move(socket));
    session_->getSocket().async_connect(ep,[this](std::error_code ec) {
        if (!ec) {
            session_->sendHandshake(1);
        } else {
            SERROR("connect to {}:{} error,{}",endpoint_srv_.address().to_string(),endpoint_srv_.port(),ec.message());
        }

    });
}

void ChatClient::run()
{
    io_ctx_.run();
}

