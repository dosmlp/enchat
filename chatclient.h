#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <string>
#include <thread>
#include <QString>
#include <map>
#include "asio.hpp"
#include "chatsession.h"

using namespace asio::ip;
using io_context_guard = asio::executor_work_guard<asio::io_context::executor_type>;


class ChatClient
{
public:
    // using ChatSession = ChatSession<ChatClient>;
    ChatClient();
    ~ChatClient();
    void doConnect(const std::string &addr, uint16_t port, const QByteArray& peer_pubkey);
    void close();

    void setName(const QString& name);
    void sendTextMsg(uint64_t id, const QString& text);
    void setEcKey(const QByteArray& pri, const QByteArray& pub)
    {
        static_prikey_ = pri;
        static_pubkey_ = pub;
    }
    void getEcKey(QByteArray& pri, QByteArray& pub)
    {
        pri = static_prikey_;
        pub = static_pubkey_;
    }
    bool containsPeerPubkey(const QByteArray&);

    //回调
    void onConnected(const uint64_t id);
    void onClose(const uint64_t id);
    void onHandShakeFinished(const uint64_t id);
    void onTextMsg(const uint64_t id, const QString& text);
private:
    void connect(const tcp::endpoint &ep, const QByteArray& peer_pubkey);
    void run();
    asio::io_context io_ctx_;
    io_context_guard io_guard_;
    std::map<uint64_t,ChatSession<ChatClient>::Ptr> sess_map_;
    std::thread thread_run_;
    QString name_;
    QByteArray static_prikey_;
    QByteArray static_pubkey_;
};

#endif // CHATCLIENT_H
