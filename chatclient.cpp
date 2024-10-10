#include "chatclient.h"
#include "xlog.h"

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

void ChatClient::doConnect(const std::string &addr, uint16_t port, const QByteArray &peer_pubkey)
{
    connect(tcp::endpoint(make_address(addr),port),peer_pubkey);
}

void ChatClient::close()
{
    for (auto ptr : sess_map_) {
        ptr.second->close();
    }
    sess_map_.clear();
}

void ChatClient::setName(const QString &name)
{
    name_ = name;
}

void ChatClient::sendTextMsg(uint64_t id, const QString &text)
{
    auto it = sess_map_.find(id);
    if (it == sess_map_.end()) {
        SERROR("find chatsession fail,id:{}",id);
        return;
    }
    ChatSession<ChatClient>::Ptr session = (*it).second;

    QByteArray u8text = text.toUtf8();

    std::unique_ptr<uint8_t> msg = std::unique_ptr<uint8_t>(new uint8_t[4096]);

    std::memcpy(msg.get()+4,u8text.data(),u8text.size());
    *((uint16_t*)(msg.get()+2)) = 1;

    session->writeMsg(std::move(msg),static_cast<uint16_t>(u8text.size()));
}

bool ChatClient::containsPeerPubkey(const QByteArray &)
{
    SERROR("shouldn't run here.");
    return false;
}

void ChatClient::onConnected(const uint64_t id)
{

}

void ChatClient::onClose(const uint64_t id)
{
    sess_map_.erase(id);
}

void ChatClient::onHandShakeFinished(const uint64_t id)
{
    SINFO("HandShakeFinished id:{}",id);
}

void ChatClient::onTextMsg(const uint64_t id, const QString &text)
{

}

void ChatClient::connect(const tcp::endpoint& ep, const QByteArray &peer_pubkey)
{
    tcp::socket socket(io_ctx_);
    ChatSession<ChatClient>::Ptr session = std::make_shared<ChatSession<ChatClient>>(std::move(socket),this);
    session->setName(name_);
    session->setPeerPubkey(peer_pubkey);

    sess_map_.insert({session->id(),session});

    session->getSocket().async_connect(ep,[this,session](std::error_code ec) {
        if (!ec) {
            session->sendHandshake(1);
            onConnected(session->id());
        } else {
            auto ep = session->getSocket().remote_endpoint();
            SERROR("connect to {}:{} error,{}",ep.address().to_string(),ep.port(),ec.message());
        }

    });
}

void ChatClient::run()
{
    io_ctx_.run();
}

