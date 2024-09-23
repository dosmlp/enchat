#include "chatclient.h"
#include "XLog.h"

ChatClient::ChatClient():
    io_ctx_(1),
    io_guard_(asio::make_work_guard(io_ctx_))
{
    thread_run_ = std::thread(std::bind(&ChatClient::run,this));
}

ChatClient::~ChatClient()
{
    io_guard_.reset();
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

void ChatClient::setName(const QString &name)
{
    name_ = name;
}

void ChatClient::sendTextMsg(const QString &text)
{
    if (!session_) {
        SINFO("chatsession is null.");
        return;
    }
    QByteArray u8text = text.toUtf8();

    std::unique_ptr<uint8_t> msg = std::unique_ptr<uint8_t>(new uint8_t[4096]);

    std::memcpy(msg.get()+4,u8text.data(),u8text.size());
    *((uint16_t*)(msg.get()+2)) = 1;

    session_->writeMsg(std::move(msg),u8text.size());
}

void ChatClient::connect(const tcp::endpoint& ep)
{
    tcp::socket socket(io_ctx_);
    session_ = std::make_shared<ChatSession>(std::move(socket));
    session_->setName(name_);

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

