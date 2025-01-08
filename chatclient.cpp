#include "chatclient.h"
#include "xlog.h"

ChatClient::ChatClient(QObject *parent, const QString &name):
    QObject(parent),
    io_ctx_(1),
    io_guard_(asio::make_work_guard(io_ctx_)),
    name_(name)
{
    thread_run_ = std::thread(std::bind(&ChatClient::run,this));
}

ChatClient::~ChatClient()
{
    io_guard_.reset();
    io_ctx_.stop();
    thread_run_.join();
}

void ChatClient::doConnect(const std::string &addr, uint16_t port, const QString &peer_pubkey)
{
    tcp::resolver resolver(io_ctx_);
    connect(resolver.resolve(addr,std::to_string(port)),peer_pubkey);
}

void ChatClient::close()
{
    for (const auto &ptr : sess_map_) {
        ptr.second->close();
    }
    sess_map_.clear();
}

void ChatClient::setName(const QString &name)
{
    name_ = name;
}

bool ChatClient::sendTextMsg(const QString &id, const QString &text)
{
    auto it = sess_map_.find(id);
    if (it == sess_map_.end()) {
        SERROR("find chatsession fail,id:{}",id.toStdString());
        return false;
    }
    ChatSession<ChatClient>::Ptr session = (*it).second;

    QByteArray u8text = text.toUtf8();

    std::unique_ptr<uint8_t> msg = std::unique_ptr<uint8_t>(new uint8_t[4096]);

    std::memcpy(msg.get()+4,u8text.data(),u8text.size());
    *((uint16_t*)(msg.get()+2)) = 1;//协议1

    session->writeMsg(std::move(msg),static_cast<uint16_t>(u8text.size()));
    return true;
}

bool ChatClient::containsPeerPubkey(const QByteArray &)
{
    SERROR("shouldn't run here.");
    return false;
}

void ChatClient::onConnected(const QString& id)
{
    emit sigConnected(id);
}

void ChatClient::onClose(const QString& id)
{
    sess_map_.erase(id);
    emit sigClose(id);
}

void ChatClient::onHandShakeFinished(const QString& id, ChatSession<ChatClient>::Ptr sess)
{
    SINFO("HandShakeFinished id:{}",id);
    emit sigHandshakeFinished(id);
}

void ChatClient::onTextMsg(const QString& id, const QString &text)
{
    emit sigTextMsg(id,text);
}

void ChatClient::connect(const tcp::resolver::results_type& endpoints, const QString &peer_pubkey)
{
    tcp::socket socket(io_ctx_);
    ChatSession<ChatClient>::Ptr session = std::make_shared<ChatSession<ChatClient>>(std::move(socket),this);
    session->setName(name_);
    session->setPeerPubkey(peer_pubkey);

    sess_map_.insert({peer_pubkey,session});

    asio::async_connect(session->getSocket(),endpoints,
                        [this,session](std::error_code ec, tcp::endpoint ep){
                            if (!ec) {
                                session->sendHandshake(1);
                                onConnected(session->id());
                            } else {
                                SERROR("connect to {}:{} error,{}",ep.address().to_string(),ep.port(),ec.message());
                            }
                        });

    // session->getSocket().async_connect(ep,[this,session](std::error_code ec) {


    // });
}

void ChatClient::run()
{
    io_ctx_.run();
}

